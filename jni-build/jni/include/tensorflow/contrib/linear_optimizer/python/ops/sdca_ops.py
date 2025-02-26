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
"""Proximal stochastic dual coordinate ascent optimizer for linear models."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import collections

from six.moves import range  # pylint: disable=redefined-builtin

from tensorflow.contrib.linear_optimizer.ops import gen_sdca_ops
from tensorflow.contrib.lookup import lookup_ops
from tensorflow.python.framework import dtypes
from tensorflow.python.framework import ops
from tensorflow.python.framework.load_library import load_op_library
from tensorflow.python.framework.ops import convert_to_tensor
from tensorflow.python.framework.ops import name_scope
from tensorflow.python.framework.ops import op_scope
from tensorflow.python.ops import array_ops
from tensorflow.python.ops import control_flow_ops
from tensorflow.python.ops import data_flow_ops
from tensorflow.python.ops import math_ops
from tensorflow.python.ops import nn_ops
from tensorflow.python.ops import state_ops
from tensorflow.python.ops import string_ops
from tensorflow.python.ops import variables as var_ops
from tensorflow.python.ops.nn import sigmoid_cross_entropy_with_logits
from tensorflow.python.platform import resource_loader

__all__ = ['SdcaModel']

_sdca_ops = load_op_library(resource_loader.get_path_to_datafile(
    '_sdca_ops.so'))
assert _sdca_ops, 'Could not load _sdca_ops.so'


class _ShardedMutableHashTable(lookup_ops.LookupInterface):
  """A sharded version of MutableHashTable.

  It is designed to be interface compatible with LookupInterface and
  MutableHashTable, with the exception of the export method, which is replaced
  by a custom values_reduce_sum method for SDCA needs. The class is not part of
  lookup ops because it is unclear how to make the device placement general
  enough to be useful.

  The _ShardedHashTable keeps `num_shards` MutableHashTables internally. If keys
  are integers, the shard is computed via the modulo operation. If keys are
  strings, the shard is computed via string_to_hash_bucket_fast.
  """

  # TODO(andreasst): consider moving this to lookup_ops

  def __init__(self,
               key_dtype,
               value_dtype,
               default_value,
               num_shards=1,
               name=None):
    with ops.op_scope([], name, 'sharded_mutable_hash_table') as scope:
      super(_ShardedMutableHashTable, self).__init__(key_dtype, value_dtype,
                                                     scope)
      table_shards = []
      for _ in range(num_shards):
        # TODO(andreasst): add placement hints once bug 30002625 is fixed.
        table_shards.append(lookup_ops.MutableHashTable(
            key_dtype=key_dtype,
            value_dtype=value_dtype,
            default_value=default_value,
            name=name))
      self._table_shards = table_shards
      # TODO(andreasst): add a value_shape() method to LookupInterface
      # pylint: disable=protected-access
      self._value_shape = self._table_shards[0]._value_shape
      # pylint: enable=protected-access

  @property
  def _num_shards(self):
    return len(self._table_shards)

  @property
  def table_shards(self):
    return self._table_shards

  def size(self, name=None):
    with ops.op_scope([], name, 'sharded_mutable_hash_table_size'):
      sizes = [
          self._table_shards[i].size() for i in range(self._num_shards)
      ]
      return math_ops.add_n(sizes)

  def _shard_indices(self, keys):
    if self._key_dtype == dtypes.string:
      indices = string_ops.string_to_hash_bucket_fast(keys, self._num_shards)
    else:
      indices = math_ops.mod(keys, self._num_shards)
    return math_ops.cast(indices, dtypes.int32)

  def lookup(self, keys, name=None):
    if keys.dtype != self._key_dtype:
      raise TypeError('Signature mismatch. Keys must be dtype %s, got %s.' %
                      (self._key_dtype, keys.dtype))
    num_shards = self._num_shards
    if num_shards == 1:
      return self._table_shards[0].lookup(keys, name=name)

    shard_indices = self._shard_indices(keys)
    # TODO(andreasst): support 'keys' that are not vectors
    key_shards = data_flow_ops.dynamic_partition(keys, shard_indices,
                                                 num_shards)
    value_shards = [
        self._table_shards[i].lookup(key_shards[i], name=name)
        for i in range(num_shards)
    ]

    original_indices = math_ops.range(array_ops.size(keys))
    partitioned_indices = data_flow_ops.dynamic_partition(original_indices,
                                                          shard_indices,
                                                          num_shards)
    result = data_flow_ops.dynamic_stitch(partitioned_indices, value_shards)
    result.set_shape(keys.get_shape().concatenate(self._value_shape))
    return result

  def insert(self, keys, values, name=None):
    num_shards = self._num_shards
    if num_shards == 1:
      return self._table_shards[0].insert(keys, values, name=name)

    shard_indices = self._shard_indices(keys)
    # TODO(andreasst): support 'keys' that are not vectors
    key_shards = data_flow_ops.dynamic_partition(keys, shard_indices,
                                                 num_shards)
    value_shards = data_flow_ops.dynamic_partition(values, shard_indices,
                                                   num_shards)
    return_values = [
        self._table_shards[i].insert(key_shards[i], value_shards[i], name=name)
        for i in range(num_shards)
    ]

    return control_flow_ops.group(*return_values)

  def values_reduce_sum(self, name=None):
    """Computes reduce_sum reducing dimension 0 across all values in all shards.

    Args:
      name: A name for the operation (optional).

    Returns:
      A tensor with the sum across all values in the same shape as the table's
      value shape.
    """
    # TODO(andreasst): consider replacing with something like export_sharded
    # and doing the sum in SdcaModel.
    sums = []
    for table_shard in self._table_shards:
      _, exported_values = table_shard.export(name=name)
      sums.append(math_ops.reduce_sum(exported_values, 0))
    return math_ops.add_n(sums)


class SparseFeatureColumn(object):
  """Represents a sparse feature column.

  Contains three tensors representing a sparse feature column, they are
  example indices (int64), feature indices (int64), and feature values (float).
  Feature weights are optional, and are treated as 1.0f if missing.

  For example, consider a batch of 4 examples, which contains the following
  features in a particular SparseFeatureColumn:
   Example 0: feature 5, value 1
   Example 1: feature 6, value 1 and feature 10, value 0.5
   Example 2: no features
   Example 3: two copies of feature 2, value 1

  This SparseFeatureColumn will be represented as follows:
   <0, 5,  1>
   <1, 6,  1>
   <1, 10, 0.5>
   <3, 2,  1>
   <3, 2,  1>

  For a batch of 2 examples below:
   Example 0: feature 5
   Example 1: feature 6

  is represented by SparseFeatureColumn as:
   <0, 5,  1>
   <1, 6,  1>

  ```

  @@__init__
  @@example_indices
  @@feature_indices
  @@feature_values
  """

  def __init__(self, example_indices, feature_indices, feature_values):
    """Creates a `SparseFeatureColumn` representation.

    Args:
      example_indices: A 1-D int64 tensor of shape `[N]`. Also, accepts
      python lists, or numpy arrays.
      feature_indices: A 1-D int64 tensor of shape `[N]`. Also, accepts
      python lists, or numpy arrays.
      feature_values: An optional 1-D tensor float tensor of shape `[N]`. Also,
      accepts python lists, or numpy arrays.

    Returns:
      A `SparseFeatureColumn`
    """
    with op_scope([example_indices, feature_indices], None,
                  'SparseFeatureColumn'):
      self._example_indices = convert_to_tensor(example_indices,
                                                name='example_indices',
                                                dtype=dtypes.int64)
      self._feature_indices = convert_to_tensor(feature_indices,
                                                name='feature_indices',
                                                dtype=dtypes.int64)
    self._feature_values = None
    if feature_values is not None:
      with op_scope([feature_values], None, 'SparseFeatureColumn'):
        self._feature_values = convert_to_tensor(feature_values,
                                                 name='feature_values',
                                                 dtype=dtypes.float32)

  @property
  def example_indices(self):
    """The example indices represented as a dense tensor.
    Returns:
      A 1-D Tensor of int64 with shape `[N]`.
    """
    return self._example_indices

  @property
  def feature_indices(self):
    """The feature indices represented as a dense tensor.
    Returns:
      A 1-D Tensor of int64 with shape `[N]`.
    """
    return self._feature_indices

  @property
  def feature_values(self):
    """The feature values represented as a dense tensor.
    Returns:
      May return None, or a 1-D Tensor of float32 with shape `[N]`.
    """
    return self._feature_values


# TODO(sibyl-Aix6ihai): add op_scope to appropriate methods.
class SdcaModel(object):
  """Stochastic dual coordinate ascent solver for linear models.

    This class currently only supports a single machine (multi-threaded)
    implementation. We expect the weights and duals to fit in a single machine.

    Loss functions supported:
     * Binary logistic loss
     * Squared loss
     * Hinge loss

    This class defines an optimizer API to train a linear model.

    ### Usage

    ```python
    # Create a solver with the desired parameters.
    lr = tf.contrib.linear_optimizer.SdcaModel(
        container, examples, variables, options)
    opt_op = lr.minimize()

    predictions = lr.predictions(examples)
    # Primal loss + L1 loss + L2 loss.
    regularized_loss = lr.regularized_loss(examples)
    # Primal loss only
    unregularized_loss = lr.unregularized_loss(examples)

    container: Name of the container (eg a hex-encoded UUID) where internal
      state of the optimizer can be stored. The container can be safely shared
      across many models.
    examples: {
      sparse_features: list of SparseFeatureColumn.
      dense_features: list of dense tensors of type float32.
      example_labels: a tensor of type float32 and shape [Num examples]
      example_weights: a tensor of type float32 and shape [Num examples]
      example_ids: a tensor of type string and shape [Num examples]
    }
    variables: {
      sparse_features_weights: list of tensors of shape [vocab size]
      dense_features_weights: list of tensors of shape [1]
    }
    options: {
      symmetric_l1_regularization: 0.0
      symmetric_l2_regularization: 1.0
      loss_type: "logistic_loss"
    }
    ```

    In the training program you will just have to run the returned Op from
    minimize().

    ```python
    # Execute opt_op and train for num_steps.
    for _ in range(num_steps):
      opt_op.run()

    # You can also check for convergence by calling
    lr.approximate_duality_gap()
    ```
  """

  def __init__(self,
               container,
               examples,
               variables,
               options,
               num_table_shards=None):  # pylint: disable=unused-argument
    """Create a new sdca optimizer."""
    # TODO(andreasst): get rid of obsolete container parameter

    if not examples or not variables or not options:
      raise ValueError('examples, variables and options must all be specified.')

    supported_losses = ('logistic_loss', 'squared_loss', 'hinge_loss')
    if options['loss_type'] not in supported_losses:
      raise ValueError('Unsupported loss_type: ', options['loss_type'])

    self._assertSpecified(['example_labels', 'example_weights', 'example_ids',
                           'sparse_features', 'dense_features'], examples)
    self._assertList(['sparse_features', 'dense_features'], examples)

    self._assertSpecified(['sparse_features_weights', 'dense_features_weights'],
                          variables)
    self._assertList(['sparse_features_weights', 'dense_features_weights'],
                     variables)

    self._assertSpecified(['loss_type', 'symmetric_l2_regularization',
                           'symmetric_l1_regularization'], options)

    for name in ['symmetric_l1_regularization', 'symmetric_l2_regularization']:
      value = options[name]
      if value < 0.0:
        raise ValueError('%s should be non-negative. Found (%f)' %
                         (name, value))

    # TODO(andreasst): set num_table_shards automatically based on the number of
    # parameter servers
    if num_table_shards is None:
      num_table_shards = 1
    self._examples = examples
    self._variables = variables
    self._options = options
    self._create_slots()
    self._hashtable = _ShardedMutableHashTable(
        key_dtype=dtypes.string,
        value_dtype=dtypes.float32,
        num_shards=num_table_shards,
        default_value=[0.0, 0.0, 0.0, 0.0])

  def _symmetric_l2_regularization(self):
    # Algorithmic requirement (for now) is to have minimal l2 of 1.0.
    return max(self._options['symmetric_l2_regularization'], 1.0)

  # TODO(sibyl-Aix6ihai): Use optimizer interface to make use of slot creation logic.
  def _create_slots(self):
    # Make internal variables which have the updates before applying L1
    # regularization.
    self._slots = collections.defaultdict(list)
    for name in ['sparse_features_weights', 'dense_features_weights']:
      for var in self._variables[name]:
        self._slots['unshrinked_' + name].append(var_ops.Variable(
            array_ops.zeros_like(var.initialized_value(), dtypes.float32)))

  def _assertSpecified(self, items, check_in):
    for x in items:
      if check_in[x] is None:
        raise ValueError(check_in[x] + ' must be specified.')

  def _assertList(self, items, check_in):
    for x in items:
      if not isinstance(check_in[x], list):
        raise ValueError(x + ' must be a list.')

  def _l1_loss(self):
    """Computes the (un-normalized) l1 loss of the model."""
    with name_scope('l1_loss'):
      sum = 0.0
      for name in ['sparse_features_weights', 'dense_features_weights']:
        for weights in self._convert_n_to_tensor(self._variables[name]):
          sum += math_ops.reduce_sum(math_ops.abs(weights))
      # SDCA L1 regularization cost is: l1 * sum(|weights|)
      return self._options['symmetric_l1_regularization'] * sum

  def _l2_loss(self, l2):
    """Computes the (un-normalized) l2 loss of the model."""
    with name_scope('l2_loss'):
      sum = 0.0
      for name in ['sparse_features_weights', 'dense_features_weights']:
        for weights in self._convert_n_to_tensor(self._variables[name]):
          sum += math_ops.reduce_sum(math_ops.square(weights))
      # SDCA L2 regularization cost is: l2 * sum(weights^2) / 2
      return l2 * sum / 2.0

  def _convert_n_to_tensor(self, input_list, as_ref=False):
    """Converts input list to a set of tensors."""
    return [convert_to_tensor(x, as_ref=as_ref) for x in input_list]

  def _linear_predictions(self, examples):
    """Returns predictions of the form w*x."""
    with name_scope('sdca/prediction'):
      sparse_variables = self._convert_n_to_tensor(self._variables[
          'sparse_features_weights'])
      result = 0.0
      for sfc, sv in zip(examples['sparse_features'], sparse_variables):
        # TODO(sibyl-Aix6ihai): following does not take care of missing features.
        result += math_ops.segment_sum(
            math_ops.mul(
                array_ops.gather(sv, sfc.feature_indices), sfc.feature_values),
            sfc.example_indices)
      dense_features = self._convert_n_to_tensor(examples['dense_features'])
      dense_variables = self._convert_n_to_tensor(self._variables[
          'dense_features_weights'])

      for i in range(len(dense_variables)):
        result += dense_features[i] * dense_variables[i]

    # Reshaping to allow shape inference at graph construction time.
    return array_ops.reshape(result, [-1])

  def predictions(self, examples):
    """Add operations to compute predictions by the model.

    If logistic_loss is being used, predicted probabilities are returned.
    Otherwise, (raw) linear predictions (w*x) are returned.

    Args:
      examples: Examples to compute predictions on.

    Returns:
      An Operation that computes the predictions for examples.

    Raises:
      ValueError: if examples are not well defined.
    """
    self._assertSpecified(
        ['example_weights', 'sparse_features', 'dense_features'], examples)
    self._assertList(['sparse_features', 'dense_features'], examples)

    result = self._linear_predictions(examples)
    if self._options['loss_type'] == 'logistic_loss':
      # Convert logits to probability for logistic loss predictions.
      with name_scope('sdca/logistic_prediction'):
        result = math_ops.sigmoid(result)
    return result

  def minimize(self, global_step=None, name=None):
    """Add operations to train a linear model by minimizing the loss function.

    Args:
      global_step: Optional `Variable` to increment by one after the
        variables have been updated.
      name: Optional name for the returned operation.

    Returns:
      An Operation that updates the variables passed in the constructor.
    """
    # Technically, the op depends on a lot more than the variables,
    # but we'll keep the list short.
    with op_scope([], name, 'sdca/minimize'):
      sparse_example_indices = []
      sparse_feature_indices = []
      sparse_features_values = []
      for sf in self._examples['sparse_features']:
        sparse_example_indices.append(sf.example_indices)
        sparse_feature_indices.append(sf.feature_indices)
        # If feature values are missing, sdca assumes a value of 1.0f.
        if sf.feature_values is not None:
          sparse_features_values.append(sf.feature_values)

      example_ids_hashed = _sdca_ops.sdca_fprint(convert_to_tensor(
          self._examples['example_ids']))
      example_state_data = self._hashtable.lookup(example_ids_hashed)
      # Solver returns example_state_update, new delta sparse_feature_weights
      # and delta dense_feature_weights.
      esu, sfw, dfw = _sdca_ops.distributed_sdca_large_batch_solver(
          sparse_example_indices,
          sparse_feature_indices,
          sparse_features_values,
          self._convert_n_to_tensor(self._examples['dense_features']),
          convert_to_tensor(self._examples['example_weights']),
          convert_to_tensor(self._examples['example_labels']),
          self._convert_n_to_tensor(self._slots[
              'unshrinked_sparse_features_weights']),
          self._convert_n_to_tensor(self._slots[
              'unshrinked_dense_features_weights']),
          example_state_data,
          loss_type=self._options['loss_type'],
          l1=self._options['symmetric_l1_regularization'],
          l2=self._symmetric_l2_regularization(),
          num_partitions=1,
          # TODO(sibyl-Aix6ihai): Provide empirical evidence for this. It is better
          # to run more than one iteration on single mini-batch as we want to
          # spend more time in compute. SDCA works better with larger
          # mini-batches and there is also recent work that shows its better to
          # reuse old samples than train on new samples.
          # See: http://arxiv.org/abs/1602.02136.
          num_inner_iterations=2)

      out_fws = [sfw, dfw]
      with ops.control_dependencies([esu]):
        update_ops = [self._hashtable.insert(example_ids_hashed, esu)]
        # Update the weights before the proximal step.
        for i, name in enumerate(
            ['sparse_features_weights', 'dense_features_weights']):
          for slot_var, out_fw in zip(self._slots['unshrinked_' + name],
                                      out_fws[i]):
            update_ops.append(slot_var.assign_add(out_fw))

        with ops.control_dependencies(update_ops):
          update_ops = []
          # Copy over unshrinked weights to user provided variables.
          for i, name in enumerate(
              ['sparse_features_weights', 'dense_features_weights']):
            for var, slot_var in zip(self._variables[name],
                                     self._slots['unshrinked_' + name]):
              update_ops.append(var.assign(slot_var))

          update_group = control_flow_ops.group(*update_ops)

          # Apply proximal step.
          with ops.control_dependencies([update_group]):
            shrink_l1 = _sdca_ops.sdca_shrink_l1(
                self._convert_n_to_tensor(
                    self._variables['sparse_features_weights'],
                    as_ref=True),
                self._convert_n_to_tensor(
                    self._variables['dense_features_weights'],
                    as_ref=True),
                l1=self._options['symmetric_l1_regularization'],
                l2=self._symmetric_l2_regularization())
      if not global_step:
        return shrink_l1
      with ops.control_dependencies([shrink_l1]):
        return state_ops.assign_add(global_step, 1, name=name).op

  def approximate_duality_gap(self):
    """Add operations to compute the approximate duality gap.

    Returns:
      An Operation that computes the approximate duality gap over all
      examples.
    """
    summed_values = self._hashtable.values_reduce_sum()
    primal_loss = summed_values[1]
    dual_loss = summed_values[2]
    example_weights = summed_values[3]
    # TODO(andreasst): what about handle examples_weights == 0?
    return (
        primal_loss + dual_loss + math_ops.to_float(self._l1_loss()) +
        (2.0 *
         math_ops.to_float(self._l2_loss(self._symmetric_l2_regularization())))
    ) / example_weights

  def unregularized_loss(self, examples):
    """Add operations to compute the loss (without the regularization loss).

    Args:
      examples: Examples to compute unregularized loss on.

    Returns:
      An Operation that computes mean (unregularized) loss for given set of
      examples.

    Raises:
      ValueError: if examples are not well defined.
    """
    self._assertSpecified(['example_labels', 'example_weights',
                           'sparse_features', 'dense_features'], examples)
    self._assertList(['sparse_features', 'dense_features'], examples)
    with name_scope('sdca/unregularized_loss'):
      predictions = self._linear_predictions(examples)
      labels = convert_to_tensor(examples['example_labels'])
      weights = convert_to_tensor(examples['example_weights'])

      if self._options['loss_type'] == 'logistic_loss':
        return math_ops.reduce_sum(math_ops.mul(
            sigmoid_cross_entropy_with_logits(predictions, labels),
            weights)) / math_ops.reduce_sum(weights)

      if self._options['loss_type'] == 'hinge_loss':
        # hinge_loss = max{0, 1 - y_i w*x} where y_i \in {-1, 1}. So, we need to
        # first convert 0/1 labels into -1/1 labels.
        all_ones = array_ops.ones_like(predictions)
        adjusted_labels = math_ops.sub(2 * labels, all_ones)
        # Tensor that contains (unweighted) error (hinge loss) per
        # example.
        error = nn_ops.relu(math_ops.sub(all_ones, math_ops.mul(adjusted_labels,
                                                                predictions)))
        weighted_error = math_ops.mul(error, weights)
        return math_ops.reduce_sum(weighted_error) / math_ops.reduce_sum(
            weights)

      # squared loss
      err = math_ops.sub(labels, predictions)

      weighted_squared_err = math_ops.mul(math_ops.square(err), weights)
      # SDCA squared loss function is sum(err^2) / (2*sum(weights))
      return (math_ops.reduce_sum(weighted_squared_err) /
              (2.0 * math_ops.reduce_sum(weights)))

  def regularized_loss(self, examples):
    """Add operations to compute the loss with regularization loss included.

    Args:
      examples: Examples to compute loss on.

    Returns:
      An Operation that computes mean (regularized) loss for given set of
      examples.
    Raises:
      ValueError: if examples are not well defined.
    """
    self._assertSpecified(['example_labels', 'example_weights',
                           'sparse_features', 'dense_features'], examples)
    self._assertList(['sparse_features', 'dense_features'], examples)
    with name_scope('sdca/regularized_loss'):
      weights = convert_to_tensor(examples['example_weights'])
      return ((
          self._l1_loss() +
          # Note that here we are using the raw regularization
          # (as specified by the user) and *not*
          # self._symmetric_l2_regularization().
          self._l2_loss(self._options['symmetric_l2_regularization'])) /
              math_ops.reduce_sum(weights) + self.unregularized_loss(examples))
