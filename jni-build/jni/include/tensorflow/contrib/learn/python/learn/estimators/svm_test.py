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
"""Tests for estimators.SVM."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import tensorflow as tf


class SVMTest(tf.test.TestCase):

  def testRealValuedFeaturesPerfectlySeparable(self):
    """Tests SVM classifier with real valued features."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'feature1': tf.constant([[0.0], [1.0], [3.0]]),
          'feature2': tf.constant([[1.0], [-1.2], [1.0]]),
      }, tf.constant([[1], [0], [1]])

    feature1 = tf.contrib.layers.real_valued_column('feature1')
    feature2 = tf.contrib.layers.real_valued_column('feature2')
    svm_classifier = tf.contrib.learn.SVM(feature_columns=[feature1, feature2],
                                          example_id_column='example_id',
                                          l1_regularization=0.0,
                                          l2_regularization=0.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    metrics = svm_classifier.evaluate(input_fn=input_fn, steps=1)
    loss = metrics['loss']
    accuracy = metrics['accuracy']
    # The points are not only separable but there exist weights (for instance
    # w1=0.0, w2=1.0) that satisfy the margin inequalities (y_i* w^T*x_i >=1).
    # The unregularized loss should therefore be 0.0.
    self.assertAlmostEqual(loss, 0.0, places=3)
    self.assertAlmostEqual(accuracy, 1.0, places=3)

  def testRealValuedFeaturesWithL2Regularization(self):
    """Tests SVM classifier with real valued features and L2 regularization."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'feature1': tf.constant([[0.5], [1.0], [1.0]]),
          'feature2': tf.constant([[1.0], [-1.0], [0.5]]),
      }, tf.constant([[1], [0], [1]])

    feature1 = tf.contrib.layers.real_valued_column('feature1')
    feature2 = tf.contrib.layers.real_valued_column('feature2')
    svm_classifier = tf.contrib.learn.SVM(feature_columns=[feature1, feature2],
                                          example_id_column='example_id',
                                          l1_regularization=0.0,
                                          l2_regularization=1.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    metrics = svm_classifier.evaluate(input_fn=input_fn, steps=1)
    loss = metrics['loss']
    accuracy = metrics['accuracy']
    # The points are in general separable. Also, if there was no regularization,
    # the margin inequalities would be satisfied too (for instance by w1=1.0,
    # w2=5.0). Due to regularization, smaller weights are chosen. This results
    # to a small but non-zero uneregularized loss. Still, all the predictions
    # will be correct resulting to perfect accuracy.
    self.assertGreater(loss, 0.01)
    self.assertLess(loss, 0.1)
    self.assertAlmostEqual(accuracy, 1.0, places=3)

  def testRealValuedFeaturesWithMildL1Regularization(self):
    """Tests SVM classifier with real valued features and L2 regularization."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'feature1': tf.constant([[0.5], [1.0], [1.0]]),
          'feature2': tf.constant([[1.0], [-1.0], [0.5]]),
      }, tf.constant([[1], [0], [1]])

    feature1 = tf.contrib.layers.real_valued_column('feature1')
    feature2 = tf.contrib.layers.real_valued_column('feature2')
    svm_classifier = tf.contrib.learn.SVM(feature_columns=[feature1, feature2],
                                          example_id_column='example_id',
                                          l1_regularization=0.5,
                                          l2_regularization=1.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    metrics = svm_classifier.evaluate(input_fn=input_fn, steps=1)
    loss = metrics['loss']
    accuracy = metrics['accuracy']

    # Adding small L1 regularization favors even smaller weights. This results
    # to somewhat moderate unregularized loss (bigger than the one when there is
    # no L1 regularization. Still, since L1 is small, all the predictions will
    # be correct resulting to perfect accuracy.
    self.assertGreater(loss, 0.1)
    self.assertAlmostEqual(accuracy, 1.0, places=3)

  def testRealValuedFeaturesWithBigL1Regularization(self):
    """Tests SVM classifier with real valued features and L2 regularization."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'feature1': tf.constant([[0.5], [1.0], [1.0]]),
          'feature2': tf.constant([[1.0], [-1.0], [0.5]]),
      }, tf.constant([[1], [0], [1]])

    feature1 = tf.contrib.layers.real_valued_column('feature1')
    feature2 = tf.contrib.layers.real_valued_column('feature2')
    svm_classifier = tf.contrib.learn.SVM(feature_columns=[feature1, feature2],
                                          example_id_column='example_id',
                                          l1_regularization=3.0,
                                          l2_regularization=1.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    metrics = svm_classifier.evaluate(input_fn=input_fn, steps=1)
    loss = metrics['loss']
    accuracy = metrics['accuracy']

    # When L1 regularization parameter is large, the loss due to regularization
    # outweights the unregularized loss. In this case, the classifier will favor
    # very small weights (in current case 0) resulting both big unregularized
    # loss and bad accuracy.
    self.assertAlmostEqual(loss, 1.0, places=3)
    self.assertAlmostEqual(accuracy, 1 / 3, places=3)

  def testSparseFeatures(self):
    """Tests SVM classifier with (hashed) sparse features."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'price': tf.constant([[0.8], [0.6], [0.3]]),
          'country': tf.SparseTensor(values=['IT', 'US', 'GB'],
                                     indices=[[0, 0], [1, 0], [2, 0]],
                                     shape=[3, 1]),
      }, tf.constant([[0], [1], [1]])

    price = tf.contrib.layers.real_valued_column('price')
    country = tf.contrib.layers.sparse_column_with_hash_bucket(
        'country', hash_bucket_size=5)
    svm_classifier = tf.contrib.learn.SVM(feature_columns=[price, country],
                                          example_id_column='example_id',
                                          l1_regularization=0.0,
                                          l2_regularization=1.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    accuracy = svm_classifier.evaluate(input_fn=input_fn, steps=1)['accuracy']
    self.assertAlmostEqual(accuracy, 1.0, places=3)

  def testBucketizedFeatures(self):
    """Tests SVM classifier with bucketized features."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'price': tf.constant([[600.0], [800.0], [400.0]]),
          'sq_footage': tf.constant([[1000.0], [800.0], [500.0]]),
          'weights': tf.constant([[1.0], [1.0], [1.0]])
      }, tf.constant([[1], [0], [1]])

    price_bucket = tf.contrib.layers.bucketized_column(
        tf.contrib.layers.real_valued_column('price'),
        boundaries=[500.0, 700.0])
    sq_footage_bucket = tf.contrib.layers.bucketized_column(
        tf.contrib.layers.real_valued_column('sq_footage'),
        boundaries=[650.0])

    svm_classifier = tf.contrib.learn.SVM(
        feature_columns=[price_bucket, sq_footage_bucket],
        example_id_column='example_id',
        l1_regularization=0.1,
        l2_regularization=1.0)
    svm_classifier.fit(input_fn=input_fn, steps=30)
    accuracy = svm_classifier.evaluate(input_fn=input_fn, steps=1)['accuracy']
    self.assertAlmostEqual(accuracy, 1.0, places=3)

  def testMixedFeatures(self):
    """Tests SVM classifier with a mix of features."""

    def input_fn():
      return {
          'example_id': tf.constant(['1', '2', '3']),
          'price': tf.constant([[0.6], [0.8], [0.3]]),
          'sq_footage': tf.constant([[900.0], [700.0], [600.0]]),
          'country': tf.SparseTensor(values=['IT', 'US', 'GB'],
                                     indices=[[0, 0], [1, 3], [2, 1]],
                                     shape=[3, 5]),
          'weights': tf.constant([[3.0], [1.0], [1.0]])
      }, tf.constant([[1], [0], [1]])

    price = tf.contrib.layers.real_valued_column('price')
    sq_footage_bucket = tf.contrib.layers.bucketized_column(
        tf.contrib.layers.real_valued_column('sq_footage'),
        boundaries=[650.0, 800.0])
    country = tf.contrib.layers.sparse_column_with_hash_bucket(
        'country', hash_bucket_size=5)
    sq_footage_country = tf.contrib.layers.crossed_column(
        [sq_footage_bucket, country],
        hash_bucket_size=10)
    svm_classifier = tf.contrib.learn.SVM(
        feature_columns=[price, sq_footage_bucket, country, sq_footage_country],
        example_id_column='example_id',
        weight_column_name='weights',
        l1_regularization=0.1,
        l2_regularization=1.0)

    svm_classifier.fit(input_fn=input_fn, steps=30)
    accuracy = svm_classifier.evaluate(input_fn=input_fn, steps=1)['accuracy']
    self.assertAlmostEqual(accuracy, 1.0, places=3)


if __name__ == '__main__':
  tf.test.main()
