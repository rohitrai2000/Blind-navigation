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

#include "tensorflow/core/util/bcast.h"
#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/platform/macros.h"
#include "tensorflow/core/platform/types.h"

namespace tensorflow {

// Given shapes of two tensors, computes the reduction indices for the
// gradient computation.
//
// TODO(zhifengc):
//   1. Adds support for n-ary (n >= 2).
class BCastGradArgsOp : public OpKernel {
 public:
  explicit BCastGradArgsOp(OpKernelConstruction* ctx) : OpKernel(ctx) {
    OP_REQUIRES_OK(
        ctx, ctx->MatchSignature({DT_INT32, DT_INT32}, {DT_INT32, DT_INT32}));
  }

  void Compute(OpKernelContext* ctx) override {
    OP_REQUIRES(
        ctx, ctx->num_inputs() == 2,
        errors::Unimplemented("Broadcast for n-ary operations (n > 2)"));
    gtl::InlinedVector<BCast::Vec, 4> shapes;
    for (int i = 0; i < ctx->num_inputs(); ++i) {
      const Tensor& in = ctx->input(i);
      OP_REQUIRES(ctx, TensorShapeUtils::IsVector(in.shape()),
                  errors::InvalidArgument("In[", i, "] must be a vector.",
                                          in.shape().DebugString()));
      BCast::Vec vec;
      for (int64 i = 0; i < in.NumElements(); ++i) {
        vec.push_back(in.vec<int32>()(i));
      }
      shapes.push_back(vec);
    }
    BCast bcast(shapes[0], shapes[1]);
    OP_REQUIRES(ctx, bcast.IsValid(),
                errors::InvalidArgument(
                    "Incompatible shapes: [", str_util::Join(shapes[0], ","),
                    "] vs. [", str_util::Join(shapes[1], ","), "]"));
    Output(ctx, 0, bcast.grad_x_reduce_idx());
    Output(ctx, 1, bcast.grad_y_reduce_idx());
  }

  bool IsExpensive() override { return false; }

 private:
  void Output(OpKernelContext* ctx, int idx, const BCast::Vec& v) {
    const int64 len = v.size();
    Tensor* o = nullptr;
    OP_REQUIRES_OK(ctx, ctx->allocate_output(idx, TensorShape({len}), &o));
    for (int64 i = 0; i < len; ++i) {
      o->flat<int32>()(i) = static_cast<int32>(v[i]);
    }
  }

  TF_DISALLOW_COPY_AND_ASSIGN(BCastGradArgsOp);
};

REGISTER_KERNEL_BUILDER(Name("BroadcastGradientArgs")
                            .Device(DEVICE_CPU)
                            .HostMemory("s0")
                            .HostMemory("s1")
                            .HostMemory("r0")
                            .HostMemory("r1"),
                        BCastGradArgsOp);
REGISTER_KERNEL_BUILDER(Name("BroadcastGradientArgs")
                            .Device(DEVICE_GPU)
                            .HostMemory("s0")
                            .HostMemory("s1")
                            .HostMemory("r0")
                            .HostMemory("r1"),
                        BCastGradArgsOp);

}  // end namespace tensorflow
