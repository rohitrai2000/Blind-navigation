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

"""Tests for tensorflow.ops.random_ops."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf


class RandomNormalTest(tf.test.TestCase):

  def _Sampler(self, num, mu, sigma, dtype, use_gpu, seed=None):
    def func():
      with self.test_session(use_gpu=use_gpu, graph=tf.Graph()) as sess:
        rng = tf.random_normal(
            [num], mean=mu, stddev=sigma, dtype=dtype, seed=seed)
        ret = np.empty([10, num])
        for i in xrange(10):
          ret[i, :] = sess.run(rng)
      return ret
    return func

  # Asserts that different trials (1000 samples per trial) is unlikely
  # to see the same sequence of values. Will catch buggy
  # implementations which uses the same random number seed.
  def testDistinct(self):
    for use_gpu in [False, True]:
      for dt in tf.float16, tf.float32, tf.float64:
        sampler = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu)
        x = sampler()
        y = sampler()
        # Number of different samples.
        count = (x == y).sum()
        if count >= 10:
          print("x = ", x)
          print("y = ", y)
          print("count = ", count)
        self.assertTrue(count < 10)

  # Checks that the CPU and GPU implementation returns the same results,
  # given the same random seed
  def testCPUGPUMatch(self):
    for dt in tf.float16, tf.float32, tf.float64:
      results = {}
      for use_gpu in [False, True]:
        sampler = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu, seed=12345)
        results[use_gpu] = sampler()
      if dt == tf.float16:
        self.assertAllClose(results[False], results[True], rtol=1e-3, atol=1e-3)
      else:
        self.assertAllClose(results[False], results[True], rtol=1e-6, atol=1e-6)

  def testSeed(self):
    for use_gpu in [False, True]:
      for dt in tf.float16, tf.float32, tf.float64:
        sx = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu, seed=345)
        sy = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu, seed=345)
        self.assertAllEqual(sx(), sy())

  def testNoCSE(self):
    for use_gpu in [False, True]:
      with self.test_session(use_gpu=use_gpu):
        shape = [2, 3, 4]
        rnd1 = tf.random_normal(shape, 0.0, 1.0, tf.float32)
        rnd2 = tf.random_normal(shape, 0.0, 1.0, tf.float32)
        diff = rnd2 - rnd1
        self.assertTrue(np.linalg.norm(diff.eval()) > 0.1)


class TruncatedNormalTest(tf.test.TestCase):

  def _Sampler(self, num, mu, sigma, dtype, use_gpu, seed=None):
    def func():
      with self.test_session(use_gpu=use_gpu, graph=tf.Graph()) as sess:
        rng = tf.truncated_normal(
            [num], mean=mu, stddev=sigma, dtype=dtype, seed=seed)
        ret = np.empty([10, num])
        for i in xrange(10):
          ret[i, :] = sess.run(rng)
      return ret
    return func

  # Asserts that different trials (1000 samples per trial) is unlikely
  # to see the same sequence of values. Will catch buggy
  # implementations which uses the same random number seed.
  def testDistinct(self):
    # NOTE: TruncatedNormal on GPU is not supported.
    for use_gpu in [False]:
      for dt in tf.float16, tf.float32, tf.float64:
        sampler = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu)
        x = sampler()
        y = sampler()
        # Number of different samples.
        count = (x == y).sum()
        if count >= 10:
          print("x = ", x)
          print("y = ", y)
          print("count = ", count)
        self.assertTrue(count < 10)

  # Checks that the CPU and GPU implementation returns the same results,
  # given the same random seed
  def testCPUGPUMatch(self):
    for dt in tf.float16, tf.float32, tf.float64:
      results = {}
      for use_gpu in [False, True]:
        # We need a particular larger number of samples to test multiple rounds
        # on GPU
        sampler = self._Sampler(1000000, 0.0, 1.0, dt, use_gpu=use_gpu,
                                seed=12345)
        results[use_gpu] = sampler()
      if dt == tf.float16:
        self.assertAllClose(results[False], results[True], rtol=1e-3, atol=1e-3)
      else:
        self.assertAllClose(results[False], results[True], rtol=1e-6, atol=1e-6)

  def testSeed(self):
    for use_gpu in [False, True]:
      for dt in tf.float16, tf.float32, tf.float64:
        sx = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu, seed=345)
        sy = self._Sampler(1000, 0.0, 1.0, dt, use_gpu=use_gpu, seed=345)
        self.assertAllEqual(sx(), sy())

  # The effective standard deviation of truncated normal is 85% of the
  # requested one.
  def testStdDev(self):
    for use_gpu in [False, True]:
      for dt in tf.float16, tf.float32, tf.float64:
        stddev = 3.0
        sampler = self._Sampler(100000, 0.0, stddev, dt, use_gpu=use_gpu)
        x = sampler()
        print("std(x)", np.std(x), abs(np.std(x) / stddev - 0.85))
        self.assertTrue(abs(np.std(x) / stddev - 0.85) < 0.04)

  def testNoCSE(self):
    for use_gpu in [False, True]:
      with self.test_session(use_gpu=use_gpu):
        shape = [2, 3, 4]
        rnd1 = tf.truncated_normal(shape, 0.0, 1.0, tf.float32)
        rnd2 = tf.truncated_normal(shape, 0.0, 1.0, tf.float32)
        diff = rnd2 - rnd1
        self.assertTrue(np.linalg.norm(diff.eval()) > 0.1)


class RandomUniformTest(tf.test.TestCase):

  def _Sampler(self, num, minv, maxv, dtype, use_gpu, seed=None):
    def func():
      with self.test_session(use_gpu=use_gpu, graph=tf.Graph()) as sess:
        rng = tf.random_uniform(
            [num], minval=minv, maxval=maxv, dtype=dtype, seed=seed)
        ret = np.empty([10, num])
        for i in xrange(10):
          ret[i, :] = sess.run(rng)
      return ret
    return func

  def testRange(self):
    for use_gpu in False, True:
      for dt in tf.float16, tf.float32, tf.float64, tf.int32, tf.int64:
        sampler = self._Sampler(1000, minv=-2, maxv=8, dtype=dt,
                                use_gpu=use_gpu)
        x = sampler()
        self.assertTrue(-2 <= np.min(x))
        self.assertTrue(np.max(x) < 8)

  # Asserts that different trials (1000 samples per trial) is unlikely
  # to see the same sequence of values. Will catch buggy
  # implementations which uses the same random number seed.
  def testDistinct(self):
    for use_gpu in False, True:
      for dt in tf.float16, tf.float32, tf.float64, tf.int32, tf.int64:
        maxv = 1.0 if dt.is_floating else 1 << 30
        sampler = self._Sampler(1000, minv=0, maxv=maxv, dtype=dt,
                                use_gpu=use_gpu)
        x = sampler()
        y = sampler()
        count = (x == y).sum()
        count_limit = 50 if dt == tf.float16 else 10
        if count >= count_limit:
          print("x = ", x)
          print("y = ", y)
          print("count = ", count)
        self.assertTrue(count < count_limit)

  # Check that uniform ints actually follow a uniform distribution.
  def testUniformInts(self):
    minv = -2
    maxv = 15
    n = 100000
    p = 1 / (maxv - minv)
    # The counts should follow an (n, p) binomial distribution.
    mean = p * n
    std = np.sqrt(n * p * (1 - p))
    for use_gpu in False, True:
      for dt in tf.int32, tf.int64:
        # Use a fixed seed here to make the test deterministic.
        # Without the fixed seed, the 5 * std bound will (very rarely) fail.
        sampler = self._Sampler(n // 10, minv=minv, maxv=maxv, dtype=dt,
                                use_gpu=use_gpu, seed=17)
        x = sampler().ravel()
        self.assertEqual(x.shape, (n,))
        counts, _ = np.histogram(x, bins=maxv - minv)
        self.assertEqual(counts.shape, (maxv - minv,))
        self.assertEqual(counts.sum(), n)
        error = np.abs(counts - mean)
        self.assertLess(error.max(), 5 * std)

  # Checks that the CPU and GPU implementation returns the same results,
  # given the same random seed
  def testCPUGPUMatch(self):
    for dt in tf.float16, tf.float32, tf.float64, tf.int32, tf.int64:
      maxv = 1.0 if dt.is_floating else 17
      results = {}
      for use_gpu in False, True:
        sampler = self._Sampler(1000, minv=0, maxv=maxv, dtype=dt,
                                use_gpu=use_gpu, seed=12345)
        results[use_gpu] = sampler()
      self.assertAllEqual(results[False], results[True])

  def testSeed(self):
    for use_gpu in False, True:
      for dt in tf.float16, tf.float32, tf.float64, tf.int32, tf.int64:
          for seed in [345, 2**100, -2**100]:
            sx = self._Sampler(1000, 0, 17, dtype=dt, use_gpu=use_gpu, seed=seed)
            sy = self._Sampler(1000, 0, 17, dtype=dt, use_gpu=use_gpu, seed=seed)
            self.assertAllEqual(sx(), sy())

  def testNoCSE(self):
    shape = [2, 3, 4]
    for use_gpu in False, True:
      for dtype in tf.float16, tf.float32, tf.int32:
        with self.test_session(use_gpu=use_gpu):
          rnd1 = tf.random_uniform(shape, 0, 17, dtype=dtype)
          rnd2 = tf.random_uniform(shape, 0, 17, dtype=dtype)
          diff = (rnd2 - rnd1).eval()
          self.assertTrue(np.linalg.norm(diff) > 0.1)


class RandomShapeTest(tf.test.TestCase):

  def testTruncatedNormal(self):
    # Fully known shape.
    rnd1 = tf.truncated_normal([1, 2, 3])
    self.assertEqual([1, 2, 3], rnd1.get_shape())
    # Partially known shape.
    rnd2 = tf.truncated_normal(tf.placeholder(tf.int32, shape=(3,)))
    self.assertEqual([None, None, None], rnd2.get_shape().as_list())
    # Unknown shape.
    rnd3 = tf.truncated_normal(tf.placeholder(tf.int32))
    self.assertIs(None, rnd3.get_shape().ndims)

  def testRandomNormal(self):
    # Fully known shape.
    rnd1 = tf.random_normal([1, 2, 3])
    self.assertEqual([1, 2, 3], rnd1.get_shape())
    # Partially known shape.
    rnd2 = tf.random_normal(tf.placeholder(tf.int32, shape=(3,)))
    self.assertEqual([None, None, None], rnd2.get_shape().as_list())
    # Unknown shape.
    rnd3 = tf.random_normal(tf.placeholder(tf.int32))
    self.assertIs(None, rnd3.get_shape().ndims)

  def testRandomUniform(self):
    # Fully known shape.
    rnd1 = tf.random_uniform([1, 2, 3])
    self.assertEqual([1, 2, 3], rnd1.get_shape())
    # Partially known shape.
    rnd2 = tf.random_uniform(
        tf.placeholder(tf.int32, shape=(3,)))
    self.assertEqual([None, None, None], rnd2.get_shape().as_list())
    # Unknown shape.
    rnd3 = tf.random_uniform(tf.placeholder(tf.int32))
    self.assertIs(None, rnd3.get_shape().ndims)


if __name__ == "__main__":
  tf.test.main()
