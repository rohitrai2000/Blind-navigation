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

"""Tests for tensorflow.kernels.listdiff_op."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf

_TYPES = [tf.int32, tf.int64, tf.float32, tf.float64, tf.string]


class ListDiffTest(tf.test.TestCase):

  def _testListDiff(self, x, y, out, idx):
    for dtype in _TYPES:
      if dtype == tf.string:
        x = [tf.compat.as_bytes(str(a)) for a in x]
        y = [tf.compat.as_bytes(str(a)) for a in y]
        out = [tf.compat.as_bytes(str(a)) for a in out]

      with self.test_session() as sess:
        x_tensor = tf.convert_to_tensor(x, dtype=dtype)
        y_tensor = tf.convert_to_tensor(y, dtype=dtype)
        out_tensor, idx_tensor = tf.listdiff(x_tensor, y_tensor)
        tf_out, tf_idx = sess.run([out_tensor, idx_tensor])

      self.assertAllEqual(tf_out, out)
      self.assertAllEqual(tf_idx, idx)
      self.assertEqual(1, out_tensor.get_shape().ndims)
      self.assertEqual(1, idx_tensor.get_shape().ndims)

  def testBasic1(self):
    x = [1, 2, 3, 4]
    y = [1, 2]
    out = [3, 4]
    idx = [2, 3]
    self._testListDiff(x, y, out, idx)

  def testBasic2(self):
    x = [1, 2, 3, 4]
    y = [2]
    out = [1, 3, 4]
    idx = [0, 2, 3]
    self._testListDiff(x, y, out, idx)

  def testBasic3(self):
    x = [1, 4, 3, 2]
    y = [4, 2]
    out = [1, 3]
    idx = [0, 2]
    self._testListDiff(x, y, out, idx)

  def testDuplicates(self):
    x = [1, 2, 4, 3, 2, 3, 3, 1]
    y = [4, 2]
    out = [1, 3, 3, 3, 1]
    idx = [0, 3, 5, 6, 7]
    self._testListDiff(x, y, out, idx)

  def testRandom(self):
    num_random_tests = 10
    int_low = -7
    int_high = 8
    max_size = 50
    for _ in xrange(num_random_tests):
      x_size = np.random.randint(max_size + 1)
      x = np.random.randint(int_low, int_high, size=x_size)
      y_size = np.random.randint(max_size + 1)
      y = np.random.randint(int_low, int_high, size=y_size)
      out_idx = [(entry, pos) for pos, entry in enumerate(x) if entry not in y]
      if out_idx:
        out, idx = map(list, zip(*out_idx))
      else:
        out = []
        idx = []
      self._testListDiff(list(x), list(y), out, idx)

  def testFullyOverlapping(self):
    x = [1, 2, 3, 4]
    y = [1, 2, 3, 4]
    out = []
    idx = []
    self._testListDiff(x, y, out, idx)

  def testNonOverlapping(self):
    x = [1, 2, 3, 4]
    y = [5, 6]
    out = x
    idx = np.arange(len(x))
    self._testListDiff(x, y, out, idx)

  def testEmptyX(self):
    x = []
    y = [1, 2]
    out = []
    idx = []
    self._testListDiff(x, y, out, idx)

  def testEmptyY(self):
    x = [1, 2, 3, 4]
    y = []
    out = x
    idx = np.arange(len(x))
    self._testListDiff(x, y, out, idx)

  def testEmptyXY(self):
    x = []
    y = []
    out = []
    idx = []
    self._testListDiff(x, y, out, idx)

if __name__ == "__main__":
  tf.test.main()
