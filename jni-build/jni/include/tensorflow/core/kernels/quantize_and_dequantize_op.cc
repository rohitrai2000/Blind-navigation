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

#define EIGEN_USE_THREADS

#if GOOGLE_CUDA
#define EIGEN_USE_GPU
#endif  // GOOGLE_CUDA

#include "tensorflow/core/kernels/quantize_and_dequantize_op.h"

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/type_traits.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/errors.h"

namespace tensorflow {

typedef Eigen::ThreadPoolDevice CPUDevice;
typedef Eigen::GpuDevice GPUDevice;

// Simulate quantization precision loss in a float tensor by:
// 1. Quantize the tensor to fixed point numbers, which should match the target
//    quantization method when it is used in inference.
// 2. Dequantize it back to floating point numbers for the following ops, most
//    likely matmul.
template <typename Device, typename T>
class QuantizeAndDequantizeOp : public OpKernel {
 public:
  explicit QuantizeAndDequantizeOp(OpKernelConstruction* ctx) : OpKernel(ctx) {
    OP_REQUIRES_OK(ctx, ctx->GetAttr("signed_input", &signed_input_));
    OP_REQUIRES_OK(ctx, ctx->GetAttr("num_bits", &num_bits_));
    OP_REQUIRES(ctx, num_bits_ > 0 && num_bits_ < (signed_input_ ? 62 : 63),
                errors::InvalidArgument("num_bits is out of range: ", num_bits_,
                                        " with signed_input_ ", signed_input_));
    OP_REQUIRES_OK(ctx, ctx->GetAttr("range_given", &range_given_));
    OP_REQUIRES_OK(ctx, ctx->GetAttr("input_min", &input_min_));
    OP_REQUIRES_OK(ctx, ctx->GetAttr("input_max", &input_max_));
    if (range_given_) {
      OP_REQUIRES(
          ctx, input_min_ <= input_max_,
          errors::InvalidArgument("Invalid range: input_min ", input_min_,
                                  " > input_max ", input_max_));
    }
  }

  void Compute(OpKernelContext* ctx) override {
    const Tensor& input = ctx->input(0);

    Tensor* output = nullptr;
    OP_REQUIRES_OK(ctx, ctx->allocate_output(0, input.shape(), &output));

    // One global scale.
    Tensor input_min_tensor;
    Tensor input_max_tensor;
    OP_REQUIRES_OK(ctx, ctx->allocate_temp(DataTypeToEnum<T>::value,
                                           TensorShape(), &input_min_tensor));
    OP_REQUIRES_OK(ctx, ctx->allocate_temp(DataTypeToEnum<T>::value,
                                           TensorShape(), &input_max_tensor));

    auto input_min = input_min_tensor.scalar<T>();
    auto input_max = input_max_tensor.scalar<T>();

    input_min() = input_min_;
    input_max() = input_max_;

    functor::QuantizeAndDequantizeOneScaleFunctor<Device, T> functor;
    functor(ctx->template eigen_device<Device>(), input.template flat<T>(),
            signed_input_, num_bits_, range_given_, input_min, input_max,
            output->template flat<T>());
  }

 private:
  bool signed_input_;
  int num_bits_;
  bool range_given_;
  float input_min_;
  float input_max_;
};

// Specialization for CPUDevice.
namespace functor {
template <typename T>
struct QuantizeAndDequantizeOneScaleFunctor<CPUDevice, T> {
  void operator()(const CPUDevice& d, typename TTypes<T>::ConstVec input,
                  const bool signed_input, const int num_bits,
                  const bool range_given, typename TTypes<T>::Scalar input_min,
                  typename TTypes<T>::Scalar input_max,
                  typename TTypes<T>::Vec out) {
    QuantizeAndDequantizeOneScaleImpl<CPUDevice, T>::Compute(
        d, input, signed_input, num_bits, range_given, input_min, input_max,
        out);
  }
};
}  // namespace functor

#define REGISTER_CPU_KERNEL(T)                                                 \
  REGISTER_KERNEL_BUILDER(                                                     \
      Name("QuantizeAndDequantize").Device(DEVICE_CPU).TypeConstraint<T>("T"), \
      QuantizeAndDequantizeOp<CPUDevice, T>);
TF_CALL_float(REGISTER_CPU_KERNEL);
TF_CALL_double(REGISTER_CPU_KERNEL);
#undef REGISTER_CPU_KERNEL

#if GOOGLE_CUDA
REGISTER_KERNEL_BUILDER(
    Name("QuantizeAndDequantize").Device(DEVICE_GPU).TypeConstraint<float>("T"),
    QuantizeAndDequantizeOp<GPUDevice, float>);

REGISTER_KERNEL_BUILDER(Name("QuantizeAndDequantize")
                            .Device(DEVICE_GPU)
                            .TypeConstraint<double>("T"),
                        QuantizeAndDequantizeOp<GPUDevice, double>);
#endif
}  // namespace tensorflow
