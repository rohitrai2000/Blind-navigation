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
"""Functional tests for ExtractImagePatches op."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class ExtractImagePatches(tf.test.TestCase):
  """Functional tests for ExtractImagePatches op."""

  def _VerifyValues(self, image, ksizes, strides, rates, padding, patches):
    """Tests input-output pairs for the ExtractImagePatches op.

    Args:
      image: Input tensor with shape: [batch, in_rows, in_cols, depth].
      ksizes: Patch size specified as: [ksize_rows, ksize_cols].
      strides: Output strides, specified as [stride_rows, stride_cols].
      rates: Atrous rates, specified as [rate_rows, rate_cols].
      padding: Padding type.
      patches: Expected output.
    """
    ksizes = [1] + ksizes + [1]
    strides = [1] + strides + [1]
    rates = [1] + rates + [1]

    with self.test_session():
      out_tensor = tf.extract_image_patches(
          tf.constant(image),
          ksizes=ksizes,
          strides=strides,
          rates=rates,
          padding=padding,
          name="im2col")
      self.assertAllClose(patches, out_tensor.eval())

  def testKsize1x1Stride1x1Rate1x1(self):
    """Verifies that for 1x1 kernel the output equals the input."""
    # [2, 3, 4, 5]
    image = np.reshape(range(120), [2, 3, 4, 5])
    # [2, 3, 4, 5]
    patches = np.reshape(range(120), [2, 3, 4, 5])
    for padding in ["VALID", "SAME"]:
      self._VerifyValues(image,
                         ksizes=[1, 1],
                         strides=[1, 1],
                         rates=[1, 1],
                         padding=padding,
                         patches=patches)

  def testKsize1x1Stride2x3Rate1x1(self):
    """Test for 1x1 kernel and strides."""
    # [2, 4, 5, 3]
    image = np.reshape(range(120), [2, 4, 5, 3])
    # [2, 2, 2, 3]
    patches = image[:, ::2, ::3, :]
    for padding in ["VALID", "SAME"]:
      self._VerifyValues(image,
                         ksizes=[1, 1],
                         strides=[2, 3],
                         rates=[1, 1],
                         padding=padding,
                         patches=patches)

  def testKsize2x2Stride1x1Rate1x1Valid(self):
    """Test for 1x1 kernel ."""
    # [1, 2, 2, 1]
    image = [[[[1], [2]], [[3], [4]]]]
    # [1, 1, 1, 4]
    patches = [[[[1, 2, 3, 4]]]]
    self._VerifyValues(image,
                       ksizes=[2, 2],
                       strides=[1, 1],
                       rates=[1, 1],
                       padding="VALID",
                       patches=patches)

  def testKsize2x2Stride1x1Rate1x1Same(self):
    """Test for 1x1 kernel ."""
    # [1, 2, 2, 1]
    image = [[[[1], [2]], [[3], [4]]]]
    # [1, 2, 2, 4]
    patches = [[[[1, 2, 3, 4], [2, 0, 4, 0]], [[3, 4, 0, 0], [4, 0, 0, 0]]]]
    self._VerifyValues(image,
                       ksizes=[2, 2],
                       strides=[1, 1],
                       rates=[1, 1],
                       padding="SAME",
                       patches=patches)


if __name__ == "__main__":
  tf.test.main()
