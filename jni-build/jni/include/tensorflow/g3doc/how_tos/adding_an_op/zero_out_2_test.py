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

"""Test for version 2 of the zero_out op."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf
from tensorflow.g3doc.how_tos.adding_an_op import zero_out_op_2
from tensorflow.g3doc.how_tos.adding_an_op import zero_out_grad_2


class ZeroOut2Test(tf.test.TestCase):

  def test(self):
    with self.test_session():
      result = zero_out_op_2.zero_out([5, 4, 3, 2, 1])
      self.assertAllEqual(result.eval(), [5, 0, 0, 0, 0])

  def test_grad(self):
    with self.test_session():
      shape = (5,)
      x = tf.constant([5, 4, 3, 2, 1], dtype=tf.float32)
      y = zero_out_op_2.zero_out(x)
      err = tf.test.compute_gradient_error(x, shape, y, shape)
      self.assertLess(err, 1e-4)


if __name__ == '__main__':
  tf.test.main()
