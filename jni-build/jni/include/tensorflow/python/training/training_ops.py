# Copyright 2015 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

"""Python wrappers for training ops."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.python.framework import ops
from tensorflow.python.framework import tensor_shape
from tensorflow.python.training import gen_training_ops
# go/tf-wildcard-import
# pylint: disable=wildcard-import
from tensorflow.python.training.gen_training_ops import *
# pylint: enable=wildcard-import


# Shape functions for fused training ops
# --------------------------------------
#
# The fused training ops all have the same basic structure: they take
# one or more variables with the same shape, and emit a reference to
# the original variable (which has the same shape as the first
# input). In addition, they take one or more scalar tensors containing
# hyperparameters.
#
# The sparse ops take the gradients as a Python IndexedSlices, which
# means that the indices are a vector of length N, and the gradient
# values are a tensor whose size is the same as the original variable,
# except for the 0th dimension, which has size N.


def _AssertInputIsScalar(op, index):
  """Raises ValueError if `op.inputs[index]` is not scalar."""
  op.inputs[index].get_shape().assert_is_compatible_with(tensor_shape.scalar())


@ops.RegisterShape("ApplyAdadelta")
def _ApplyAdadeltaShape(op):
  """Shape function for the ApplyAdadelta op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  accum_update_shape = op.inputs[2].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 3)  # lr
  _AssertInputIsScalar(op, 4)  # rho
  _AssertInputIsScalar(op, 5)  # epsilon
  grad_shape = op.inputs[6].get_shape().merge_with(accum_shape)
  return [grad_shape]

@ops.RegisterShape("ApplyAdagrad")
def _ApplyAdagradShape(op):
  """Shape function for the ApplyAdagrad op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  grad_shape = op.inputs[3].get_shape().merge_with(accum_shape)
  return [grad_shape]

@ops.RegisterShape("ApplyProximalAdagrad")
def _ApplyProximalAdagradShape(op):
  """Shape function for the ApplyProximalAdagrad op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  _AssertInputIsScalar(op, 3)  # l1
  _AssertInputIsScalar(op, 4)  # l2
  grad_shape = op.inputs[5].get_shape().merge_with(accum_shape)
  return [grad_shape]


@ops.RegisterShape("ApplyFtrl")
def _ApplyFtrlShape(op):
  """Shape function for the ApplyFtrlOp op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  linear_shape = op.inputs[2].get_shape().merge_with(accum_shape)
  grad_shape = op.inputs[3].get_shape().merge_with(linear_shape)
  _AssertInputIsScalar(op, 4)  # lr
  _AssertInputIsScalar(op, 5)  # l1
  _AssertInputIsScalar(op, 6)  # l2
  _AssertInputIsScalar(op, 7)  # lr_power
  return [grad_shape]


@ops.RegisterShape("ApplyAdam")
def _ApplyAdamShape(op):
  """Shape function for the ApplyAdam op."""
  var_shape = op.inputs[0].get_shape()
  m_shape = op.inputs[1].get_shape().merge_with(var_shape)
  v_shape = op.inputs[2].get_shape().merge_with(m_shape)
  _AssertInputIsScalar(op, 3)  # beta1_power
  _AssertInputIsScalar(op, 4)  # beta2_power
  _AssertInputIsScalar(op, 5)  # lr
  _AssertInputIsScalar(op, 6)  # beta1
  _AssertInputIsScalar(op, 7)  # beta2
  _AssertInputIsScalar(op, 8)  # epsilon
  grad_shape = op.inputs[9].get_shape().merge_with(v_shape)
  return [grad_shape]


@ops.RegisterShape("ApplyMomentum")
def _ApplyMomentumShape(op):
  """Shape function for the ApplyMomentum op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  grad_shape = op.inputs[3].get_shape().merge_with(accum_shape)
  _AssertInputIsScalar(op, 4)  # momentum
  return [grad_shape]


@ops.RegisterShape("ApplyRMSProp")
def _ApplyRMSPropShape(op):
  """Shape function for the ApplyRMSProp op."""
  var_shape = op.inputs[0].get_shape()
  ms_shape = op.inputs[1].get_shape().merge_with(var_shape)
  mom_shape = op.inputs[2].get_shape().merge_with(ms_shape)
  _AssertInputIsScalar(op, 3)  # lr
  _AssertInputIsScalar(op, 4)  # rho
  _AssertInputIsScalar(op, 5)  # momentum
  _AssertInputIsScalar(op, 6)  # epsilon
  grad_shape = op.inputs[7].get_shape().merge_with(mom_shape)
  return [grad_shape]


@ops.RegisterShape("ApplyGradientDescent")
def _ApplyGradientDescentShape(op):
  """Shape function for the ApplyGradientDescent op."""
  var_shape = op.inputs[0].get_shape()
  _AssertInputIsScalar(op, 1)  # alpha
  delta_shape = op.inputs[2].get_shape().merge_with(var_shape)
  return [delta_shape]


@ops.RegisterShape("ApplyProximalGradientDescent")
def _ApplyProximalGradientDescentShape(op):
  """Shape function for the ApplyProximalGradientDescent op."""
  var_shape = op.inputs[0].get_shape()
  _AssertInputIsScalar(op, 1)  # alpha
  _AssertInputIsScalar(op, 2)  # l1
  _AssertInputIsScalar(op, 3)  # l2
  delta_shape = op.inputs[4].get_shape().merge_with(var_shape)
  return [delta_shape]


@ops.RegisterShape("SparseApplyProximalGradientDescent")
def _SparseApplyProximalGradientDescentShape(op):
  """Shape function for the SparseApplyGradientDescent op."""
  var_shape = op.inputs[0].get_shape()
  _AssertInputIsScalar(op, 1)  # lr
  _AssertInputIsScalar(op, 2)  # l1
  _AssertInputIsScalar(op, 3)  # l2
  grad_shape = op.inputs[4].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(var_shape[1:]))
  unused_indices_shape = op.inputs[5].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  return [var_shape]


@ops.RegisterShape("SparseApplyRMSProp")
def _SparseApplyRMSPropShape(op):
  """Shape function for the SparseApplyRMSProp op."""
  var_shape = op.inputs[0].get_shape()
  ms_shape = op.inputs[1].get_shape().merge_with(var_shape)
  mom_shape = op.inputs[2].get_shape().merge_with(ms_shape)
  _AssertInputIsScalar(op, 3)  # lr
  _AssertInputIsScalar(op, 4)  # rho
  _AssertInputIsScalar(op, 5)  # momentum
  _AssertInputIsScalar(op, 6)  # epsilon
  grad_shape = op.inputs[7].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(mom_shape[1:]))
  unused_indices_shape = op.inputs[8].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  return [mom_shape]


@ops.RegisterShape("SparseApplyAdadelta")
def _SparseApplyAdadeltaShape(op):
   """Shape function for the SparseApplyAdadelta op."""
   var_shape = op.inputs[0].get_shape()
   accum_grad_shape = op.inputs[1].get_shape().merge_with(var_shape)
   accum_update_shape = op.inputs[2].get_shape().merge_with(accum_grad_shape)
   _AssertInputIsScalar(op, 3)  # lr
   _AssertInputIsScalar(op, 4)  # decay_rate
   _AssertInputIsScalar(op, 5)  # epsilon
   grad_shape = op.inputs[6].get_shape().merge_with(
       tensor_shape.TensorShape([None]).concatenate(accum_update_shape[1:]))
   unused_indices_shape = op.inputs[7].get_shape().merge_with(
       tensor_shape.vector(grad_shape[0]))
   return [accum_update_shape]


@ops.RegisterShape("SparseApplyAdagrad")
def _SparseApplyAdagradShape(op):
  """Shape function for the SparseApplyAdagrad op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  grad_shape = op.inputs[3].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(accum_shape[1:]))
  unused_indices_shape = op.inputs[4].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  return [accum_shape]


@ops.RegisterShape("SparseApplyProximalAdagrad")
def _SparseApplyProximalAdagradShape(op):
  """Shape function for the SparseApplyProximalAdagrad op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  _AssertInputIsScalar(op, 3)  # l1
  _AssertInputIsScalar(op, 4)  # l2
  grad_shape = op.inputs[5].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(accum_shape[1:]))
  unused_indices_shape = op.inputs[6].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  return [accum_shape]


@ops.RegisterShape("SparseApplyFtrl")
def _SparseApplyFtrlShape(op):
  """Shape function for the SparseApplyFtrl op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  linear_shape = op.inputs[2].get_shape().merge_with(accum_shape)
  grad_shape = op.inputs[3].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(linear_shape[1:]))
  unused_indices_shape = op.inputs[4].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  _AssertInputIsScalar(op, 5)  # lr
  _AssertInputIsScalar(op, 6)  # l1
  _AssertInputIsScalar(op, 7)  # l2
  _AssertInputIsScalar(op, 8)  # lr_power
  return [linear_shape]


@ops.RegisterShape("SparseApplyMomentum")
def _SparseApplyMomentumShape(op):
  """Shape function for the SparseApplyMomentum op."""
  var_shape = op.inputs[0].get_shape()
  accum_shape = op.inputs[1].get_shape().merge_with(var_shape)
  _AssertInputIsScalar(op, 2)  # lr
  grad_shape = op.inputs[3].get_shape().merge_with(
      tensor_shape.TensorShape([None]).concatenate(accum_shape[1:]))
  unused_indices_shape = op.inputs[4].get_shape().merge_with(
      tensor_shape.vector(grad_shape[0]))
  _AssertInputIsScalar(op, 5)  # momentum
  return [accum_shape]
