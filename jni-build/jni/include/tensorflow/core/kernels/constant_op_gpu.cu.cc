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

#if GOOGLE_CUDA

#define EIGEN_USE_GPU

#include "tensorflow/core/framework/register_types.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/kernels/fill_functor.h"
#include "tensorflow/core/platform/types.h"

namespace Eigen {
namespace internal {

template <typename T>
struct scalar_const_op {
  typedef typename packet_traits<T>::type Packet;

  const T* val;

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE
  scalar_const_op(const scalar_const_op& x)
      : val(x.val) {}

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE scalar_const_op(const T* v) : val(v) {}

  template <typename Index>
  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const T operator()(Index,
                                                           Index = 0) const {
    return *val;
  }

  template <typename Index, typename PacketType = Packet>
  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE const PacketType
      packetOp(Index, Index = 0) const {
    return internal::pset1<PacketType>(*val);
  }
};

template <typename T>
struct functor_traits<scalar_const_op<T> > {
  enum {
    Cost = 1,
    PacketAccess = packet_traits<T>::Vectorizable,
    IsRepeatable = true
  };
};

}  // end namespace internal
}  // end namespace Eigen

namespace tensorflow {

namespace functor {

typedef Eigen::GpuDevice GPUDevice;

// Partial specialization FillFunctor<Device=GPUDevice, T>
template <typename T>
struct FillFunctor<GPUDevice, T> {
  void operator()(const GPUDevice& d, typename TTypes<T>::Flat out,
                  typename TTypes<T>::ConstScalar in) {
    Eigen::internal::scalar_const_op<T> f(in.data());
    To32Bit(out).device(d) = To32Bit(out).nullaryExpr(f);
  }
};

#define DEFINE_FILL_GPU(T) template struct FillFunctor<GPUDevice, T>;
TF_CALL_REAL_NUMBER_TYPES(DEFINE_FILL_GPU);
DEFINE_FILL_GPU(bool);
#undef DEFINE_FILL_GPU

// Partial specialization of FillFunctor<Device=GPUDevice, T>.
template <typename T>
struct SetZeroFunctor<GPUDevice, T> {
  void operator()(const GPUDevice& d, typename TTypes<T>::Flat out) {
    To32Bit(out).device(d) = To32Bit(out).constant(T(0));
  }
};

#define DEFINE_SETZERO_GPU(T) template struct SetZeroFunctor<GPUDevice, T>
DEFINE_SETZERO_GPU(Eigen::half);
DEFINE_SETZERO_GPU(float);
DEFINE_SETZERO_GPU(double);
DEFINE_SETZERO_GPU(complex64);
DEFINE_SETZERO_GPU(complex128);
#undef DEFINE_SETZERO_GPU

}  // end namespace functor
}  // end namespace tensorflow

#endif  // GOOGLE_CUDA
