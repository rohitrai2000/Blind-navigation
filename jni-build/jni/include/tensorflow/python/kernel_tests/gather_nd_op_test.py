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

"""Tests for tensorflow.ops.tf.gather_nd."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import time

import numpy as np
import tensorflow as tf


class GatherNdTest(tf.test.TestCase):
  use_gpu = False

  def _testSimpleDtype(self, dtype):
    with self.test_session(use_gpu=self.use_gpu):
      params = tf.constant(np.array([8, 1, 2, 3, 7, 5], dtype=dtype))
      indices = tf.constant([[4], [4], [0]])
      gather_nd_t = tf.gather_nd(params, indices)
      gather_nd_val = gather_nd_t.eval()

    self.assertAllEqual(np.array([7, 7, 8], dtype=dtype), gather_nd_val)
    self.assertEqual([3], gather_nd_t.get_shape())

  def testSimpleDtype(self):
    self._testSimpleDtype(np.float32)
    self._testSimpleDtype(np.float64)
    self._testSimpleDtype(np.int32)
    self._testSimpleDtype(np.int64)
    self._testSimpleDtype(np.complex64)
    self._testSimpleDtype("|S")  # byte strings in python2 + 3

  def testEmptyIndicesAndParamsOKButJustEmptyParamsFails(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.ones((3, 3), dtype=np.float32)

      indices_empty = np.empty((0, 2), dtype=np.int32)
      gather_nd_ok_t = tf.gather_nd(params, indices_empty)
      gather_nd_ok_val = gather_nd_ok_t.eval()
      self.assertEqual([0], gather_nd_ok_t.get_shape())
      self.assertAllEqual(np.empty((0,), dtype=np.float32), gather_nd_ok_val)

      indices_empty = np.empty((0, 1), dtype=np.int32)
      gather_nd_ok_t = tf.gather_nd(params, indices_empty)
      gather_nd_ok_val = gather_nd_ok_t.eval()
      self.assertEqual([0, 3], gather_nd_ok_t.get_shape())
      self.assertAllEqual(np.empty((0, 3), dtype=np.float32), gather_nd_ok_val)

      params_empty = np.empty((0, 3), dtype=np.float32)
      indices_empty = np.empty((0, 2), dtype=np.int32)
      gather_nd_ok_t = tf.gather_nd(params_empty, indices_empty)
      gather_nd_ok_val = gather_nd_ok_t.eval()
      self.assertEqual([0], gather_nd_ok_t.get_shape())
      self.assertAllEqual(np.empty((0,), dtype=np.float32), gather_nd_ok_val)

      params_empty = np.empty((0, 3), dtype=np.float32)
      indices_nonempty = np.zeros((1, 2), dtype=np.int32)
      gather_nd_break_t = tf.gather_nd(params_empty, indices_nonempty)
      with self.assertRaisesOpError(
          r"Requested more than 0 entries, but params is empty."):
        gather_nd_break_t.eval()
      self.assertAllEqual(np.empty((0,), dtype=np.float32), gather_nd_ok_val)

  def testIndexScalar(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array([[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
                        dtype=np.float32).T
      indices = tf.constant([4, 1])
      gather_nd_t = tf.gather_nd(params, indices)
      gather_nd_val = gather_nd_t.eval()
      self.assertEqual([], gather_nd_t.get_shape())
      self.assertAllEqual(np.array(7), gather_nd_val)

  def testParamsRankLargerThanIndexIndexScalarSlices(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array([[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
                        dtype=np.float32).T
      indices = tf.constant([4])
      gather_nd_t = tf.gather_nd(params, indices)
      gather_nd_val = gather_nd_t.eval()
      self.assertEqual([2], gather_nd_t.get_shape())
      self.assertAllEqual(np.array([-7, 7]), gather_nd_val)

  def testParamsRankLargerThanIndexSlices(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array([[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
                        dtype=np.float32).T
      indices = tf.constant([[4], [4], [0]])
      gather_nd_t = tf.gather_nd(params, indices)
      gather_nd_val = gather_nd_t.eval()

    self.assertEqual([3, 2], gather_nd_t.get_shape())
    self.assertAllEqual(np.array([[-7, 7], [-7, 7], [-8, 8]]), gather_nd_val)

  def testHigherRankParamsLargerThanIndexSlices(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array(
          [[[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
           [[-80, -10, -20, -30, -70, -50], [80, 10, 20, 30, 70, 50]]],
          dtype=np.float32).T
      params_t = tf.constant(params)
      indices = tf.constant([[4], [4], [0]])
      gather_nd_t = tf.gather_nd(params_t, indices)
      gather_nd_val = gather_nd_t.eval()

    self.assertEqual([3, 2, 2], gather_nd_t.get_shape())
    self.assertAllEqual(params[[4, 4, 0]], gather_nd_val)

  def testEmptyIndicesLastRankMeansCopyEntireTensor(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array(
          [[[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
           [[-80, -10, -20, -30, -70, -50], [80, 10, 20, 30, 70, 50]]],
          dtype=np.float32).T
      params_t = tf.constant(params)
      indices = tf.constant([[], []], dtype=tf.int32)  # Size (2, 0)
      gather_nd_t = tf.gather_nd(params_t, indices)
      gather_nd_val = gather_nd_t.eval()

    self.assertEqual([2, 6, 2, 2], gather_nd_t.get_shape())
    self.assertAllEqual(
        np.vstack((params[np.newaxis, :], params[np.newaxis, :])),
        gather_nd_val)

  def testHigherRankParamsAndIndicesLargerThanIndexSlices(self):
    with self.test_session(use_gpu=self.use_gpu):
      params = np.array(
          [[[-8, -1, -2, -3, -7, -5], [8, 1, 2, 3, 7, 5]],
           [[-80, -10, -20, -30, -70, -50], [80, 10, 20, 30, 70, 50]]],
          dtype=np.float32).T
      params_t = tf.constant(params)
      indices = tf.constant([[[3], [2], [1]], [[4], [4], [0]]])
      gather_nd_t = tf.gather_nd(params_t, indices)
      gather_nd_val = gather_nd_t.eval()

    self.assertEqual([2, 3, 2, 2], gather_nd_t.get_shape())
    self.assertAllEqual(params[[3, 2, 1, 4, 4, 0]].reshape(2, 3, 2, 2),
                        gather_nd_val)

  def testHigherRankParams(self):
    with self.test_session(use_gpu=self.use_gpu):
      shape = (10, 20, 5, 1, 17)
      params = np.random.rand(*shape)
      indices = np.vstack([
          np.random.randint(0, s, size=2000) for s in shape]).T
      gather_nd_t = tf.gather_nd(params, indices)
      gather_nd_val = gather_nd_t.eval()

    expected = params[tuple(indices.T)]
    self.assertAllEqual(expected, gather_nd_val)
    self.assertEqual([2000], gather_nd_t.get_shape())

  def testHigherRankParamsAndIndices(self):
    with self.test_session(use_gpu=self.use_gpu):
      shape = (10, 20, 5, 1, 17)
      params = np.random.rand(*shape)
      indices = np.vstack([
          np.random.randint(0, s, size=2000) for s in shape]).T
      indices_reshaped = indices.reshape([10, 10, 20, 5])
      gather_nd_t = tf.gather_nd(params, indices_reshaped)
      gather_nd_val = gather_nd_t.eval()

    expected = params[tuple(indices.T)]
    self.assertAllEqual(expected.reshape([10, 10, 20]), gather_nd_val)
    self.assertEqual([10, 10, 20], gather_nd_t.get_shape())

  def testUnknownIndices(self):
    params = tf.constant([[0, 1, 2]])
    indices = tf.placeholder(tf.int32)
    gather_nd_t = tf.gather_nd(params, indices)
    shape = gather_nd_t.get_shape()
    self.assertEqual(shape.ndims, None)
    self.assertEqual(shape[0].value, None)

  def testBadIndices(self):
    with self.test_session(use_gpu=False):
      params = [0, 1, 2]
      indices = [[[0], [7]]]  # Make this one higher rank
      gather_nd = tf.gather_nd(params, indices)
      with self.assertRaisesOpError(
          r"flat indices\[1, :\] = \[7\] does not index into param "
          r"\(shape: \[3\]\)"):
        gather_nd.eval()

  def testBadIndicesWithSlices(self):
    with self.test_session(use_gpu=False):
      params = [[0, 1, 2]]
      indices = [[[0], [0], [1]]]  # Make this one higher rank
      gather_nd = tf.gather_nd(params, indices)
      with self.assertRaisesOpError(
          r"flat indices\[2, :\] = \[1\] does not index into param "
          r"\(shape: \[1,3\]\)"):
        gather_nd.eval()


class GatherNdGpuTest(GatherNdTest):
  use_gpu = True


class GatherNdOpBenchmark(tf.test.Benchmark):

  def benchmark_gather_nd_op(self):
    shape = (100, 47, 18, 170, 13)
    np.random.seed(127)
    params = np.random.rand(*shape)
    indices = np.vstack([
        np.random.randint(0, s, size=10000) for s in shape]).T

    with tf.Session():
      t_params = tf.Variable(params)
      t_indices = tf.Variable(indices)
      gather_op = tf.gather_nd(t_params, t_indices)
      tf.initialize_all_variables().run()
      for _ in range(10):
        gather_op.eval()
      t1 = time.time()
      for _ in range(1000):
        gather_op.eval()
      t2 = time.time()
      self.report_benchmark(iters=1000, wall_time=(t2-t1)/1000.0)


if __name__ == "__main__":
  tf.test.main()
