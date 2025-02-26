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

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif  // GOOGLE_CUDA

#include "tensorflow/core/kernels/reverse_sequence_op.h"

#include <memory>
#include <vector>
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/macros.h"

namespace tensorflow {

typedef Eigen::ThreadPoolDevice CPUDevice;
typedef Eigen::GpuDevice GPUDevice;

template <typename Device>
void CheckErrors(OpKernelContext* context, int batch_dim, int seq_dim) {
  const Tensor& input = context->input(0);
  const Tensor& seq_lens = context->input(1);

  auto seq_lens_t = seq_lens.vec<int64>();

  std::vector<int64> seq_lens_vec(seq_lens_t.size());

  // Copy seq_len info down for validity checks
  context->eigen_device<Device>().memcpyDeviceToHost(
      seq_lens_vec.data(), seq_lens_t.data(),
      sizeof(int64) * seq_lens_t.size());

  OP_REQUIRES(context, batch_dim != seq_dim,
              errors::InvalidArgument("batch_dim == seq_dim == ", seq_dim));
  OP_REQUIRES(context, seq_dim < input.dims(),
              errors::InvalidArgument("seq_dim must be < input.dims()", "( ",
                                      seq_dim, " vs. ", input.dims(), ")"));
  OP_REQUIRES(context, batch_dim < input.dims(),
              errors::InvalidArgument("batch_dim must be < input.dims()", "( ",
                                      batch_dim, " vs. ", input.dims(), ")"));
  OP_REQUIRES(context, seq_lens.NumElements() == input.dim_size(batch_dim),
              errors::InvalidArgument("len(seq_lens) != input.dims(", batch_dim,
                                      "), ", "(", seq_lens.NumElements(),
                                      " vs. ", input.dim_size(batch_dim)));

  for (size_t d = 0; d < seq_lens_vec.size(); ++d) {
    OP_REQUIRES(context, seq_lens_vec[d] >= 0,
                errors::InvalidArgument("seq_lens(", d, ") < 0"));
    OP_REQUIRES(context, seq_lens_vec[d] <= input.dim_size(seq_dim),
                errors::InvalidArgument("seq_lens(", d, ") > input.dims(",
                                        seq_dim, ")"));
  }
}

template <>
void CheckErrors<GPUDevice>(OpKernelContext* context, int batch_dim,
                            int seq_dim) {
  const Tensor& input = context->input(0);
  const Tensor& seq_lens = context->input(1);

  OP_REQUIRES(context, batch_dim != seq_dim,
              errors::InvalidArgument("batch_dim == seq_dim == ", seq_dim));
  OP_REQUIRES(context, seq_dim < input.dims(),
              errors::InvalidArgument("seq_dim must be < input.dims()", "( ",
                                      seq_dim, " vs. ", input.dims(), ")"));
  OP_REQUIRES(context, batch_dim < input.dims(),
              errors::InvalidArgument("batch_dim must be < input.dims()", "( ",
                                      batch_dim, " vs. ", input.dims(), ")"));

  OP_REQUIRES(context, seq_lens.NumElements() == input.dim_size(batch_dim),
              errors::InvalidArgument("len(seq_lens) != input.dims(", batch_dim,
                                      "), ", "(", seq_lens.NumElements(),
                                      " vs. ", input.dim_size(batch_dim)));
}

template <typename Device, typename T>
class ReverseSequenceOp : public OpKernel {
 public:
  explicit ReverseSequenceOp(OpKernelConstruction* context)
      : OpKernel(context) {
    OP_REQUIRES_OK(context, context->GetAttr("batch_dim", &batch_dim_));
    OP_REQUIRES_OK(context, context->GetAttr("seq_dim", &seq_dim_));
  }

  void Compute(OpKernelContext* context) override {
    const Tensor& input = context->input(0);
    const Tensor& seq_lens = context->input(1);

    // Preliminary validation of sizes.
    OP_REQUIRES(context, TensorShapeUtils::IsVector(seq_lens.shape()),
                errors::InvalidArgument("seq_lens input must be 1-dim, not ",
                                        seq_lens.dims()));

    auto seq_lens_t = seq_lens.vec<int64>();

    CheckErrors<Device>(context, batch_dim_, seq_dim_);

    const int input_dims = input.dims();

    Tensor* output = nullptr;
    OP_REQUIRES_OK(context,
                   context->allocate_output(0, input.shape(), &output));

#define HANDLE_DIM(NDIM)                                                      \
  case NDIM:                                                                  \
    functor::ReverseSequence<Device, T, NDIM>::Compute(                       \
        context->eigen_device<Device>(), input.tensor<T, NDIM>(), batch_dim_, \
        seq_dim_, seq_lens_t, output->tensor<T, NDIM>());                     \
    break;

    switch (input_dims) {
      HANDLE_DIM(2);
      HANDLE_DIM(3);
      HANDLE_DIM(4);
      HANDLE_DIM(5);

      default:
        OP_REQUIRES(context, false,
                    errors::InvalidArgument(
                        "ReverseSequenceOp : Unhandled input dimensions: ",
                        input_dims));
    }
  }

 private:
  int32 batch_dim_;
  int32 seq_dim_;

  TF_DISALLOW_COPY_AND_ASSIGN(ReverseSequenceOp);
};

#define REGISTER_REVERSE_SEQUENCE(type)                                     \
  REGISTER_KERNEL_BUILDER(                                                  \
      Name("ReverseSequence").Device(DEVICE_CPU).TypeConstraint<type>("T"), \
      ReverseSequenceOp<CPUDevice, type>);

TF_CALL_NUMBER_TYPES(REGISTER_REVERSE_SEQUENCE);

#if GOOGLE_CUDA

// Forward declarations of the functor specializations for GPU.
namespace functor {
#define DECLARE_GPU_SPEC(T, Dims)                                       \
  template <>                                                           \
  void ReverseSequence<GPUDevice, T, Dims>::Compute(                    \
      const GPUDevice& d, typename TTypes<T, Dims>::ConstTensor input,  \
      int32 batch_dim, int32 seq_dim, TTypes<int64>::ConstVec seq_lens, \
      typename TTypes<T, Dims>::Tensor output);                         \
  extern template struct ReverseSequence<GPUDevice, T, Dims>;

#define DECLARE_GPU_SPECS(T) \
  DECLARE_GPU_SPEC(T, 2);    \
  DECLARE_GPU_SPEC(T, 3);    \
  DECLARE_GPU_SPEC(T, 4);    \
  DECLARE_GPU_SPEC(T, 5);

TF_CALL_GPU_NUMBER_TYPES(DECLARE_GPU_SPECS);

}  // namespace functor

// Registration of the GPU implementations.
#define REGISTER_REVERSE_SEQUENCE_GPU(type)                                 \
  REGISTER_KERNEL_BUILDER(                                                  \
      Name("ReverseSequence").Device(DEVICE_GPU).TypeConstraint<type>("T"), \
      ReverseSequenceOp<GPUDevice, type>);

TF_CALL_GPU_NUMBER_TYPES(REGISTER_REVERSE_SEQUENCE_GPU);

#undef REGISTER_REVERSE_SEQUENCE_GPU

#endif  // GOOGLE_CUDA

}  // namespace tensorflow
