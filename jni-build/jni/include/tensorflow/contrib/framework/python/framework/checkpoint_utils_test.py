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

"""Tests for checkpoints tools."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os

import tensorflow as tf


def _create_checkpoints(sess, checkpoint_dir):
  checkpoint_prefix = os.path.join(checkpoint_dir, "model")
  checkpoint_state_name = "checkpoint"
  v1 = tf.get_variable("var1", [1, 10])
  v2 = tf.get_variable("var2", [10, 10])
  v3 = tf.get_variable("var3", [100, 100])
  with tf.variable_scope("useful_scope"):
    v4 = tf.get_variable("var4", [9, 9])
  sess.run(tf.initialize_all_variables())
  v1_value, v2_value, v3_value, v4_value = sess.run([v1, v2, v3, v4])
  saver = tf.train.Saver()
  saver.save(sess, checkpoint_prefix, global_step=0,
             latest_filename=checkpoint_state_name)
  return v1_value, v2_value, v3_value, v4_value


def _create_partition_checkpoints(sess, checkpoint_dir):
  checkpoint_prefix = os.path.join(checkpoint_dir, "model")
  checkpoint_state_name = "checkpoint"
  v1 = tf.get_variable(
      name="var1",
      shape=[100, 100],
      initializer=tf.truncated_normal_initializer(0.5),
      partitioner=tf.min_max_variable_partitioner(max_partitions=5, axis=0,
                                                  min_slice_size=8 << 10))
  sess.run(tf.initialize_all_variables())
  v1_value = sess.run(v1._get_variable_list())
  saver = tf.train.Saver()
  saver.save(sess, checkpoint_prefix, global_step=0,
             latest_filename=checkpoint_state_name)
  return v1_value


class CheckpointsTest(tf.test.TestCase):

  def testNoCheckpoints(self):
    checkpoint_dir = self.get_temp_dir() + "/no_checkpoints"
    with self.assertRaises(tf.errors.OpError):
      self.assertAllEqual(tf.contrib.framework.load_variable(
          checkpoint_dir, "var1"), [])

  def testNoTensor(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      _, _, _, _ = _create_checkpoints(session, checkpoint_dir)
    with self.assertRaises(tf.errors.OpError):
      self.assertAllEqual(tf.contrib.framework.load_variable(
          checkpoint_dir, "var5"), [])

  def testGetTensor(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      v1, v2, v3, v4 = _create_checkpoints(session, checkpoint_dir)
    self.assertAllEqual(tf.contrib.framework.load_variable(
        checkpoint_dir, "var1"), v1)
    self.assertAllEqual(tf.contrib.framework.load_variable(
        checkpoint_dir, "var2"), v2)
    self.assertAllEqual(tf.contrib.framework.load_variable(
        checkpoint_dir, "var3"), v3)
    self.assertAllEqual(
        tf.contrib.framework.load_variable(
            checkpoint_dir, "useful_scope/var4"), v4)

  def testGetAllVariables(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      _create_checkpoints(session, checkpoint_dir)
    self.assertEqual(tf.contrib.framework.list_variables(checkpoint_dir),
                     [("useful_scope/var4", [9, 9]),
                      ("var1", [1, 10]),
                      ("var2", [10, 10]),
                      ("var3", [100, 100])])

  def testInitFromCheckpoint(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      v1, v2, v3, v4 = _create_checkpoints(session, checkpoint_dir)

    # New graph and session.
    with tf.Graph().as_default() as g:
      with self.test_session(graph=g) as session:
        with tf.variable_scope("some_scope"):
          my1 = tf.get_variable("my1", [1, 10])
          with tf.variable_scope("some_other_scope"):
            my2 = tf.get_variable("my2", [10, 10])
            with tf.variable_scope("other_useful_scope"):
              my4 = tf.get_variable("var4", [9, 9])
        my3 = tf.get_variable("my3", [100, 100])

        tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
            "var1": "some_scope/my1",
            "useful_scope/": "some_scope/some_other_scope/other_useful_scope/",
        })
        tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
            "var2": "some_scope/some_other_scope/my2",
            "var3": my3,
        })

        session.run(tf.initialize_all_variables())
        self.assertAllEqual(my1.eval(session), v1)
        self.assertAllEqual(my2.eval(session), v2)
        self.assertAllEqual(my3.eval(session), v3)
        self.assertAllEqual(my4.eval(session), v4)

        # Check that tensors are not explicitly in the graph.
        self.assertLess(len(str(session.graph.as_graph_def())), 26000)

  def testInitFromRootCheckpoint(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      v1, v2, v3, v4 = _create_checkpoints(session, checkpoint_dir)

    # New graph and session.
    with tf.Graph().as_default() as g:
      with self.test_session(graph=g) as session:
        with tf.variable_scope("some_scope"):
          my1 = tf.get_variable("var1", [1, 10])
          my2 = tf.get_variable("var2", [10, 10])
          my3 = tf.get_variable("var3", [100, 100])
          with tf.variable_scope("useful_scope"):
            my4 = tf.get_variable("var4", [9, 9])

        tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
            "/": "some_scope/",
        })

        session.run(tf.initialize_all_variables())
        self.assertAllEqual(my1.eval(session), v1)
        self.assertAllEqual(my2.eval(session), v2)
        self.assertAllEqual(my3.eval(session), v3)
        self.assertAllEqual(my4.eval(session), v4)

  def testInitFromPartitionVar(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      v1 = _create_partition_checkpoints(session, checkpoint_dir)

    # New graph and session.
    with tf.Graph().as_default() as g:
      with self.test_session(graph=g) as session:
        with tf.variable_scope("some_scope"):
          my1 = tf.get_variable(
              name="my1",
              shape=[100, 100],
              initializer=tf.truncated_normal_initializer(0.5),
              partitioner=tf.min_max_variable_partitioner(
                  max_partitions=5, axis=0, min_slice_size=8 << 10))
          my1_var_list = my1._get_variable_list()

        tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
            "var1": "some_scope/my1",
        })

        session.run(tf.initialize_all_variables())
        my1_values = session.run(my1_var_list)
        self.assertAllEqual(my1_values, v1)

    # New graph and session.
    with tf.Graph().as_default() as g:
      with self.test_session(graph=g) as session:
        with tf.variable_scope("some_scope"):
          my1 = tf.get_variable(
              name="my1",
              shape=[100, 100],
              initializer=tf.truncated_normal_initializer(0.5),
              partitioner=tf.min_max_variable_partitioner(
                  max_partitions=5, axis=0, min_slice_size=8 << 10))
          my1_var_list = my1._get_variable_list()

        tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
            "var1": my1_var_list,
        })

        session.run(tf.initialize_all_variables())
        my1_values = session.run(my1_var_list)
        self.assertAllEqual(my1_values, v1)

  def testInitFromCheckpointMissing(self):
    checkpoint_dir = self.get_temp_dir()
    with self.test_session() as session:
      _, _, _, _ = _create_checkpoints(session, checkpoint_dir)

    # New graph and session.
    with tf.Graph().as_default() as g:
      with self.test_session(graph=g) as session:
        with tf.variable_scope("some_scope"):
          _ = tf.get_variable("my1", [10, 10])
          _ = tf.get_variable("my2", [1, 10],
                              dtype=tf.int64, initializer=tf.zeros_initializer)

        # No directory.
        with self.assertRaises(tf.errors.OpError):
          tf.contrib.framework.init_from_checkpoint("no_dir", {
              "var1": "some_scope/my1"})

        # No variable in checkpoint.
        with self.assertRaises(ValueError):
          tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
              "no_var": "some_scope/my1"})

        # No variable in the graph.
        with self.assertRaises(ValueError):
          tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
              "var3": "some_scope/no_var"})

        # Shape mismatch.
        with self.assertRaises(ValueError):
          tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
              "var1": "some_scope/my1"})

        # Variable 'my1' and 'my2' are missing in given checkpoint scope.
        with self.assertRaises(ValueError):
          tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
              "useful_scope/": "some_scope/"})

        # Mapping is not to scope name.
        with self.assertRaises(ValueError):
          tf.contrib.framework.init_from_checkpoint(checkpoint_dir, {
              "useful_scope": "some_scope/"})

if __name__ == "__main__":
  tf.test.main()
