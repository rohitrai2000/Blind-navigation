# Copyright 2015 Google Inc. All Rights Reserved.
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

"""Functional tests for Proximal Gradient Descent operations."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class ProximalGradientDescentOptimizerTest(tf.test.TestCase):

  def testProximalGradientDescentwithoutRegularization(self):
    with self.test_session() as sess:
      var0 = tf.Variable([0.0, 0.0])
      var1 = tf.Variable([0.0, 0.0])
      grads0 = tf.constant([0.1, 0.2])
      grads1 = tf.constant([0.01, 0.02])
      opt = tf.train.ProximalGradientDescentOptimizer(
          3.0,
          l1_regularization_strength=0.0,
          l2_regularization_strength=0.0)
      update = opt.apply_gradients(zip([grads0, grads1], [var0, var1]))
      tf.initialize_all_variables().run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose([0.0, 0.0], v0_val)
      self.assertAllClose([0.0, 0.0], v1_val)

      # Run 3 steps Proximal Gradient Descent.
      for _ in range(3):
        update.run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose(np.array([-0.9, -1.8]),
                          v0_val)
      self.assertAllClose(np.array([-0.09, -0.18]),
                          v1_val)

  def testProximalGradientDescentwithoutRegularization2(self):
    with self.test_session() as sess:
      var0 = tf.Variable([1.0, 2.0])
      var1 = tf.Variable([4.0, 3.0])
      grads0 = tf.constant([0.1, 0.2])
      grads1 = tf.constant([0.01, 0.02])

      opt = tf.train.ProximalGradientDescentOptimizer(
          3.0,
          l1_regularization_strength=0.0,
          l2_regularization_strength=0.0)
      update = opt.apply_gradients(zip([grads0, grads1], [var0, var1]))
      tf.initialize_all_variables().run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose([1.0, 2.0], v0_val)
      self.assertAllClose([4.0, 3.0], v1_val)

      # Run 3 steps Proximal Gradient Descent
      for _ in range(3):
        update.run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose(np.array([0.1, 0.2]),
                          v0_val)
      self.assertAllClose(np.array([3.91, 2.82]),
                          v1_val)

  def testProximalGradientDescentWithL1_L2(self):
    with self.test_session() as sess:
      var0 = tf.Variable([1.0, 2.0])
      var1 = tf.Variable([4.0, 3.0])
      grads0 = tf.constant([0.1, 0.2])
      grads1 = tf.constant([0.01, 0.02])

      opt = tf.train.ProximalGradientDescentOptimizer(
          3.0,
          l1_regularization_strength=0.001,
          l2_regularization_strength=2.0)
      update = opt.apply_gradients(zip([grads0, grads1], [var0, var1]))
      tf.initialize_all_variables().run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose([1.0, 2.0], v0_val)
      self.assertAllClose([4.0, 3.0], v1_val)

      # Run 10 steps Proximal Gradient Descent
      for _ in range(10):
        update.run()

      v0_val, v1_val = sess.run([var0, var1])
      self.assertAllClose(np.array([0.037125, 0.074625]),
                          v0_val)
      self.assertAllClose(np.array([0.003375, 0.007125]),
                          v1_val)

  def applyOptimizer(self, opt, steps=5, is_sparse=False):
    if is_sparse:
      var0 = tf.Variable([[1.0], [2.0]])
      var1 = tf.Variable([[3.0], [4.0]])
      grads0 = tf.IndexedSlices(tf.constant([0.1], shape=[1, 1]),
                                tf.constant([0]),
                                tf.constant([2, 1]))
      grads1 = tf.IndexedSlices(tf.constant([0.02], shape=[1, 1]),
                                tf.constant([1]),
                                tf.constant([2, 1]))
    else:
      var0 = tf.Variable([1.0, 2.0])
      var1 = tf.Variable([3.0, 4.0])
      grads0 = tf.constant([0.1, 0.2])
      grads1 = tf.constant([0.01, 0.02])

    update = opt.apply_gradients(zip([grads0, grads1], [var0, var1]))
    tf.initialize_all_variables().run()

    sess = tf.get_default_session()
    v0_val, v1_val = sess.run([var0, var1])
    if is_sparse:
      self.assertAllClose([[1.0], [2.0]], v0_val)
      self.assertAllClose([[3.0], [4.0]], v1_val)
    else:
      self.assertAllClose([1.0, 2.0], v0_val)
      self.assertAllClose([3.0, 4.0], v1_val)

    # Run ProximalAdagrad for a few steps
    for _ in range(steps):
      update.run()

    v0_val, v1_val = sess.run([var0, var1])
    return v0_val, v1_val

  def testEquivSparseGradientDescentwithoutRegularizaion(self):
    with self.test_session():
      val0, val1 = self.applyOptimizer(
          tf.train.ProximalGradientDescentOptimizer(
              3.0,
              l1_regularization_strength=0.0,
              l2_regularization_strength=0.0),
          is_sparse=True)

    with self.test_session():
      val2, val3 = self.applyOptimizer(
          tf.train.GradientDescentOptimizer(3.0), is_sparse=True)

    self.assertAllClose(val0, val2)
    self.assertAllClose(val1, val3)

  def testEquivGradientDescentwithoutRegularizaion(self):
    with self.test_session():
      val0, val1 = self.applyOptimizer(
          tf.train.ProximalGradientDescentOptimizer(
              3.0,
              l1_regularization_strength=0.0,
              l2_regularization_strength=0.0))

    with self.test_session():
      val2, val3 = self.applyOptimizer(
          tf.train.GradientDescentOptimizer(3.0))

    self.assertAllClose(val0, val2)
    self.assertAllClose(val1, val3)


if __name__ == "__main__":
  tf.test.main()
