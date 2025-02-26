# Copyright 2016 The TensorFlow Authors. All Rights Reserved.
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

"""TensorFlow composable models used as building blocks for estimators."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import math
import re

import six

from tensorflow.contrib import layers
from tensorflow.contrib.layers.python.layers import feature_column_ops
from tensorflow.contrib.learn.python.learn.utils import checkpoints
from tensorflow.python.framework import ops
from tensorflow.python.ops import clip_ops
from tensorflow.python.ops import gradients
from tensorflow.python.ops import logging_ops
from tensorflow.python.ops import nn
from tensorflow.python.ops import partitioned_variables
from tensorflow.python.ops import variable_scope


class _ComposableModel(object):
  """ABC for building blocks that can be used to create estimators.

  Subclasses need to implement the following methods:
    - build_model
    - _get_optimizer
  See below for the required signatures.
  _ComposableModel and its subclasses are not part of the public tf.learn API.
  """

  def __init__(self,
               num_label_columns,
               optimizer,
               gradient_clip_norm,
               num_ps_replicas,
               scope):
    """Common initialization for all _ComposableModel objects.

    Args:
      num_label_columns: The number of label/target columns.
      optimizer: An instance of `tf.Optimizer` used to apply gradients to
        the model. If `None`, will use a FTRL optimizer.
      gradient_clip_norm: A float > 0. If provided, gradients are clipped
        to their global norm with this clipping ratio. See
        tf.clip_by_global_norm for more details.
      num_ps_replicas: The number of parameter server replicas.
      scope: Scope for variables created in this model.
    """
    self._num_label_columns = num_label_columns
    self._optimizer = optimizer
    self._gradient_clip_norm = gradient_clip_norm
    self._num_ps_replicas = num_ps_replicas
    self._scope = scope
    self._feature_columns = None

  def get_scope_name(self):
    """Returns the scope name used by this model for variables."""
    return self._scope

  def build_model(self, features, feature_columns, is_training):
    """Builds the model that can calculate the logits.

    Args:
      features: A mapping from feature columns to tensors.
      feature_columns: An iterable containing all the feature columns used
        by the model. All items in the set should be instances of
        classes derived from `FeatureColumn`.
      is_training: Set to True when training, False otherwise.

    Returns:
      The logits for this model.
    """
    raise NotImplementedError

  def get_train_step(self, loss):
    """Returns the ops to run to perform a training step on this estimator.

    Args:
      loss: The loss to use when calculating gradients.

    Returns:
      The ops to run to perform a training step.
    """
    my_vars = self._get_vars()
    if not (self._get_feature_columns() or my_vars):
      return []

    grads = gradients.gradients(loss, my_vars)
    if self._gradient_clip_norm:
      grads, _ = clip_ops.clip_by_global_norm(grads, self._gradient_clip_norm)
    return [self._get_optimizer().apply_gradients(zip(grads, my_vars))]

  def _get_feature_columns(self):
    if not self._feature_columns:
      return None
    feature_column_ops.check_feature_columns(self._feature_columns)
    return sorted(set(self._feature_columns), key=lambda x: x.key)

  def _get_vars(self):
    if self._get_feature_columns():
      return ops.get_collection(self._scope)
    return []

  def _get_optimizer(self):
    if (self._optimizer is None or isinstance(self._optimizer,
                                              six.string_types)):
      optimizer = self._get_default_optimizer(self._optimizer)
    elif callable(self._optimizer):
      optimizer = self._optimizer()
    else:
      optimizer = self._optimizer
    return optimizer

  def _get_default_optimizer(self, optimizer_name=None):
    raise NotImplementedError


class LinearComposableModel(_ComposableModel):
  """A _ComposableModel that implements linear regression.

  Instances of this class can be used to build estimators through the use
  of composition.
  """

  def __init__(self,
               num_label_columns,
               optimizer=None,
               gradient_clip_norm=None,
               num_ps_replicas=0,
               scope=None):
    """Initializes LinearComposableModel objects.

    Args:
      num_label_columns: The number of label/target columns.
      optimizer: An instance of `tf.Optimizer` used to apply gradients to
        the model. If `None`, will use a FTRL optimizer.
      gradient_clip_norm: A float > 0. If provided, gradients are clipped
        to their global norm with this clipping ratio. See
        tf.clip_by_global_norm for more details.
      num_ps_replicas: The number of parameter server replicas.
      scope: Optional scope for variables created in this model. If scope
        is not supplied, it will default to 'linear'.
    """
    scope = "linear" if not scope else scope
    super(LinearComposableModel, self).__init__(
        num_label_columns=num_label_columns,
        optimizer=optimizer,
        gradient_clip_norm=gradient_clip_norm,
        num_ps_replicas=num_ps_replicas,
        scope=scope)

  def get_weights(self, model_dir):
    """Returns weights per feature of the linear part.

    Args:
      model_dir: Directory where model parameters, graph and etc. are saved.

    Returns:
      The weights created by this model (without the optimizer weights).
    """
    all_variables = [name for name, _ in checkpoints.list_variables(model_dir)]
    values = {}
    optimizer_regex = r".*/" + self._get_optimizer().get_name() + r"(_\d)?$"
    for name in all_variables:
      if (name.startswith(self._scope + "/") and
          name != self._scope + "/bias_weight" and
          not re.match(optimizer_regex, name)):
        values[name] = checkpoints.load_variable(model_dir, name)
    if len(values) == 1:
      return values[list(values.keys())[0]]
    return values

  def get_bias(self, model_dir):
    """Returns bias of the model.

    Args:
      model_dir: Directory where model parameters, graph and etc. are saved.

    Returns:
      The bias weights created by this model.
    """
    return checkpoints.load_variable(model_dir,
                                     name=(self._scope+"/bias_weight"))

  def build_model(self, features, feature_columns, is_training):
    """See base class."""
    self._feature_columns = feature_columns
    partitioner = partitioned_variables.min_max_variable_partitioner(
        max_partitions=self._num_ps_replicas,
        min_slice_size=64 << 20)
    with variable_scope.variable_op_scope(
        features.values(), self._scope, partitioner=partitioner) as scope:
      logits, _, _ = layers.weighted_sum_from_feature_columns(
          columns_to_tensors=features,
          feature_columns=self._get_feature_columns(),
          num_outputs=self._num_label_columns,
          weight_collections=[self._scope],
          scope=scope)
    return logits

  def _get_default_optimizer(self, optimizer_name=None):
    if optimizer_name is None:
      optimizer_name = "Ftrl"
    default_learning_rate = 1. / math.sqrt(len(self._get_feature_columns()))
    default_learning_rate = min(0.2, default_learning_rate)
    return layers.OPTIMIZER_CLS_NAMES[optimizer_name](
        learning_rate=default_learning_rate)


class DNNComposableModel(_ComposableModel):
  """A _ComposableModel that implements a DNN.

  Instances of this class can be used to build estimators through the use
  of composition.
  """

  def __init__(self,
               num_label_columns,
               hidden_units,
               optimizer=None,
               activation_fn=nn.relu,
               dropout=None,
               gradient_clip_norm=None,
               num_ps_replicas=0,
               scope=None):
    """Initializes DNNComposableModel objects.

    Args:
      num_label_columns: The number of label/target columns.
      hidden_units: List of hidden units per layer. All layers are fully
        connected.
      optimizer: An instance of `tf.Optimizer` used to apply gradients to
        the model. If `None`, will use a FTRL optimizer.
      activation_fn: Activation function applied to each layer. If `None`,
        will use `tf.nn.relu`.
      dropout: When not None, the probability we will drop out
        a given coordinate.
      gradient_clip_norm: A float > 0. If provided, gradients are clipped
        to their global norm with this clipping ratio. See
        tf.clip_by_global_norm for more details.
      num_ps_replicas: The number of parameter server replicas.
      scope: Optional scope for variables created in this model. If not scope
        is supplied, one is generated.
    """
    scope = "dnn" if not scope else scope
    super(DNNComposableModel, self).__init__(
        num_label_columns=num_label_columns,
        optimizer=optimizer,
        gradient_clip_norm=gradient_clip_norm,
        num_ps_replicas=num_ps_replicas,
        scope=scope)
    self._hidden_units = hidden_units
    self._activation_fn = activation_fn
    self._dropout = dropout

  def get_weights(self, model_dir):
    """Returns the weights of the model.

    Args:
      model_dir: Directory where model parameters, graph and etc. are saved.

    Returns:
      The weights created by this model.
    """
    return [checkpoints.load_variable(
        model_dir, name=(self._scope+"/hiddenlayer_%d/weights" % i))
            for i, _ in enumerate(self._hidden_units)] + [
                checkpoints.load_variable(
                    model_dir, name=(self._scope+"/logits/weights"))]

  def get_bias(self, model_dir):
    """Returns the bias of the model.

    Args:
      model_dir: Directory where model parameters, graph and etc. are saved.

    Returns:
      The bias weights created by this model.
    """
    return [checkpoints.load_variable(
        model_dir, name=(self._scope+"/hiddenlayer_%d/biases" % i))
            for i, _ in enumerate(self._hidden_units)] + [
                checkpoints.load_variable(
                    model_dir, name=(self._scope+"/logits/biases"))]

  def _add_hidden_layer_summary(self, value, tag):
    # TODO(zakaria): Move this code to tf.learn and add test.
    logging_ops.scalar_summary("%s:fraction_of_zero_values" % tag,
                               nn.zero_fraction(value))
    logging_ops.histogram_summary("%s:activation" % tag, value)

  def build_model(self, features, feature_columns, is_training):
    """See base class."""
    self._feature_columns = feature_columns

    input_layer_partitioner = (
        partitioned_variables.min_max_variable_partitioner(
            max_partitions=self._num_ps_replicas,
            min_slice_size=64 << 20))
    with variable_scope.variable_op_scope(
        features.values(),
        self._scope + "/input_from_feature_columns",
        partitioner=input_layer_partitioner) as scope:
      net = layers.input_from_feature_columns(
          features,
          self._get_feature_columns(),
          weight_collections=[self._scope],
          scope=scope)

    hidden_layer_partitioner = (
        partitioned_variables.min_max_variable_partitioner(
            max_partitions=self._num_ps_replicas))
    for layer_id, num_hidden_units in enumerate(self._hidden_units):
      with variable_scope.variable_op_scope(
          [net], self._scope + "/hiddenlayer_%d" % layer_id,
          partitioner=hidden_layer_partitioner) as scope:
        net = layers.fully_connected(
            net,
            num_hidden_units,
            activation_fn=self._activation_fn,
            variables_collections=[self._scope],
            scope=scope)
        if self._dropout is not None and is_training:
          net = layers.dropout(
              net,
              keep_prob=(1.0 - self._dropout))
      self._add_hidden_layer_summary(net, scope.name)

    with variable_scope.variable_op_scope(
        [net], self._scope + "/logits",
        partitioner=hidden_layer_partitioner) as scope:
      logits = layers.fully_connected(
          net,
          self._num_label_columns,
          activation_fn=None,
          variables_collections=[self._scope],
          scope=scope)
    self._add_hidden_layer_summary(logits, "logits")
    return logits

  def _get_default_optimizer(self, optimizer_name=None):
    if optimizer_name is None:
      optimizer_name = "Adagrad"
    return layers.OPTIMIZER_CLS_NAMES[optimizer_name](learning_rate=0.05)
