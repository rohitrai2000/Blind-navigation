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
"""Tests of the DataFrame class."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf

from tensorflow.contrib.layers import feature_column
from tensorflow.contrib.learn.python import learn
from tensorflow.contrib.learn.python.learn.dataframe import estimator_utils
from tensorflow.contrib.learn.python.learn.tests.dataframe import mocks
from tensorflow.python.framework import tensor_shape


def setup_test_df():
  """Create a dataframe populated with some test columns."""
  df = learn.DataFrame()
  df["a"] = learn.TransformedSeries(
      [mocks.MockSeries("foobar", mocks.MockTensor("Tensor a", tf.int32))],
      mocks.MockTwoOutputTransform("iue", "eui", "snt"), "out1")
  df["b"] = learn.TransformedSeries(
      [mocks.MockSeries("foobar", mocks.MockTensor("Tensor b", tf.int32))],
      mocks.MockTwoOutputTransform("iue", "eui", "snt"), "out2")
  df["c"] = learn.TransformedSeries(
      [mocks.MockSeries("foobar", mocks.MockTensor("Tensor c", tf.int32))],
      mocks.MockTwoOutputTransform("iue", "eui", "snt"), "out1")
  return df


def setup_test_df_3layer():
  """Create a dataframe populated with some test columns."""
  df = learn.DataFrame()

  df["a"] = mocks.MockSeries("a_series", mocks.MockTensor("Tensor a", tf.int32))
  df["b"] = mocks.MockSeries("b_series",
                             mocks.MockSparseTensor("SparseTensor b", tf.int32))
  df["c"] = mocks.MockSeries("c_series", mocks.MockTensor("Tensor c", tf.int32))
  df["d"] = mocks.MockSeries("d_series",
                             mocks.MockSparseTensor("SparseTensor d", tf.int32))

  df["e"] = learn.TransformedSeries([df["a"], df["b"]],
                                    mocks.Mock2x2Transform("iue", "eui", "snt"),
                                    "out1")
  df["f"] = learn.TransformedSeries([df["c"], df["d"]],
                                    mocks.Mock2x2Transform("iue", "eui", "snt"),
                                    "out2")
  df["g"] = learn.TransformedSeries([df["e"], df["f"]],
                                    mocks.Mock2x2Transform("iue", "eui", "snt"),
                                    "out1")
  return df


class EstimatorUtilsTest(tf.test.TestCase):
  """Test of estimator utils."""

  def test_to_feature_columns_and_input_fn(self):
    df = setup_test_df_3layer()
    feature_columns, input_fn = (
        estimator_utils.to_feature_columns_and_input_fn(
            df,
            base_input_keys_with_defaults={"a": 1,
                                           "b": 2,
                                           "c": 3,
                                           "d": 4},
            target_keys=["g"],
            feature_keys=["a", "b", "f"]))

    expected_feature_column_a = feature_column.DataFrameColumn(
        "a", learn.PredefinedSeries(
            "a", tf.FixedLenFeature(tensor_shape.unknown_shape(), tf.int32, 1)))
    expected_feature_column_b = feature_column.DataFrameColumn(
        "b", learn.PredefinedSeries("b", tf.VarLenFeature(tf.int32)))
    expected_feature_column_f = feature_column.DataFrameColumn(
        "f", learn.TransformedSeries([
            learn.PredefinedSeries("c", tf.FixedLenFeature(
                tensor_shape.unknown_shape(), tf.int32, 3)),
            learn.PredefinedSeries("d", tf.VarLenFeature(tf.int32))
        ], mocks.Mock2x2Transform("iue", "eui", "snt"), "out2"))

    expected_feature_columns = [expected_feature_column_a,
                                expected_feature_column_b,
                                expected_feature_column_f]
    self.assertEqual(sorted(expected_feature_columns), sorted(feature_columns))

    base_features, targets = input_fn()
    expected_base_features = {
        "a": mocks.MockTensor("Tensor a", tf.int32),
        "b": mocks.MockSparseTensor("SparseTensor b", tf.int32),
        "c": mocks.MockTensor("Tensor c", tf.int32),
        "d": mocks.MockSparseTensor("SparseTensor d", tf.int32)
    }
    self.assertEqual(expected_base_features, base_features)

    expected_targets = mocks.MockTensor("Out iue", tf.int32)
    self.assertEqual(expected_targets, targets)

    self.assertEqual(3, len(feature_columns))

  def test_to_feature_columns_and_input_fn_no_targets(self):
    df = setup_test_df_3layer()
    feature_columns, input_fn = (
        estimator_utils.to_feature_columns_and_input_fn(
            df,
            base_input_keys_with_defaults={"a": 1,
                                           "b": 2,
                                           "c": 3,
                                           "d": 4},
            feature_keys=["a", "b", "f"]))

    base_features, targets = input_fn()
    expected_base_features = {
        "a": mocks.MockTensor("Tensor a", tf.int32),
        "b": mocks.MockSparseTensor("SparseTensor b", tf.int32),
        "c": mocks.MockTensor("Tensor c", tf.int32),
        "d": mocks.MockSparseTensor("SparseTensor d", tf.int32)
    }
    self.assertEqual(expected_base_features, base_features)

    expected_targets = {}
    self.assertEqual(expected_targets, targets)

    self.assertEqual(3, len(feature_columns))

  def test_to_estimator_not_disjoint(self):
    df = setup_test_df_3layer()

    # pylint: disable=unused-variable
    def get_not_disjoint():
      feature_columns, input_fn = (
          estimator_utils.to_feature_columns_and_input_fn(
              df,
              base_input_keys_with_defaults={"a": 1,
                                             "b": 2,
                                             "c": 3,
                                             "d": 4},
              target_keys=["f"],
              feature_keys=["a", "b", "f"]))

    self.assertRaises(ValueError, get_not_disjoint)


if __name__ == "__main__":
  tf.test.main()
