/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// See docs in ../ops/array_ops.cc.

#define EIGEN_USE_THREADS

#include "tensorflow/core/kernels/transpose_op.h"

#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/kernels/bounds_check.h"
#include "tensorflow/core/kernels/transpose_functor.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/platform/logging.h"

namespace tensorflow {

// inv = InvertPermutationOp(T<int32> p) takes a permutation of
// integers 0, 1, ..., n - 1 and returns the inverted
// permutation of p. I.e., inv[p[i]] == i, for i in [0 .. n).
//
// REQUIRES: input is a vector of int32.
// REQUIRES: input is a permutation of 0, 1, ..., n-1.

class InvertPermutationOp : public OpKernel {
 public:
  explicit InvertPermutationOp(OpKernelConstruction* context)
      : OpKernel(context) {}

  void Compute(OpKernelContext* context) override {
    const Tensor& input = context->input(0);
    OP_REQUIRES(
        context, TensorShapeUtils::IsVector(input.shape()),
        errors::InvalidArgument("invert_permutation expects a 1D vector."));
    auto Tin = input.vec<int32>();
    OP_REQUIRES(context,
                FastBoundsCheck(Tin.size(), std::numeric_limits<int32>::max()),
                errors::InvalidArgument("permutation of nonnegative int32s "
                                        "must have <= int32 max elements"));
    const int32 N =
        static_cast<int32>(Tin.size());  // Safe: bounds-checked above.
    Tensor* output = nullptr;
    OP_REQUIRES_OK(context,
                   context->allocate_output(0, input.shape(), &output));
    auto Tout = output->vec<int32>();
    std::fill_n(Tout.data(), N, -1);
    for (int i = 0; i < N; ++i) {
      const int32 d = internal::SubtleMustCopy(Tin(i));
      OP_REQUIRES(context, FastBoundsCheck(d, N),
                  errors::InvalidArgument(d, " is not between 0 and ", N));
      OP_REQUIRES(context, Tout(d) == -1,
                  errors::InvalidArgument(d, " is duplicated in the input."));
      Tout(d) = i;
    }
  }
};

REGISTER_KERNEL_BUILDER(Name("InvertPermutation").Device(DEVICE_CPU),
                        InvertPermutationOp);

REGISTER_KERNEL_BUILDER(Name("InvertPermutation")
                            .Device(DEVICE_GPU)
                            .HostMemory("x")
                            .HostMemory("y"),
                        InvertPermutationOp);

// output = TransposeOp(T<any> input, T<int32> perm) takes a tensor
// of type T and rank N, and a permutation of 0, 1, ..., N-1. It
// shuffles the dimensions of the input tensor according to permutation.
//
// Specifically, the returned tensor output meets the following condition:
// 1) output.dims() == input.dims();
// 2) output.dim_size(i) == input.dim_size(perm[i]);
// 3) output.tensor<T, N>(i_0, i_1, ..., i_N-1) ==
//      input.tensor<T, N>(j_0, j_1, ..., j_N-1),
//    where i_s == j_{perm[s]}
//
// REQUIRES: perm is a vector of int32.
// REQUIRES: input.dims() == perm.size().
// REQUIRES: perm is a permutation.

void TransposeOp::Compute(OpKernelContext* ctx) {
  const Tensor& input = ctx->input(0);
  const Tensor& perm = ctx->input(1);
  // Preliminary validation of sizes.
  OP_REQUIRES(ctx, TensorShapeUtils::IsVector(perm.shape()),
              errors::InvalidArgument("perm must be a vector, not ",
                                      perm.shape().DebugString()));
  auto Vperm = perm.vec<int32>();
  const int dims = input.dims();
  static const int kMinDims = 0;
  static const int kMaxDims = 10;
  OP_REQUIRES(ctx, kMinDims <= dims && dims <= kMaxDims,
              errors::Unimplemented("Transposing a tensor of rank ", dims,
                                    " is not implemented."));
  OP_REQUIRES(ctx, dims == Vperm.size(),
              errors::InvalidArgument(
                  "transpose expects a vector of size ", input.dims(),
                  ". But input(1) is a vector of size ", Vperm.size()));
  // using volatile instead of SubtleMustCopy here so that the
  // asynchrony boundary is permutation.
  const volatile int32* perm_begin =
      reinterpret_cast<const volatile int32*>(Vperm.data());
  const std::vector<int32> permutation(perm_begin, perm_begin + dims);
  TensorShape shape;

  // Check whether permutation is a permutation of integers of [0 .. dims).
  gtl::InlinedVector<bool, 8> bits(dims);
  bool is_identity = true;
  int32 non_singleton_dims = 0;
  for (int i = 0; i < dims; ++i) {
    const int32 d = permutation[i];
    OP_REQUIRES(
        ctx, 0 <= d && d < dims,
        errors::InvalidArgument(d, " is out of range [0 .. ", dims, ")"));
    bits[d] = true;
    const auto dim_size = input.dim_size(d);
    shape.AddDim(dim_size);
    non_singleton_dims += dim_size != 1 ? 1 : 0;
    if (d != i) {
      is_identity = false;
    }
  }
  for (int i = 0; i < dims; ++i) {
    OP_REQUIRES(ctx, bits[i], errors::InvalidArgument(
                                  i, " is missing from {",
                                  str_util::Join(permutation, ","), "}."));
  }

  // 0-D, 1-D, and identity transposes do nothing.
  if (dims <= 1 || is_identity) {
    ctx->set_output(0, input);
    return;
  } else if (non_singleton_dims <= 1) {
    Tensor output;
    OP_REQUIRES(ctx, output.CopyFrom(input, shape),
                errors::Unknown("Error reshaping Tensor."));
    ctx->set_output(0, output);
    return;
  }

  Tensor* output = nullptr;
  OP_REQUIRES_OK(ctx, ctx->allocate_output(0, shape, &output));
  if (shape.num_elements() > 0) {
    OP_REQUIRES_OK(ctx, DoTranspose(ctx, input, permutation, output));
  }
}

Status TransposeCpuOp::DoTranspose(OpKernelContext* ctx, const Tensor& in,
                                   gtl::ArraySlice<int32> perm, Tensor* out) {
  typedef Eigen::ThreadPoolDevice CPUDevice;
  return ::tensorflow::DoTranspose(ctx->eigen_device<CPUDevice>(), in, perm,
                                   out);
}

#define REGISTER(T)                                   \
  REGISTER_KERNEL_BUILDER(Name("Transpose")           \
                              .Device(DEVICE_CPU)     \
                              .TypeConstraint<T>("T") \
                              .HostMemory("perm"),    \
                          TransposeCpuOp);
TF_CALL_ALL_TYPES(REGISTER)
REGISTER(bfloat16);
#undef REGISTER

#if GOOGLE_CUDA
Status TransposeGpuOp::DoTranspose(OpKernelContext* ctx, const Tensor& in,
                                   gtl::ArraySlice<int32> perm, Tensor* out) {
  typedef Eigen::GpuDevice GPUDevice;
  return ::tensorflow::DoTranspose(ctx->eigen_device<GPUDevice>(), in, perm,
                                   out);
}

#define REGISTER(T)                                   \
  REGISTER_KERNEL_BUILDER(Name("Transpose")           \
                              .Device(DEVICE_GPU)     \
                              .TypeConstraint<T>("T") \
                              .HostMemory("perm"),    \
                          TransposeGpuOp);
TF_CALL_POD_TYPES(REGISTER);
#undef REGISTER
#endif

}  // namespace tensorflow
