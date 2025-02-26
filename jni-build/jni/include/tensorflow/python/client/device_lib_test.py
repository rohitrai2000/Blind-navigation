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

"""Tests for the SWIG-wrapped device lib."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf

from tensorflow.python.client import device_lib
from tensorflow.python.framework import test_util
from tensorflow.python.platform import googletest


class DeviceLibTest(test_util.TensorFlowTestCase):

  # TODO(ebrevdo): fix python3 compatibility: b/27727661
  def _testListLocalDevices(self):
    devices = device_lib.list_local_devices()
    self.assertGreater(len(devices), 0)
    self.assertEqual(devices[0].device_type, "CPU")

    # GPU test
    if tf.test.is_built_with_cuda():
      self.assertGreater(len(devices), 1)
      self.assertTrue("GPU" in [d.device_type for d in devices])


if __name__ == "__main__":
  googletest.main()
