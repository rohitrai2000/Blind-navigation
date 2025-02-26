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
"""Tests for tf.contrib.layers.sparse_feature_cross."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf


class SparseCrossOpTest(tf.test.TestCase):

  def test_simple(self):
    """Tests a simple scenario.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            ['batch1-FC1-F1'], ['batch2-FC1-F1', 'batch2-FC1-F2']
        ]), self._sparse_tensor([
            ['batch1-FC2-F1'], ['batch2-FC2-F1', 'batch2-FC2-F2']
        ])
    ])
    expected_out = self._sparse_tensor([
        ['batch1-FC1-F1_X_batch1-FC2-F1'],
        ['batch2-FC1-F1_X_batch2-FC2-F1', 'batch2-FC1-F1_X_batch2-FC2-F2',
         'batch2-FC1-F2_X_batch2-FC2-F1', 'batch2-FC1-F2_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_dense(self):
    """Tests only dense inputs.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        tf.constant(
            [['batch1-FC1-F1', 'batch1-FC1-F2'], ['batch2-FC1-F1',
                                                  'batch2-FC1-F2']], tf.string),
        tf.constant(
            [['batch1-FC2-F1', 'batch1-FC2-F2'], ['batch2-FC2-F1',
                                                  'batch2-FC2-F2']], tf.string),
    ])
    expected_out = self._sparse_tensor([
        ['batch1-FC1-F1_X_batch1-FC2-F1', 'batch1-FC1-F1_X_batch1-FC2-F2',
         'batch1-FC1-F2_X_batch1-FC2-F1', 'batch1-FC1-F2_X_batch1-FC2-F2'],
        ['batch2-FC1-F1_X_batch2-FC2-F1', 'batch2-FC1-F1_X_batch2-FC2-F2',
         'batch2-FC1-F2_X_batch2-FC2-F1', 'batch2-FC1-F2_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_integer_mixed_string_sparse(self):
    """Tests mixed type."""
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            [11], [333, 55555]
        ]), self._sparse_tensor([
            ['batch1-FC2-F1'], ['batch2-FC2-F1', 'batch2-FC2-F2']
        ])
    ])
    expected_out = self._sparse_tensor([
        ['11_X_batch1-FC2-F1'
        ], ['333_X_batch2-FC2-F1', '333_X_batch2-FC2-F2',
            '55555_X_batch2-FC2-F1', '55555_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_integer_mixed_string_dense(self):
    """Tests mixed dense inputs.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        tf.constant(
            [[11, 333], [55555, 999999]], tf.int64),
        tf.constant(
            [['batch1-FC2-F1', 'batch1-FC2-F2'], ['batch2-FC2-F1',
                                                  'batch2-FC2-F2']], tf.string),
    ])
    expected_out = self._sparse_tensor([
        ['11_X_batch1-FC2-F1', '11_X_batch1-FC2-F2', '333_X_batch1-FC2-F1',
         '333_X_batch1-FC2-F2'
        ], ['55555_X_batch2-FC2-F1', '55555_X_batch2-FC2-F2',
            '999999_X_batch2-FC2-F1', '999999_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_sparse_cross_dense(self):
    """Tests sparse and dense inputs.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            ['batch1-FC1-F1'], ['batch2-FC1-F1', 'batch2-FC1-F2']
        ]),
        tf.constant(
            [['batch1-FC2-F1', 'batch1-FC2-F2'], ['batch2-FC2-F1',
                                                  'batch2-FC2-F2']], tf.string),
    ])
    expected_out = self._sparse_tensor([
        ['batch1-FC1-F1_X_batch1-FC2-F1', 'batch1-FC1-F1_X_batch1-FC2-F2'],
        ['batch2-FC1-F1_X_batch2-FC2-F1', 'batch2-FC1-F1_X_batch2-FC2-F2',
         'batch2-FC1-F2_X_batch2-FC2-F1', 'batch2-FC1-F2_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_integer_sparse_input(self):
    """Tests mixed type sparse and dense inputs."""
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            [11], [333, 5555]
        ]),
        tf.constant(
            [['batch1-FC2-F1', 'batch1-FC2-F2'], ['batch2-FC2-F1',
                                                  'batch2-FC2-F2']], tf.string),
    ])
    expected_out = self._sparse_tensor([
        ['11_X_batch1-FC2-F1', '11_X_batch1-FC2-F2'
        ], ['333_X_batch2-FC2-F1', '333_X_batch2-FC2-F2',
            '5555_X_batch2-FC2-F1', '5555_X_batch2-FC2-F2']
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_permutation_3x3x3(self):
    """Tests 3x3x3 permutation.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            ['batch1-FC1-F1', 'batch1-FC1-F2', 'batch1-FC1-F3']
        ]), self._sparse_tensor([
            ['batch1-FC2-F1', 'batch1-FC2-F2', 'batch1-FC2-F3']
        ]), self._sparse_tensor([
            ['batch1-FC3-F1', 'batch1-FC3-F2', 'batch1-FC3-F3']
        ])
    ])
    expected_out = self._sparse_tensor([
        ['batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F2',
         'batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F3',
         'batch1-FC1-F1_X_batch1-FC2-F2_X_batch1-FC3-F1',
         'batch1-FC1-F1_X_batch1-FC2-F2_X_batch1-FC3-F2',
         'batch1-FC1-F1_X_batch1-FC2-F2_X_batch1-FC3-F3',
         'batch1-FC1-F1_X_batch1-FC2-F3_X_batch1-FC3-F1',
         'batch1-FC1-F1_X_batch1-FC2-F3_X_batch1-FC3-F2',
         'batch1-FC1-F1_X_batch1-FC2-F3_X_batch1-FC3-F3',

         'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F2',
         'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F3',
         'batch1-FC1-F2_X_batch1-FC2-F2_X_batch1-FC3-F1',
         'batch1-FC1-F2_X_batch1-FC2-F2_X_batch1-FC3-F2',
         'batch1-FC1-F2_X_batch1-FC2-F2_X_batch1-FC3-F3',
         'batch1-FC1-F2_X_batch1-FC2-F3_X_batch1-FC3-F1',
         'batch1-FC1-F2_X_batch1-FC2-F3_X_batch1-FC3-F2',
         'batch1-FC1-F2_X_batch1-FC2-F3_X_batch1-FC3-F3',

         'batch1-FC1-F3_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F3_X_batch1-FC2-F1_X_batch1-FC3-F2',
         'batch1-FC1-F3_X_batch1-FC2-F1_X_batch1-FC3-F3',
         'batch1-FC1-F3_X_batch1-FC2-F2_X_batch1-FC3-F1',
         'batch1-FC1-F3_X_batch1-FC2-F2_X_batch1-FC3-F2',
         'batch1-FC1-F3_X_batch1-FC2-F2_X_batch1-FC3-F3',
         'batch1-FC1-F3_X_batch1-FC2-F3_X_batch1-FC3-F1',
         'batch1-FC1-F3_X_batch1-FC2-F3_X_batch1-FC3-F2',
         'batch1-FC1-F3_X_batch1-FC2-F3_X_batch1-FC3-F3']])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_permutation_3x1x2(self):
    """Tests 3x1x2 permutation.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([
            ['batch1-FC1-F1', 'batch1-FC1-F2', 'batch1-FC1-F3']
        ]), self._sparse_tensor([
            ['batch1-FC2-F1']
        ]), self._sparse_tensor([
            ['batch1-FC3-F1', 'batch1-FC3-F2']
        ])
    ])
    expected_out = self._sparse_tensor([
        ['batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F2',
         'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F2',
         'batch1-FC1-F3_X_batch1-FC2-F1_X_batch1-FC3-F1',
         'batch1-FC1-F3_X_batch1-FC2-F1_X_batch1-FC3-F2']])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_large_batch(self):
    """Tests with large batch size to force multithreding.
    """
    batch_size = 5000
    col1 = []
    col2 = []
    col3 = []
    for b in range(batch_size):
      col1.append(['batch%d-FC1-F1' % b,
                   'batch%d-FC1-F2' % b,
                   'batch%d-FC1-F3' % b])
      col2.append(['batch%d-FC2-F1' % b])
      col3.append(['batch%d-FC3-F1' % b,
                   'batch%d-FC3-F2' % b])

    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor(col1), self._sparse_tensor(col2),
        self._sparse_tensor(col3)
    ])

    col_out = []
    for b in range(batch_size):
      col_out.append([
          'batch%d-FC1-F1_X_batch%d-FC2-F1_X_batch%d-FC3-F1' % (b, b, b),
          'batch%d-FC1-F1_X_batch%d-FC2-F1_X_batch%d-FC3-F2' % (b, b, b),
          'batch%d-FC1-F2_X_batch%d-FC2-F1_X_batch%d-FC3-F1' % (b, b, b),
          'batch%d-FC1-F2_X_batch%d-FC2-F1_X_batch%d-FC3-F2' % (b, b, b),
          'batch%d-FC1-F3_X_batch%d-FC2-F1_X_batch%d-FC3-F1' % (b, b, b),
          'batch%d-FC1-F3_X_batch%d-FC2-F1_X_batch%d-FC3-F2' % (b, b, b)])

    expected_out = self._sparse_tensor(col_out)
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_one_column_empty(self):
    """Tests when one column is empty.

    The crossed tensor should be empty.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([['batch1-FC1-F1', 'batch1-FC1-F2']]),
        self._sparse_tensor(
            [], 1), self._sparse_tensor([['batch1-FC3-F1', 'batch1-FC3-F2']])
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_empty(sess.run(op))

  def test_some_columns_empty(self):
    """Tests when more than one columns are empty.

    Cross for the corresponding batch should be empty.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor(
            [['batch1-FC1-F1', 'batch1-FC1-F2']], 2), self._sparse_tensor(
                [['batch1-FC2-F1'], ['batch2-FC2-F1']], 2), self._sparse_tensor(
                    [['batch1-FC3-F1', 'batch1-FC3-F2']], 2)
    ])
    expected_out = self._sparse_tensor([[
        'batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F1',
        'batch1-FC1-F1_X_batch1-FC2-F1_X_batch1-FC3-F2',
        'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F1',
        'batch1-FC1-F2_X_batch1-FC2-F1_X_batch1-FC3-F2'
    ]], 2)
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_all_columns_empty(self):
    """Tests when all columns are empty.

    The crossed tensor should be empty.
    """
    op = tf.contrib.layers.sparse_feature_cross([
        self._sparse_tensor([]), self._sparse_tensor([]), self._sparse_tensor(
            [])
    ])
    with self.test_session() as sess:
      self._assert_sparse_tensor_empty(sess.run(op))

  def test_hashed_output_zero_bucket(self):
    """Tests a simple scenario.
    """
    op = tf.contrib.layers.sparse_feature_cross(
        [
            self._sparse_tensor([
                ['batch1-FC1-F1']
            ]), self._sparse_tensor([
                ['batch1-FC2-F1']
            ]), self._sparse_tensor([
                ['batch1-FC3-F1']
            ])
        ],
        hashed_output=True)
    # Check actual hashed output to prevent unintentional hashing changes.
    expected_out = self._sparse_tensor([[3735511728867393167]])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  # TODO(sibyl-Aix6ihai): Add benchmark to compare Hashed vs Non-hashed.
  def test_hashed_output(self):
    """Tests a simple scenario.
    """
    op = tf.contrib.layers.sparse_feature_cross(
        [
            self._sparse_tensor([
                ['batch1-FC1-F1']
            ]), self._sparse_tensor([
                ['batch1-FC2-F1']
            ]), self._sparse_tensor([
                ['batch1-FC3-F1']
            ])
        ],
        hashed_output=True,
        num_buckets=100)
    # Check actual hashed output to prevent unintentional hashing changes.
    expected_out = self._sparse_tensor([[74]])
    with self.test_session() as sess:
      self._assert_sparse_tensor_equals(expected_out, sess.run(op))

  def test_hashed_3x1x2(self):
    """Tests 3x1x2 permutation with hashed output.
    """
    op = tf.contrib.layers.sparse_feature_cross(
        [
            self._sparse_tensor([
                ['batch1-FC1-F1', 'batch1-FC1-F2', 'batch1-FC1-F3']
            ]), self._sparse_tensor([
                ['batch1-FC2-F1']
            ]), self._sparse_tensor([
                ['batch1-FC3-F1', 'batch1-FC3-F2']
            ])
        ],
        hashed_output=True,
        num_buckets=1000)
    with self.test_session() as sess:
      out = sess.run(op)
      self.assertEqual(6, len(out.values))
      self.assertAllEqual([[0, i] for i in range(6)], out.indices)
      self.assertTrue(all(x < 1000 and x >= 0 for x in out.values))
      all_values_are_different = len(out.values) == len(set(out.values))
      self.assertTrue(all_values_are_different)

  def _assert_sparse_tensor_empty(self, sp):
    self.assertEquals(0, sp.indices.size)
    self.assertEquals(0, sp.values.size)
    # TODO(zakaria): check if we can ignore the first dim of the shape.
    self.assertEquals(0, sp.shape[1])

  def _assert_sparse_tensor_equals(self, sp1, sp2):
    self.assertAllEqual(sp1.indices.eval(), sp2.indices)
    self.assertAllEqual(sp1.values.eval(), sp2.values)
    self.assertAllEqual(sp1.shape.eval(), sp2.shape)

  def _sparse_tensor(self, data, batch_size=-1):
    """Generates a SparseTensor.

    Args:
      data: Should be a list of list of strings or int64. Each item of the outer
          list represents a batch. Each item of the batch is a feature of a
          specific feature column.
      batch_size: optional batch size, especially for cases when data has no
          entry for some batches.

    Returns:
     A SparseTensor.
    """
    indices = []
    values = []
    max_col_count = 0
    for batch, batch_ix in zip(data, range(len(data))):
      for column, column_ix in zip(batch, range(len(batch))):
        indices.append([batch_ix, column_ix])
        values.append(column)
        max_col_count = max(max_col_count, column_ix + 1)
    shape = [batch_size if batch_size != -1 else len(data), max_col_count]
    value_type = (tf.string if not values or isinstance(values[0], str) else
                  tf.int64)
    return tf.SparseTensor(
        tf.constant(indices, tf.int64, [len(indices), 2]),
        tf.constant(values, value_type, [len(indices)]),
        tf.constant(shape, tf.int64))

if __name__ == '__main__':
  tf.test.main()
