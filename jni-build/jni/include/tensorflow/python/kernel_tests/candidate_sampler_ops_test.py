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

"""Tests for CandidateSamplerOp."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np
import tensorflow as tf


class RangeSamplerOpsTest(tf.test.TestCase):

  BATCH_SIZE = 3
  NUM_TRUE = 2
  RANGE = 5
  NUM_SAMPLED = RANGE

  TRUE_LABELS = [[1, 2], [0, 4], [3, 3]]

  def testTrueCandidates(self):
    with self.test_session() as sess:
      indices = tf.constant([0, 0, 1, 1, 2, 2])
      true_candidates_vec = tf.constant([1, 2, 0, 4, 3, 3])
      true_candidates_matrix = tf.reshape(
          true_candidates_vec, [self.BATCH_SIZE, self.NUM_TRUE])
      indices_val, true_candidates_val = sess.run(
          [indices, true_candidates_matrix])

    self.assertAllEqual(indices_val, [0, 0, 1, 1, 2, 2])
    self.assertAllEqual(true_candidates_val, self.TRUE_LABELS)

  def testSampledCandidates(self):
    with self.test_session():
      true_classes = tf.constant([[1, 2], [0, 4], [3, 3]],
                                 dtype=tf.int64)
      sampled_candidates, _, _ = tf.nn.all_candidate_sampler(
          true_classes, self.NUM_TRUE, self.NUM_SAMPLED, True)
      result = sampled_candidates.eval()

    expected_ids = [0, 1, 2, 3, 4]
    self.assertAllEqual(result, expected_ids)
    self.assertEqual(sampled_candidates.get_shape(), [self.NUM_SAMPLED])

  def testTrueLogExpectedCount(self):
    with self.test_session():
      true_classes = tf.constant([[1, 2], [0, 4], [3, 3]],
                                 dtype=tf.int64)
      _, true_expected_count, _ = tf.nn.all_candidate_sampler(
          true_classes, self.NUM_TRUE, self.NUM_SAMPLED, True)
      true_log_expected_count = tf.log(true_expected_count)
      result = true_log_expected_count.eval()

    self.assertAllEqual(result, [[0.0] * self.NUM_TRUE] * self.BATCH_SIZE)
    self.assertEqual(true_expected_count.get_shape(), [self.BATCH_SIZE,
                                                       self.NUM_TRUE])
    self.assertEqual(true_log_expected_count.get_shape(), [self.BATCH_SIZE,
                                                           self.NUM_TRUE])

  def testSampledLogExpectedCount(self):
    with self.test_session():
      true_classes = tf.constant([[1, 2], [0, 4], [3, 3]],
                                 dtype=tf.int64)
      _, _, sampled_expected_count = tf.nn.all_candidate_sampler(
          true_classes, self.NUM_TRUE, self.NUM_SAMPLED, True)
      sampled_log_expected_count = tf.log(sampled_expected_count)
      result = sampled_log_expected_count.eval()

    self.assertAllEqual(result, [0.0] * self.NUM_SAMPLED)
    self.assertEqual(sampled_expected_count.get_shape(), [self.NUM_SAMPLED])
    self.assertEqual(sampled_log_expected_count.get_shape(), [self.NUM_SAMPLED])

  def testAccidentalHits(self):
    with self.test_session() as sess:
      true_classes = tf.constant([[1, 2], [0, 4], [3, 3]],
                                          dtype=tf.int64)
      sampled_candidates, _, _ = tf.nn.all_candidate_sampler(
          true_classes, self.NUM_TRUE, self.NUM_SAMPLED, True)
      accidental_hits = tf.nn.compute_accidental_hits(
          true_classes, sampled_candidates, self.NUM_TRUE)
      indices, ids, weights = sess.run(accidental_hits)

    self.assertEqual(1, accidental_hits[0].get_shape().ndims)
    self.assertEqual(1, accidental_hits[1].get_shape().ndims)
    self.assertEqual(1, accidental_hits[2].get_shape().ndims)
    for index, id_, weight in zip(indices, ids, weights):
      self.assertTrue(id_ in self.TRUE_LABELS[index])
      self.assertLess(weight, -1.0e37)

  def testSeed(self):

    def draw(seed):
      with self.test_session():
        true_classes = tf.constant([[1, 2], [0, 4], [3, 3]],
                                   dtype=tf.int64)
        sampled, _, _ = tf.nn.log_uniform_candidate_sampler(
            true_classes,
            self.NUM_TRUE,
            self.NUM_SAMPLED,
            True,
            5,
            seed=seed)
        return sampled.eval()
    # Non-zero seed. Repeatable.
    for seed in [1, 12, 123, 1234]:
      self.assertAllEqual(draw(seed), draw(seed))
    # Seed=0 means random seeds.
    num_same = 0
    for _ in range(10):
      if np.allclose(draw(None), draw(None)):
        num_same += 1
    # Accounts for the fact that the same random seed may be picked
    # twice very rarely.
    self.assertLessEqual(num_same, 2)


if __name__ == "__main__":
  tf.test.main()
