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

"""Tests for tensorflow.ops.math_ops.matrix_solve."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class MatrixSolveOpTest(tf.test.TestCase):

  def _verifySolve(self, x, y, batch_dims=None):
    for adjoint in False, True:
      for np_type in [np.float32, np.float64]:
        a = x.astype(np_type)
        b = y.astype(np_type)
        if adjoint:
          a_np = np.conj(np.transpose(a))
        else:
          a_np = a
        if batch_dims is not None:
          a = np.tile(a, batch_dims + [1, 1])
          a_np = np.tile(a_np, batch_dims + [1, 1])
          b = np.tile(b, batch_dims + [1, 1])

        np_ans = np.linalg.solve(a_np, b)
        with self.test_session():
          # Test the batch version, which works for ndim >= 2
          tf_ans = tf.batch_matrix_solve(a, b, adjoint=adjoint)
          out = tf_ans.eval()
          self.assertEqual(tf_ans.get_shape(), out.shape)
          self.assertEqual(np_ans.shape, out.shape)
          self.assertAllClose(np_ans, out)

          if a.ndim == 2:
            # Test the simple version
            tf_ans = tf.matrix_solve(a, b, adjoint=adjoint)
            out = tf_ans.eval()
            self.assertEqual(out.shape, tf_ans.get_shape())
            self.assertEqual(np_ans.shape, out.shape)
            self.assertAllClose(np_ans, out)

  def testSolve(self):
    # 2x2 matrices, 2x1 right-hand side.
    matrix = np.array([[1., 2.], [3., 4.]])
    rhs0 = np.array([[1.], [1.]])
    self._verifySolve(matrix, rhs0)
    # 2x2 matrices, 2xx right-hand sides.
    matrix = np.array([[1., 2.], [3., 4.]])
    rhs1 = np.array([[1., 0., 1.], [0., 1., 1.]])
    self._verifySolve(matrix, rhs1)

  def testSolveBatch(self):
    matrix = np.array([[1., 2.], [3., 4.]])
    rhs = np.array([[1., 0., 1.], [0., 1., 1.]])
    # Batch of 2x3x2x2 matrices, 2x3x2x3 right-hand sides.
    self._verifySolve(matrix, rhs, batch_dims=[2, 3])
    # Batch of 3x2x2x2 matrices, 3x2x2x3 right-hand sides.
    self._verifySolve(matrix, rhs, batch_dims=[3, 2])

  def testNonSquareMatrix(self):
    # When the solve of a non-square matrix is attempted we should return
    # an error
    with self.test_session():
      with self.assertRaises(ValueError):
        matrix = tf.constant([[1., 2., 3.], [3., 4., 5.]])
        tf.matrix_solve(matrix, matrix)

  def testWrongDimensions(self):
    # The matrix and right-hand sides should have the same number of rows.
    with self.test_session():
      matrix = tf.constant([[1., 0.], [0., 1.]])
      rhs = tf.constant([[1., 0.]])
      with self.assertRaises(ValueError):
        tf.matrix_solve(matrix, rhs)

  def testNotInvertible(self):
    # The input should be invertible.
    with self.test_session():
      with self.assertRaisesOpError("Input matrix is not invertible."):
        # All rows of the matrix below add to zero
        matrix = tf.constant([[1., 0., -1.], [-1., 1., 0.], [0., -1., 1.]])
        tf.matrix_solve(matrix, matrix).eval()

  def testEmpty(self):
    with self.test_session():
      self._verifySolve(np.empty([0, 0]), np.empty([0, 0]))
      self._verifySolve(np.empty([2, 2]), np.empty([2, 0]))


if __name__ == "__main__":
  tf.test.main()
