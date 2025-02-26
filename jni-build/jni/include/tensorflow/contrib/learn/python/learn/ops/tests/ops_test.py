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
"""Ops tests."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf

from tensorflow.contrib.learn.python.learn import ops


class OpsTest(tf.test.TestCase):
  """Ops tests."""

  def test_softmax_classifier(self):
    with self.test_session() as session:
      features = tf.placeholder(tf.float32, [None, 3])
      labels = tf.placeholder(tf.float32, [None, 2])
      weights = tf.constant([[0.1, 0.1], [0.1, 0.1], [0.1, 0.1]])
      biases = tf.constant([0.2, 0.3])
      class_weight = tf.constant([0.1, 0.9])
      prediction, loss = ops.softmax_classifier(features, labels, weights,
                                                biases, class_weight)
      self.assertEqual(prediction.get_shape()[1], 2)
      self.assertEqual(loss.get_shape(), [])
      value = session.run(loss, {features: [[0.2, 0.3, 0.2]],
                                 labels: [[0, 1]]})
      self.assertAllClose(value, 0.55180627)

  def test_embedding_lookup(self):
    d_embed = 5
    n_embed = 10
    ids_shape = (2, 3, 4)
    embeds = np.random.randn(n_embed, d_embed)
    ids = np.random.randint(0, n_embed, ids_shape)
    with self.test_session():
      embed_np = embeds[ids]
      embed_tf = ops.embedding_lookup(embeds, ids).eval()
    self.assertEqual(embed_np.shape, embed_tf.shape)
    self.assertAllClose(embed_np, embed_tf)

  def test_categorical_variable(self):
    tf.set_random_seed(42)
    with self.test_session() as sess:
      cat_var_idx = tf.placeholder(tf.int64, [2, 2])
      embeddings = ops.categorical_variable(cat_var_idx,
                                            n_classes=5,
                                            embedding_size=10,
                                            name="my_cat_var")
      sess.run(tf.initialize_all_variables())
      emb1 = sess.run(embeddings,
                      feed_dict={cat_var_idx.name: [[0, 1], [2, 3]]})
      emb2 = sess.run(embeddings,
                      feed_dict={cat_var_idx.name: [[0, 2], [1, 3]]})
    self.assertEqual(emb1.shape, emb2.shape)
    self.assertAllEqual(np.transpose(emb2, axes=[1, 0, 2]), emb1)

  def test_conv2d(self):
    tf.set_random_seed(42)
    batch_size = 32
    input_shape = (10, 10)
    n_filters = 7
    filter_shape = (5, 5)
    vals = np.random.randn(batch_size, input_shape[0], input_shape[1], 1)
    with self.test_session() as sess:
      tf.add_to_collection("IS_TRAINING", True)
      tensor_in = tf.placeholder(tf.float32, [batch_size, input_shape[0],
                                              input_shape[1], 1])
      res = ops.conv2d(tensor_in, n_filters, filter_shape, batch_norm=True)
      sess.run(tf.initialize_all_variables())
      conv = sess.run(res, feed_dict={tensor_in.name: vals})
    self.assertEqual(conv.shape, (batch_size, input_shape[0], input_shape[1],
                                  n_filters))

  def test_one_hot_matrix(self):
    with self.test_session() as sess:
      tensor_in = tf.placeholder(tf.int64, [10, 2])
      one_hot_tensor = ops.one_hot_matrix(tensor_in, 3)
      res = sess.run(ops.one_hot_matrix([[0, 1], [2, 1]], 3))
    self.assertAllEqual(one_hot_tensor.get_shape(), [10, 2, 3])
    self.assertAllEqual(res, [[[1.0, 0, 0], [0, 1.0, 0]],
                              [[0, 0, 1.0], [0, 1.0, 0]]])


if __name__ == "__main__":
  tf.test.main()
