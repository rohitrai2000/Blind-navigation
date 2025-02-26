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

"""Tests for SparseConcat."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class SparseConcatTest(tf.test.TestCase):

  def _SparseTensor_UnknownShape(self, ind_shape=None, val_shape=None,
                                 shape_shape=None):
    return tf.SparseTensor(
        tf.placeholder(tf.int64, shape=ind_shape),
        tf.placeholder(tf.float32, shape=val_shape),
        tf.placeholder(tf.int64, shape=shape_shape))

  def _SparseTensor_3x3(self):
    # [    1]
    # [2    ]
    # [3   4]
    ind = np.array([[0, 2], [1, 0], [2, 0], [2, 2]])
    val = np.array([1, 2, 3, 4])
    shape = np.array([3, 3])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.float32),
        tf.constant(shape, tf.int64))

  def _SparseTensor_3x5(self):
    # [         ]
    # [  1      ]
    # [2     1 0]
    ind = np.array([[1, 1], [2, 0], [2, 3], [2, 4]])
    val = np.array([1, 2, 1, 0])
    shape = np.array([3, 5])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.float32),
        tf.constant(shape, tf.int64))

  def _SparseTensor_3x2(self):
    # [   ]
    # [1  ]
    # [2  ]
    ind = np.array([[1, 0], [2, 0]])
    val = np.array([1, 2])
    shape = np.array([3, 2])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.float32),
        tf.constant(shape, tf.int64))

  def _SparseTensor_2x3(self):
    # [  1  ]
    # [1   2]
    ind = np.array([[0, 1], [1, 0], [1, 2]])
    val = np.array([1, 1, 2])
    shape = np.array([2, 3])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.float32),
        tf.constant(shape, tf.int64))

  def _SparseTensor_2x3x4(self):
    ind = np.array([
        [0, 0, 1],
        [0, 1, 0], [0, 1, 2],
        [1, 0, 3],
        [1, 1, 1], [1, 1, 3],
        [1, 2, 2]])
    val = np.array([1, 10, 12, 103, 111, 113, 122])
    shape = np.array([2, 3, 4])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.float32),
        tf.constant(shape, tf.int64))

  def _SparseTensor_String3x3(self):
    # [    a]
    # [b    ]
    # [c   d]
    ind = np.array([[0, 2], [1, 0], [2, 0], [2, 2]])
    val = np.array(["a", "b", "c", "d"])
    shape = np.array([3, 3])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.string),
        tf.constant(shape, tf.int64))

  def _SparseTensor_String3x5(self):
    # [         ]
    # [  e      ]
    # [f     g h]
    ind = np.array([[1, 1], [2, 0], [2, 3], [2, 4]])
    val = np.array(["e", "f", "g", "h"])
    shape = np.array([3, 5])
    return tf.SparseTensor(
        tf.constant(ind, tf.int64),
        tf.constant(val, tf.string),
        tf.constant(shape, tf.int64))

  def testConcat1(self):
    with self.test_session(use_gpu=False) as sess:
      # concat(A):
      # [    1]
      # [2    ]
      # [3   4]
      sp_a = self._SparseTensor_3x3()

      sp_concat = tf.sparse_concat(1, [sp_a])

      self.assertEqual(sp_concat.indices.get_shape(), [4, 2])
      self.assertEqual(sp_concat.values.get_shape(), [4])
      self.assertEqual(sp_concat.shape.get_shape(), [2])

      concat_out = sess.run(sp_concat)

      self.assertAllEqual(
          concat_out.indices, [[0, 2], [1, 0], [2, 0], [2, 2]])
      self.assertAllEqual(concat_out.values, [1, 2, 3, 4])
      self.assertAllEqual(concat_out.shape, [3, 3])

  def testConcat2(self):
    with self.test_session(use_gpu=False) as sess:
      # concat(A, B):
      # [    1          ]
      # [2       1      ]
      # [3   4 2     1 0]
      sp_a = self._SparseTensor_3x3()
      sp_b = self._SparseTensor_3x5()

      sp_concat = tf.sparse_concat(1, [sp_a, sp_b])

      self.assertEqual(sp_concat.indices.get_shape(), [8, 2])
      self.assertEqual(sp_concat.values.get_shape(), [8])
      self.assertEqual(sp_concat.shape.get_shape(), [2])

      concat_out = sess.run(sp_concat)

      self.assertAllEqual(
          concat_out.indices,
          [[0, 2], [1, 0], [1, 4], [2, 0], [2, 2], [2, 3], [2, 6], [2, 7]])
      self.assertAllEqual(concat_out.values, [1, 2, 1, 3, 4, 2, 1, 0])
      self.assertAllEqual(concat_out.shape, [3, 8])

  def testConcatDim0(self):
    with self.test_session(use_gpu=False) as sess:
      # concat(A, D):
      # [    1]
      # [2    ]
      # [3   4]
      # [  1  ]
      # [1   2]
      sp_a = self._SparseTensor_3x3()
      sp_d = self._SparseTensor_2x3()

      sp_concat = tf.sparse_concat(0, [sp_a, sp_d])

      self.assertEqual(sp_concat.indices.get_shape(), [7, 2])
      self.assertEqual(sp_concat.values.get_shape(), [7])
      self.assertEqual(sp_concat.shape.get_shape(), [2])

      concat_out = sess.run(sp_concat)

      self.assertAllEqual(
          concat_out.indices,
          [[0, 2], [1, 0], [2, 0], [2, 2], [3, 1], [4, 0], [4, 2]])
      self.assertAllEqual(
          concat_out.values, np.array([1, 2, 3, 4, 1, 1, 2]))
      self.assertAllEqual(
          concat_out.shape, np.array([5, 3]))

  def testConcat3(self):
    with self.test_session(use_gpu=False) as sess:
      # concat(A, B, C):
      # [    1              ]
      # [2       1       1  ]
      # [3   4 2     1 0 2  ]
      sp_a = self._SparseTensor_3x3()
      sp_b = self._SparseTensor_3x5()
      sp_c = self._SparseTensor_3x2()

      sp_concat = tf.sparse_concat(1, [sp_a, sp_b, sp_c])

      self.assertEqual(sp_concat.indices.get_shape(), [10, 2])
      self.assertEqual(sp_concat.values.get_shape(), [10])
      self.assertEqual(sp_concat.shape.get_shape(), [2])

      concat_out = sess.run(sp_concat)

      self.assertAllEqual(
          concat_out.indices,
          [[0, 2], [1, 0], [1, 4], [1, 8], [2, 0], [2, 2], [2, 3], [2, 6],
           [2, 7], [2, 8]])
      self.assertAllEqual(concat_out.values, [1, 2, 1, 1, 3, 4, 2, 1, 0, 2])
      self.assertAllEqual(concat_out.shape, [3, 10])

  def testConcatNonNumeric(self):
    with self.test_session(use_gpu=False) as sess:
      # concat(A, B):
      # [    a          ]
      # [b       e      ]
      # [c   d f     g h]
      sp_a = self._SparseTensor_String3x3()
      sp_b = self._SparseTensor_String3x5()

      sp_concat = tf.sparse_concat(1, [sp_a, sp_b])

      self.assertEqual(sp_concat.indices.get_shape(), [8, 2])
      self.assertEqual(sp_concat.values.get_shape(), [8])
      self.assertEqual(sp_concat.shape.get_shape(), [2])

      concat_out = sess.run(sp_concat)

      self.assertAllEqual(
          concat_out.indices,
          [[0, 2], [1, 0], [1, 4], [2, 0], [2, 2], [2, 3], [2, 6], [2, 7]])
      self.assertAllEqual(
          concat_out.values, [b"a", b"b", b"e", b"c", b"d", b"f", b"g", b"h"])
      self.assertAllEqual(concat_out.shape, [3, 8])

  def testMismatchedRank(self):
    with self.test_session(use_gpu=False):
      sp_a = self._SparseTensor_3x3()
      sp_e = self._SparseTensor_2x3x4()

      # Rank mismatches can be caught at shape-inference time
      with self.assertRaises(ValueError):
        tf.sparse_concat(1, [sp_a, sp_e])

  def testMismatchedRankExpandNonconcatDim(self):
    with self.test_session(use_gpu=False):
      sp_a = self._SparseTensor_3x3()
      sp_e = self._SparseTensor_2x3x4()

      # Rank mismatches should be caught at shape-inference time, even for
      # expand_nonconcat_dim=True.
      with self.assertRaises(ValueError):
        tf.sparse_concat(1, [sp_a, sp_e], expand_nonconcat_dim=True)

  def testMismatchedShapes(self):
    with self.test_session(use_gpu=False) as sess:
      sp_a = self._SparseTensor_3x3()
      sp_b = self._SparseTensor_3x5()
      sp_c = self._SparseTensor_3x2()
      sp_d = self._SparseTensor_2x3()
      sp_concat = tf.sparse_concat(1, [sp_a, sp_b, sp_c, sp_d])

      # Shape mismatches can only be caught when the op is run
      with self.assertRaisesOpError("Input shapes must match"):
        sess.run(sp_concat)

  def testMismatchedShapesExpandNonconcatDim(self):
    with self.test_session(use_gpu=False) as sess:
      sp_a = self._SparseTensor_3x3()
      sp_b = self._SparseTensor_3x5()
      sp_c = self._SparseTensor_3x2()
      sp_d = self._SparseTensor_2x3()
      sp_concat_dim0 = tf.sparse_concat(0, [sp_a, sp_b, sp_c, sp_d],
                                        expand_nonconcat_dim=True)
      sp_concat_dim1 = tf.sparse_concat(1, [sp_a, sp_b, sp_c, sp_d],
                                        expand_nonconcat_dim=True)

      sp_concat_dim0_out = sess.run(sp_concat_dim0)
      sp_concat_dim1_out = sess.run(sp_concat_dim1)

      self.assertAllEqual(
          sp_concat_dim0_out.indices,
          [[0, 2], [1, 0], [2, 0], [2, 2], [4, 1], [5, 0], [5, 3], [5, 4],
           [7, 0], [8, 0], [9, 1], [10, 0], [10, 2]])
      self.assertAllEqual(
          sp_concat_dim0_out.values,
          [1, 2, 3, 4, 1, 2, 1, 0, 1, 2, 1, 1, 2])
      self.assertAllEqual(
          sp_concat_dim0_out.shape,
          [11, 5])

      self.assertAllEqual(
          sp_concat_dim1_out.indices,
          [[0, 2], [0, 11], [1, 0], [1, 4], [1, 8], [1, 10], [1, 12], [2, 0],
           [2, 2], [2, 3], [2, 6], [2, 7], [2, 8]])
      self.assertAllEqual(
          sp_concat_dim1_out.values,
          [1, 1, 2, 1, 1, 1, 2, 3, 4, 2, 1, 0, 2])
      self.assertAllEqual(
          sp_concat_dim1_out.shape,
          [3, 13])

  def testShapeInferenceUnknownShapes(self):
    with self.test_session(use_gpu=False):
      sp_inputs = [
          self._SparseTensor_UnknownShape(),
          self._SparseTensor_UnknownShape(val_shape=[3]),
          self._SparseTensor_UnknownShape(ind_shape=[1, 3]),
          self._SparseTensor_UnknownShape(shape_shape=[3])]

      sp_concat = tf.sparse_concat(0, sp_inputs)

      self.assertEqual(sp_concat.indices.get_shape().as_list(), [None, 3])
      self.assertEqual(sp_concat.values.get_shape().as_list(), [None])
      self.assertEqual(sp_concat.shape.get_shape(), [3])


if __name__ == "__main__":
  tf.test.main()
