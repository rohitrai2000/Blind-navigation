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

"""Tests for summary image op."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf

from tensorflow.python.ops import image_ops


class SummaryImageOpTest(tf.test.TestCase):

  def _AsSummary(self, s):
    summ = tf.Summary()
    summ.ParseFromString(s)
    return summ

  def _CheckProto(self, image_summ, shape):
    """Verify that the non-image parts of the image_summ proto match shape."""
    # Only the first 3 images are returned.
    for v in image_summ.value:
      v.image.ClearField("encoded_image_string")
    expected = '\n'.join("""
        value {
          tag: "img/image/%d"
          image { height: %d width: %d colorspace: %d }
        }""" % ((i,) + shape[1:]) for i in xrange(3))
    self.assertProtoEquals(expected, image_summ)

  def testImageSummary(self):
    np.random.seed(7)
    with self.test_session() as sess:
      for depth in 1, 3, 4:
        shape = (4, 5, 7) + (depth,)
        bad_color = [255, 0, 0, 255][:depth]
        for positive in False, True:
          # Build a mostly random image with one nan
          const = np.random.randn(*shape).astype(np.float32)
          const[0, 1, 2] = 0  # Make the nan entry not the max
          if positive:
            const = 1 + np.maximum(const, 0)
            scale = 255 / const.reshape(4, -1).max(axis=1)
            offset = 0
          else:
            scale = 127 / np.abs(const.reshape(4, -1)).max(axis=1)
            offset = 128
          adjusted = np.floor(scale[:, None, None, None] * const + offset)
          const[0, 1, 2, depth // 2] = np.nan

          # Summarize
          summ = tf.image_summary("img", const)
          value = sess.run(summ)
          self.assertEqual([], summ.get_shape())
          image_summ = self._AsSummary(value)

          # Decode the first image and check consistency
          image = image_ops.decode_png(
              image_summ.value[0].image.encoded_image_string).eval()
          self.assertAllEqual(image[1, 2], bad_color)
          image[1, 2] = adjusted[0, 1, 2]
          self.assertAllClose(image, adjusted[0])

          # Check the rest of the proto
          self._CheckProto(image_summ, shape)

  def testImageSummaryUint8(self):
    np.random.seed(7)
    with self.test_session() as sess:
      for depth in 1, 3, 4:
        shape = (4, 5, 7) + (depth,)

        # Build a random uint8 image
        images = np.random.randint(256, size=shape).astype(np.uint8)
        tf_images = tf.convert_to_tensor(images)
        self.assertEqual(tf_images.dtype, tf.uint8)

        # Summarize
        summ = tf.image_summary("img", tf_images)
        value = sess.run(summ)
        self.assertEqual([], summ.get_shape())
        image_summ = self._AsSummary(value)

        # Decode the first image and check consistency.
        # Since we're uint8, everything should be exact.
        image = image_ops.decode_png(
            image_summ.value[0].image.encoded_image_string).eval()
        self.assertAllEqual(image, images[0])

        # Check the rest of the proto
        self._CheckProto(image_summ, shape)


if __name__ == "__main__":
  tf.test.main()
