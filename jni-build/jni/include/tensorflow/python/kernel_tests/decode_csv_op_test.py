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

"""Tests for DecodeCSV op from parsing_ops."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class DecodeCSVOpTest(tf.test.TestCase):

  def _test(self, args, expected_out=None, expected_err_re=None):
    with self.test_session() as sess:
      decode = tf.decode_csv(**args)

      if expected_err_re is None:
        out = sess.run(decode)

        for i, field in enumerate(out):
          if field.dtype == np.float32:
            self.assertAllClose(field, expected_out[i])
          else:
            self.assertAllEqual(field, expected_out[i])

      else:
        with self.assertRaisesOpError(expected_err_re):
          sess.run(decode)

  def testSimple(self):
    args = {"records": ["1", "2", '"3"'], "record_defaults": [[1]],}

    expected_out = [[1, 2, 3]]

    self._test(args, expected_out)

  def testScalar(self):
    args = {"records": '1,""', "record_defaults": [[3], [4]]}

    expected_out = [1, 4]

    self._test(args, expected_out)

  def test2D(self):
    args = {"records": [["1", "2"], ['""', "4"]], "record_defaults": [[5]]}
    expected_out = [[[1, 2], [5, 4]]]

    self._test(args, expected_out)

  def testInt64(self):
    args = {
        "records": ["1", "2", '"2147483648"'],
        "record_defaults": [np.array([],
                                     dtype=np.int64)],
    }

    expected_out = [[1, 2, 2147483648]]

    self._test(args, expected_out)

  def testComplexString(self):
    args = {
        "records": ['"1.0"', '"ab , c"', '"a\nbc"', '"ab""c"', " abc "],
        "record_defaults": [["1"]]
    }

    expected_out = [[b"1.0", b"ab , c", b"a\nbc", b'ab"c', b" abc "]]

    self._test(args, expected_out)

  def testMultiRecords(self):
    args = {
        "records": ["1.0,4,aa", "0.2,5,bb", "3,6,cc"],
        "record_defaults": [[1.0], [1], ["aa"]]
    }

    expected_out = [[1.0, 0.2, 3], [4, 5, 6], [b"aa", b"bb", b"cc"]]

    self._test(args, expected_out)

  def testWithDefaults(self):
    args = {
        "records": [",1,", "0.2,3,bcd", "3.0,,"],
        "record_defaults": [[1.0], [0], ["a"]]
    }

    expected_out = [[1.0, 0.2, 3.0], [1, 3, 0], [b"a", b"bcd", b"a"]]

    self._test(args, expected_out)

  def testWithTabDelim(self):
    args = {
        "records": ["1\t1", "0.2\t3", "3.0\t"],
        "record_defaults": [[1.0], [0]],
        "field_delim": "\t"
    }

    expected_out = [[1.0, 0.2, 3.0], [1, 3, 0]]

    self._test(args, expected_out)

  def testWithoutDefaultsError(self):
    args = {
        "records": [",1", "0.2,3", "3.0,"],
        "record_defaults": [[1.0], np.array([],
                                            dtype=np.int32)]
    }

    self._test(args,
               expected_err_re="Field 1 is required but missing in record 2!")

  def testWrongFieldIntError(self):
    args = {
        "records": [",1", "0.2,234a", "3.0,2"],
        "record_defaults": [[1.0], np.array([],
                                            dtype=np.int32)]
    }

    self._test(args,
               expected_err_re="Field 1 in record 1 is not a valid int32: 234a")

  def testOutOfRangeError(self):
    args = {
        "records": ["1", "9999999999999999999999999", "3"],
        "record_defaults": [[1]]
    }

    self._test(args,
               expected_err_re="Field 0 in record 1 is not a valid int32: ")

  def testWrongFieldFloatError(self):
    args = {
        "records": [",1", "0.2,2", "3.0adf,3"],
        "record_defaults": [[1.0], np.array([],
                                            dtype=np.int32)]
    }

    self._test(args,
               expected_err_re="Field 0 in record 2 is not a valid float: ")

  def testWrongFieldStringError(self):
    args = {"records": ['"1,a,"', "0.22", 'a"bc'], "record_defaults": [["a"]]}

    self._test(
        args,
        expected_err_re="Unquoted fields cannot have quotes/CRLFs inside")

  def testWrongDefaults(self):
    args = {
        "records": [",1", "0.2,2", "3.0adf,3"],
        "record_defaults": [[1.0]]
    }

    self._test(args,
               expected_err_re="Expect 1 fields but have 2 in record 0")

  def testShortQuotedString(self):
    args = {"records": ["\""], "record_defaults": [["default"]],}

    self._test(args,
               expected_err_re="Quoted field has to end with quote followed.*")


if __name__ == "__main__":
  tf.test.main()
