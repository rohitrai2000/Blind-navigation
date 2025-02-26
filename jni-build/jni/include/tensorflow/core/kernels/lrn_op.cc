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

// LRN = Local Response Normalization
// See docs in ../ops/nn_ops.cc.

#define EIGEN_USE_THREADS

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/kernels/bounds_check.h"
#include "tensorflow/core/kernels/ops_util.h"
#include "tensorflow/core/lib/core/errors.h"

#if !defined(IS_MOBILE_PLATFORM)
#include "tensorflow/core/util/work_sharder.h"
#endif

#if GOOGLE_CUDA
#include "cuda/include/cuda.h"
#include "tensorflow/core/platform/stream_executor.h"
#include "tensorflow/core/util/stream_executor_util.h"
#endif  // GOOGLE_CUDA

namespace tensorflow {

namespace {

// When the depth is large and beta_ is 0.5 or 1.0, Single-threaded
// LRN is faster than the main band matrix approach used
// below. Benchmarks suggest switching to SingleThreadedLRN when depth > 384.
const int kSingleThreadedLRNDepthCutoff = 384;

// Create a depth-by-depth band matrix with 1s along a swath of size (2 *
// depth_radius + 1) around the diagonal.
template <typename T>
void GetBandMatrix(int depth, int depth_radius,
                   Eigen::Tensor<T, 2, Eigen::RowMajor>* result) {
  result->setZero();
  for (int row = 0; row < depth; ++row) {
    const int begin = std::max<int>(0, row - depth_radius);
    const int end = std::min<int>(depth, row + depth_radius + 1);
    Eigen::DSizes<Eigen::DenseIndex, 2> start(row, begin);
    Eigen::DSizes<Eigen::DenseIndex, 2> sizes(1, end - begin);
    result->slice(start, sizes).setConstant(T(1));
  }
}

}  // namespace

typedef Eigen::ThreadPoolDevice CPUDevice;
typedef Eigen::GpuDevice GPUDevice;

template <typename Device, typename T>
struct LaunchLRN;

template <typename T>
struct LaunchLRN<CPUDevice, T> {
  LaunchLRN(int depth_radius, T bias, T alpha, T beta)
      : depth_radius_(depth_radius), bias_(bias), alpha_(alpha), beta_(beta) {}

  void launch(OpKernelContext* context, OpKernel* kernel, const Tensor& in,
              Tensor* output) {
    const int batch = static_cast<int>(in.dim_size(0));
    const int rows = static_cast<int>(in.dim_size(1));
    const int cols = static_cast<int>(in.dim_size(2));
    const int depth = static_cast<int>(in.dim_size(3));
    const int nodes = cols * rows;

#if defined(IS_MOBILE_PLATFORM)
    SingleThreadedLRN(in, batch, rows, cols, depth, output);
#else
    if (depth > kSingleThreadedLRNDepthCutoff &&
        (beta_ == T(0.5) || beta_ == T(1))) {
      SingleThreadedLRN(in, batch, rows, cols, depth, output);
      return;
    }

    auto in_shaped = in.shaped<T, 2>({nodes * batch, depth});

    // Multiplying the input with the band matrix has the effect of reducing the
    // correct patch along the depth.
    Eigen::Tensor<T, 2, Eigen::RowMajor> multiplier(depth, depth);
    GetBandMatrix<T>(depth, depth_radius_, &multiplier);

    auto out_shaped = output->shaped<T, 2>({nodes * batch, depth});
    Eigen::array<DimPair, 1> dims = {{DimPair(1, 0)}};
    auto tmp = in_shaped.square().contract(multiplier, dims) * alpha_ + bias_;
    if (beta_ == T(1)) {
      out_shaped.device(context->eigen_cpu_device()) =
          in_shaped * tmp.inverse();
    } else if (beta_ == T(0.5)) {
      out_shaped.device(context->eigen_cpu_device()) = in_shaped * tmp.rsqrt();
    } else {
      out_shaped.device(context->eigen_cpu_device()) =
          in_shaped * (tmp.log() * -beta_).exp();
    }
#endif
  }

 private:
  typedef typename Eigen::Tensor<T, 1, Eigen::RowMajor>::DimensionPair DimPair;

  void SingleThreadedLRN(const Tensor& in, const int batch, const int rows,
                         const int cols, const int depth, Tensor* out) {
    Eigen::Map<const Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> data_in(
        in.flat<T>().data(), depth, batch * rows * cols);

    Eigen::Map<Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>> data_out(
        out->flat<T>().data(), depth, batch * rows * cols);

    const int double_depth_radius = depth_radius_ * 2;
    Eigen::Matrix<T, Eigen::Dynamic, 1> padded_square(data_in.rows() +
                                                      double_depth_radius);
    padded_square.setZero();
    for (int r = 0; r < data_in.cols(); ++r) {
      // Do local response normalization for data_in(:, r). First, compute the
      // square and store them in buffer for repeated use.
      padded_square.block(depth_radius_, 0, data_out.rows(), 1) =
          data_in.col(r).cwiseProduct(data_in.col(r)) * alpha_;
      // Then, compute the scale and write it to data_out.
      T accumulated_scale(0);
      for (int i = 0; i < double_depth_radius; ++i) {
        accumulated_scale += padded_square(i);
      }
      for (int i = 0; i < data_in.rows(); ++i) {
        accumulated_scale += padded_square(i + double_depth_radius);
        data_out(i, r) = bias_ + accumulated_scale;
        accumulated_scale -= padded_square(i);
      }
    }

    if (beta_ == T(1)) {
      data_out.array() = data_in.array() * data_out.array().inverse();
    } else if (beta_ == T(0.5)) {
      data_out.array() = data_in.array() * data_out.array().rsqrt();
    } else {
      data_out.array() =
          data_in.array() * (data_out.array().log() * -beta_).exp();
    }
  }

  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#if GOOGLE_CUDA

template <typename T>
struct LaunchLRN<GPUDevice, T> {
  LaunchLRN(int depth_radius, T bias, T alpha, T beta)
      : depth_radius_(depth_radius), bias_(bias), alpha_(alpha), beta_(beta) {}

  void launch(OpKernelContext* context, OpKernel* kernel, const Tensor& in,
              Tensor* output) {
    OP_REQUIRES(
        context, beta_ >= 0.01,
        errors::InvalidArgument("cuDNN requires beta >= 0.01, got: ", beta_));

    OP_REQUIRES(
        context, depth_radius_ > 0 && depth_radius_ <= 7,
        errors::InvalidArgument("cuDNN requires depth_radius in [1, 7], got: ",
                                depth_radius_));
    OP_REQUIRES(
        context, bias_ >= 1e-5,
        errors::InvalidArgument("cuDNN requires bias >= 1e-5, got: ", bias_));

    // Cast to platform-specific int to avoid conversion warnings.
    const int batch = static_cast<int>(in.dim_size(0));
    const int rows = static_cast<int>(in.dim_size(1));
    const int cols = static_cast<int>(in.dim_size(2));
    const int depth = static_cast<int>(in.dim_size(3));

    perftools::gputools::dnn::BatchDescriptor dimensions_desc;
    dimensions_desc.set_count(batch)
        .set_height(rows)
        .set_width(cols)
        .set_feature_map_count(depth)
        .set_layout(perftools::gputools::dnn::DataLayout::kBatchYXDepth);

    perftools::gputools::dnn::NormalizeDescriptor normalize_desc;
    normalize_desc.set_bias(bias_)
        .set_range(depth_radius_)
        .set_alpha(alpha_)
        .set_beta(beta_);

    auto input_data = StreamExecutorUtil::AsDeviceMemory<T>(in);
    auto output_data = StreamExecutorUtil::AsDeviceMemory<T>(*output);

    auto* stream = context->op_device_context()->stream();
    OP_REQUIRES(context, stream, errors::Internal("No GPU stream available."));

    bool status =
        stream
            ->ThenNormalizeWithDimensions(normalize_desc, dimensions_desc,
                                          input_data, &output_data)
            .ok();
    OP_REQUIRES(context, status,
                errors::Internal("NormalizeWithDimensions launch failed"));
  }

  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#endif  // GOOGLE_CUDA

template <typename Device, typename T>
class LRNOp : public OpKernel {
 public:
  explicit LRNOp(OpKernelConstruction* context) : OpKernel(context) {
    int64 depth_radius64;
    OP_REQUIRES_OK(context, context->GetAttr("depth_radius", &depth_radius64));
    OP_REQUIRES(context, FastBoundsCheck(depth_radius64,
                                         std::numeric_limits<int>::max()),
                errors::InvalidArgument("depth_radius = ", depth_radius64,
                                        " larger than int max"));
    depth_radius_ = static_cast<int>(depth_radius64);
    float tmp;
    OP_REQUIRES_OK(context, context->GetAttr("bias", &tmp));
    bias_ = T(tmp);
    OP_REQUIRES_OK(context, context->GetAttr("alpha", &tmp));
    alpha_ = T(tmp);
    OP_REQUIRES_OK(context, context->GetAttr("beta", &tmp));
    beta_ = T(tmp);
  }

  void Compute(OpKernelContext* context) override {
    const Tensor& in = context->input(0);
    OP_REQUIRES(context, in.dims() == 4,
                errors::InvalidArgument("in must be 4-dimensional"));
    OP_REQUIRES(context, FastBoundsCheck(in.NumElements(),
                                         std::numeric_limits<int>::max()),
                errors::InvalidArgument("argument to LRN too large"));
    // Cast to platform-specific int to avoid conversion warnings.
    const int batch = static_cast<int>(in.dim_size(0));
    const int rows = static_cast<int>(in.dim_size(1));
    const int cols = static_cast<int>(in.dim_size(2));
    const int depth = static_cast<int>(in.dim_size(3));

    OP_REQUIRES(context,
                (depth + depth_radius_) <= std::numeric_limits<int>::max(),
                errors::InvalidArgument("depth ", depth, " + depth_radius ",
                                        depth_radius_, " exceeds int max."));

    Tensor* output = nullptr;
    OP_REQUIRES_OK(context,
                   context->allocate_output(
                       0, TensorShape({batch, rows, cols, depth}), &output));

    LaunchLRN<Device, T> launcher(depth_radius_, bias_, alpha_, beta_);
    launcher.launch(context, this, in, output);
  }

 private:
  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#define REGISTER_CPU(T)                                      \
  REGISTER_KERNEL_BUILDER(                                   \
      Name("LRN").Device(DEVICE_CPU).TypeConstraint<T>("T"), \
      LRNOp<CPUDevice, T>);
TF_CALL_float(REGISTER_CPU);
TF_CALL_half(REGISTER_CPU);

#undef REGISTER_CPU

#if GOOGLE_CUDA

#define REGISTER_GPU(T)                                      \
  REGISTER_KERNEL_BUILDER(                                   \
      Name("LRN").Device(DEVICE_GPU).TypeConstraint<T>("T"), \
      LRNOp<GPUDevice, T>);
TF_CALL_float(REGISTER_GPU);

#undef REGISTER_GPU

#endif  // GOOGLE_CUDA

#if !defined(IS_MOBILE_PLATFORM)

template <typename Device, typename T>
struct LaunchLRNGrad;

template <typename T>
struct LaunchLRNGrad<CPUDevice, T> {
  LaunchLRNGrad(int depth_radius, T bias, T alpha, T beta)
      : depth_radius_(depth_radius), bias_(bias), alpha_(alpha), beta_(beta) {}

  void launch(OpKernelContext* context, OpKernel* kernel,
              const Tensor& in_grads, const Tensor& in_image,
              const Tensor& out_image, Tensor* output) {
    const int64 batch = in_grads.dim_size(0);
    const int64 rows = in_grads.dim_size(1);
    const int64 cols = in_grads.dim_size(2);
    const int64 depth = in_grads.dim_size(3);
    const auto nodes = cols * rows;
    auto grads_shaped = in_grads.shaped<T, 2>({nodes * batch, depth});
    auto in_shaped = in_image.shaped<T, 2>({nodes * batch, depth});
    auto activations = out_image.shaped<T, 2>({nodes * batch, depth});

    auto out_shaped = output->shaped<T, 2>({nodes * batch, depth});
    out_shaped.setZero();

    auto shard = [this, activations, in_shaped, grads_shaped, out_shaped,
                  depth](int64 begin, int64 end) {
      for (int64 i = begin; i < end; ++i) {
        for (int64 j = 0; j < depth; ++j) {
          // Let y be the LRN activations and x be the inputs along the depth
          // dimension. (LRN operates independently along rows, cols, and
          // batch).
          // We have
          // yi = xi / (bias + alpha(sum_j_{i - depth_radius}^{i + depth_radius}
          //      x_j^2))^beta
          //
          // Let N = (bias + alpha(sum_j_{i - depth_radius}^{i + depth_radius}
          //           x_j^2))
          // dy_i/dx_i = (N^beta - xi. beta*N^(beta-1)*2*alpha*xi)/N^(2*beta)
          // dy_i/dx_j = (       - xi. beta*N^(beta-1)*2*alpha*xj)/N^(2*beta)
          //
          // NOTE(keveman) : We can compute N by doing (yi/xi) ^ (1/beta).
          // However, this is numerically unstable for small values of xi. We
          // compute N explicitly here to avoid that.

          int64 depth_begin = std::max<int64>(0, j - depth_radius_);
          int64 depth_end = std::min<int64>(depth, j + depth_radius_ + 1);

          T norm(0);
          for (int64 k = depth_begin; k < depth_end; ++k) {
            norm += in_shaped(i, k) * in_shaped(i, k);
          }
          norm = alpha_ * norm + bias_;
          DCHECK_GT(norm, T(1e-6));
          for (int64 k = depth_begin; k < depth_end; ++k) {
            T dyi = T(-2) * alpha_ * beta_ * in_shaped(i, k) *
                    activations(i, j) / norm;
            if (k == j) {
              dyi += Eigen::numext::pow(norm, -beta_);
            }
            dyi *= grads_shaped(i, j);
            const_cast<typename TTypes<T, 2>::Tensor&>(out_shaped)(i, k) += dyi;
          }
        }
      }
    };
    auto worker_threads = *(context->device()->tensorflow_cpu_worker_threads());
    Shard(worker_threads.num_threads, worker_threads.workers, nodes * batch,
          depth * depth, shard);
  }

  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#if GOOGLE_CUDA

template <typename T>
struct LaunchLRNGrad<GPUDevice, T> {
  LaunchLRNGrad(int depth_radius, T bias, T alpha, T beta)
      : depth_radius_(depth_radius), bias_(bias), alpha_(alpha), beta_(beta) {}

  void launch(OpKernelContext* context, OpKernel* kernel,
              const Tensor& in_grads, const Tensor& in_image,
              const Tensor& out_image, Tensor* output) {
    OP_REQUIRES(
        context, beta_ >= 0.01,
        errors::InvalidArgument("cuDNN requires beta >= 0.01, got: ", beta_));

    OP_REQUIRES(
        context, depth_radius_ > 0 && depth_radius_ <= 7,
        errors::InvalidArgument("cuDNN requires depth_radius in [1, 7], got: ",
                                depth_radius_));
    OP_REQUIRES(
        context, bias_ >= 1e-5,
        errors::InvalidArgument("cuDNN requires bias >= 1e-5, got: ", bias_));

    const int64 batch = in_grads.dim_size(0);
    const int64 rows = in_grads.dim_size(1);
    const int64 cols = in_grads.dim_size(2);
    const int64 depth = in_grads.dim_size(3);

    perftools::gputools::dnn::BatchDescriptor dimensions_desc;
    dimensions_desc.set_count(batch)
        .set_height(rows)
        .set_width(cols)
        .set_feature_map_count(depth)
        .set_layout(perftools::gputools::dnn::DataLayout::kBatchYXDepth);

    perftools::gputools::dnn::NormalizeDescriptor normalize_desc;
    normalize_desc.set_bias(bias_)
        .set_range(depth_radius_)
        .set_alpha(alpha_)
        .set_beta(beta_);

    auto input_grads_data = StreamExecutorUtil::AsDeviceMemory<T>(in_grads);
    auto input_image_data = StreamExecutorUtil::AsDeviceMemory<T>(in_image);
    auto output_image_data = StreamExecutorUtil::AsDeviceMemory<T>(out_image);
    auto output_grads_data = StreamExecutorUtil::AsDeviceMemory<T>(*output);

    auto* stream = context->op_device_context()->stream();
    OP_REQUIRES(context, stream, errors::Internal("No GPU stream available."));

    bool status =
        stream
            ->ThenNormalizeBackwardWithDimensions(
                normalize_desc, dimensions_desc, input_image_data,
                output_image_data, input_grads_data, &output_grads_data)
            .ok();
    OP_REQUIRES(
        context, status,
        errors::Internal("NormalizeBackwardWithDimensions launch failed"));
  }

  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#endif  // GOOGLE_CUDA

template <typename Device, typename T>
class LRNGradOp : public OpKernel {
 public:
  explicit LRNGradOp(OpKernelConstruction* context) : OpKernel(context) {
    int64 depth_radius64;
    OP_REQUIRES_OK(context, context->GetAttr("depth_radius", &depth_radius64));
    OP_REQUIRES(context, FastBoundsCheck(depth_radius64,
                                         std::numeric_limits<int>::max()),
                errors::InvalidArgument("depth_radius = ", depth_radius64,
                                        " larger than int max"));
    depth_radius_ = static_cast<int>(depth_radius64);
    float tmp;
    OP_REQUIRES_OK(context, context->GetAttr("bias", &tmp));
    bias_ = T(tmp);
    OP_REQUIRES_OK(context, context->GetAttr("alpha", &tmp));
    alpha_ = T(tmp);
    OP_REQUIRES_OK(context, context->GetAttr("beta", &tmp));
    beta_ = T(tmp);
  }

  void Compute(OpKernelContext* context) override {
    const Tensor& in_grads = context->input(0);
    const Tensor& in_image = context->input(1);
    const Tensor& out_image = context->input(2);

    OP_REQUIRES(context, in_grads.dims() == 4 && in_image.dims() == 4,
                errors::InvalidArgument("inputs must be 4-dimensional"));
    const int64 batch = in_grads.dim_size(0);
    const int64 rows = in_grads.dim_size(1);
    const int64 cols = in_grads.dim_size(2);
    const int64 depth = in_grads.dim_size(3);
    OP_REQUIRES(
        context,
        in_image.dim_size(0) == batch && in_image.dim_size(1) == rows &&
            in_image.dim_size(2) == cols && in_image.dim_size(3) == depth &&
            out_image.dim_size(0) == batch && out_image.dim_size(1) == rows &&
            out_image.dim_size(2) == cols && out_image.dim_size(3) == depth,
        errors::InvalidArgument(
            "input_grads, input_image, and out_image should have the same "
            "shape"));

    Tensor* output = nullptr;
    OP_REQUIRES_OK(context,
                   context->allocate_output(
                       0, TensorShape({batch, rows, cols, depth}), &output));

    LaunchLRNGrad<Device, T> launcher(depth_radius_, bias_, alpha_, beta_);
    launcher.launch(context, this, in_grads, in_image, out_image, output);
  }

 private:
  int depth_radius_;
  T bias_;
  T alpha_;
  T beta_;
};

#define REGISTER_CPU(T)                                          \
  REGISTER_KERNEL_BUILDER(                                       \
      Name("LRNGrad").Device(DEVICE_CPU).TypeConstraint<T>("T"), \
      LRNGradOp<CPUDevice, T>);
TF_CALL_float(REGISTER_CPU);
TF_CALL_half(REGISTER_CPU);

#undef REGISTER_CPU

#if GOOGLE_CUDA

#define REGISTER_GPU(T)                                          \
  REGISTER_KERNEL_BUILDER(                                       \
      Name("LRNGrad").Device(DEVICE_GPU).TypeConstraint<T>("T"), \
      LRNGradOp<GPUDevice, T>);
TF_CALL_float(REGISTER_GPU);

#undef REGISTER_GPU

#endif  // GOOGLE_CUDA

#endif  // !defined(IS_MOBILE_PLATFORM)

}  // namespace tensorflow
