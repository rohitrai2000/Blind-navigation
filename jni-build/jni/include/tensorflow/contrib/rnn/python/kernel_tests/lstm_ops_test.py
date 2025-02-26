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

"""LSTM Fused Cell ops."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import numpy as np

import tensorflow as tf

from tensorflow.contrib.rnn.python.ops import lstm_ops


fused_lstm = lstm_ops._fused_lstm  # pylint: disable=protected-access


class LSTMFusedCellTest(tf.test.TestCase):
  _use_gpu = False

  def testNoneDimsWithDynamicRNN(self):
    with self.test_session(use_gpu=self._use_gpu, graph=tf.Graph()) as sess:
      batch_size = 4
      num_steps = 5
      input_dim = 6
      cell_size = 7

      cell = tf.contrib.rnn.LSTMFusedCell(cell_size)
      x = tf.placeholder(tf.float32, shape=(None, None, input_dim))

      output, _ = tf.nn.dynamic_rnn(cell, x, time_major=True, dtype=tf.float32)
      sess.run(tf.initialize_all_variables())
      feed = {}
      feed[x] = np.random.randn(num_steps, batch_size, input_dim)
      sess.run(output, feed)

  def testLSTMFusedCell(self):
    with self.test_session(use_gpu=self._use_gpu, graph=tf.Graph()) as sess:
      with tf.variable_scope("root", initializer=tf.constant_initializer(0.5)):
        x = tf.zeros([1, 2])
        m0 = tf.zeros([1, 2])
        m1 = tf.zeros([1, 2])
        m2 = tf.zeros([1, 2])
        m3 = tf.zeros([1, 2])
        g, ((out_m0, out_m1), (out_m2, out_m3)) = tf.nn.rnn_cell.MultiRNNCell(
            [tf.contrib.rnn.LSTMFusedCell(2)] * 2,
            state_is_tuple=True)(x, ((m0, m1), (m2, m3)))
        sess.run([tf.initialize_all_variables()])
        res = sess.run([g, out_m0, out_m1, out_m2, out_m3],
                       {x.name: np.array([[1., 1.]]),
                        m0.name: 0.1 * np.ones([1, 2]),
                        m1.name: 0.1 * np.ones([1, 2]),
                        m2.name: 0.1 * np.ones([1, 2]),
                        m3.name: 0.1 * np.ones([1, 2])})
        self.assertEqual(len(res), 5)
        self.assertAllClose(res[0], [[0.24024698, 0.24024698]])
        # These numbers are from testBasicLSTMCell and only test c/h.
        self.assertAllClose(res[1], [[0.68967271, 0.68967271]])
        self.assertAllClose(res[2], [[0.44848421, 0.44848421]])
        self.assertAllClose(res[3], [[0.39897051, 0.39897051]])
        self.assertAllClose(res[4], [[0.24024698, 0.24024698]])

  def testLSTMBasicToBlockCell(self):
    with self.test_session(use_gpu=self._use_gpu) as sess:
      x = tf.zeros([1, 2])
      x_values = np.random.randn(1, 2)

      m0_val = 0.1 * np.ones([1, 2])
      m1_val = -0.1 * np.ones([1, 2])
      m2_val = -0.2 * np.ones([1, 2])
      m3_val = 0.2 * np.ones([1, 2])

      initializer = tf.random_uniform_initializer(-0.01, 0.01, seed=19890212)
      with tf.variable_scope("basic", initializer=initializer):
        m0 = tf.zeros([1, 2])
        m1 = tf.zeros([1, 2])
        m2 = tf.zeros([1, 2])
        m3 = tf.zeros([1, 2])
        g, ((out_m0, out_m1), (out_m2, out_m3)) = tf.nn.rnn_cell.MultiRNNCell(
            [tf.nn.rnn_cell.BasicLSTMCell(2, state_is_tuple=True)] * 2,
            state_is_tuple=True)(x, ((m0, m1), (m2, m3)))
        sess.run([tf.initialize_all_variables()])
        basic_res = sess.run([g, out_m0, out_m1, out_m2, out_m3],
                             {x.name: x_values,
                              m0.name: m0_val,
                              m1.name: m1_val,
                              m2.name: m2_val,
                              m3.name: m3_val})

      with tf.variable_scope("block", initializer=initializer):
        m0 = tf.zeros([1, 2])
        m1 = tf.zeros([1, 2])
        m2 = tf.zeros([1, 2])
        m3 = tf.zeros([1, 2])
        g, ((out_m0, out_m1), (out_m2, out_m3)) = tf.nn.rnn_cell.MultiRNNCell(
            [tf.contrib.rnn.LSTMFusedCell(2)] * 2,
            state_is_tuple=True)(x, ((m0, m1), (m2, m3)))
        sess.run([tf.initialize_all_variables()])
        block_res = sess.run([g, out_m0, out_m1, out_m2, out_m3],
                             {x.name: x_values,
                              m0.name: m0_val,
                              m1.name: m1_val,
                              m2.name: m2_val,
                              m3.name: m3_val})

      self.assertEqual(len(basic_res), len(block_res))
      for basic, block in zip(basic_res, block_res):
        self.assertAllClose(basic, block)

  def testLSTMBasicToBlockCellPeeping(self):
    with self.test_session(use_gpu=self._use_gpu) as sess:
      x = tf.zeros([1, 2])
      x_values = np.random.randn(1, 2)

      m0_val = 0.1 * np.ones([1, 2])
      m1_val = -0.1 * np.ones([1, 2])
      m2_val = -0.2 * np.ones([1, 2])
      m3_val = 0.2 * np.ones([1, 2])

      initializer = tf.random_uniform_initializer(-0.01, 0.01, seed=19890212)
      with tf.variable_scope("basic", initializer=initializer):
        m0 = tf.zeros([1, 2])
        m1 = tf.zeros([1, 2])
        m2 = tf.zeros([1, 2])
        m3 = tf.zeros([1, 2])
        g, ((out_m0, out_m1), (out_m2, out_m3)) = tf.nn.rnn_cell.MultiRNNCell(
            [tf.nn.rnn_cell.LSTMCell(2,
                                     use_peepholes=True,
                                     state_is_tuple=True)] * 2,
            state_is_tuple=True)(x, ((m0, m1), (m2, m3)))
        sess.run([tf.initialize_all_variables()])
        basic_res = sess.run([g, out_m0, out_m1, out_m2, out_m3],
                             {x.name: x_values,
                              m0.name: m0_val,
                              m1.name: m1_val,
                              m2.name: m2_val,
                              m3.name: m3_val})

      with tf.variable_scope("block", initializer=initializer):
        m0 = tf.zeros([1, 2])
        m1 = tf.zeros([1, 2])
        m2 = tf.zeros([1, 2])
        m3 = tf.zeros([1, 2])
        g, ((out_m0, out_m1), (out_m2, out_m3)) = tf.nn.rnn_cell.MultiRNNCell(
            [tf.contrib.rnn.LSTMFusedCell(2, use_peephole=True)] * 2,
            state_is_tuple=True)(x, ((m0, m1), (m2, m3)))
        sess.run([tf.initialize_all_variables()])
        block_res = sess.run([g, out_m0, out_m1, out_m2, out_m3],
                             {x.name: x_values,
                              m0.name: m0_val,
                              m1.name: m1_val,
                              m2.name: m2_val,
                              m3.name: m3_val})

      self.assertEqual(len(basic_res), len(block_res))
      for basic, block in zip(basic_res, block_res):
        self.assertAllClose(basic, block)

  def testLSTMBasicToBlock(self):
    with self.test_session(use_gpu=self._use_gpu) as sess:
      batch_size = 2
      input_size = 3
      cell_size = 4
      sequence_length = 5

      inputs = []
      for _ in range(sequence_length):
        inp = tf.convert_to_tensor(
            np.random.randn(batch_size, input_size),
            dtype=tf.float32)
        inputs.append(inp)

      initializer = tf.random_uniform_initializer(-0.01, 0.01, seed=19890212)
      with tf.variable_scope("basic", initializer=initializer):
        cell = tf.nn.rnn_cell.BasicLSTMCell(cell_size, state_is_tuple=True)
        outputs, _ = tf.nn.rnn(cell, inputs, dtype=tf.float32)

        sess.run([tf.initialize_all_variables()])
        basic_outputs = sess.run(outputs)
        basic_grads = sess.run(tf.gradients(outputs, inputs))
        basic_wgrads = sess.run(tf.gradients(outputs, tf.trainable_variables()))

      with tf.variable_scope("block", initializer=initializer):
        w = tf.get_variable("w",
                            shape=[input_size + cell_size, cell_size * 4],
                            dtype=tf.float32)
        b = tf.get_variable("b",
                            shape=[cell_size * 4],
                            dtype=tf.float32,
                            initializer=tf.zeros_initializer)

        _, _, _, _, _, _, outputs = fused_lstm(
            tf.convert_to_tensor(sequence_length,
                                 dtype=tf.int64),
            inputs,
            w,
            b,
            cell_clip=0)

        sess.run([tf.initialize_all_variables()])
        block_outputs = sess.run(outputs)
        block_grads = sess.run(tf.gradients(outputs, inputs))
        block_wgrads = sess.run(tf.gradients(outputs, [w, b]))

      self.assertAllClose(basic_outputs, block_outputs)
      self.assertAllClose(basic_grads, block_grads)
      for basic, block in zip(basic_wgrads, block_wgrads):
        self.assertAllClose(basic, block, rtol=1e-2, atol=1e-2)

  def testLSTMBasicToBlockPeeping(self):
    with self.test_session(use_gpu=self._use_gpu) as sess:
      batch_size = 2
      input_size = 3
      cell_size = 4
      sequence_length = 5

      inputs = []
      for _ in range(sequence_length):
        inp = tf.convert_to_tensor(
            np.random.randn(batch_size, input_size),
            dtype=tf.float32)
        inputs.append(inp)

      initializer = tf.random_uniform_initializer(-0.01, 0.01, seed=19890212)
      with tf.variable_scope("basic", initializer=initializer):
        cell = tf.nn.rnn_cell.LSTMCell(cell_size,
                                       use_peepholes=True,
                                       state_is_tuple=True)
        outputs, _ = tf.nn.rnn(cell, inputs, dtype=tf.float32)

        sess.run([tf.initialize_all_variables()])
        basic_outputs = sess.run(outputs)
        basic_grads = sess.run(tf.gradients(outputs, inputs))
        basic_wgrads = sess.run(tf.gradients(outputs, tf.trainable_variables()))

      with tf.variable_scope("block", initializer=initializer):
        w = tf.get_variable("w",
                            shape=[input_size + cell_size, cell_size * 4],
                            dtype=tf.float32)
        b = tf.get_variable("b",
                            shape=[cell_size * 4],
                            dtype=tf.float32,
                            initializer=tf.zeros_initializer)

        wci = tf.get_variable("wci", shape=[cell_size], dtype=tf.float32)
        wcf = tf.get_variable("wcf", shape=[cell_size], dtype=tf.float32)
        wco = tf.get_variable("wco", shape=[cell_size], dtype=tf.float32)

        _, _, _, _, _, _, outputs = fused_lstm(
            tf.convert_to_tensor(sequence_length,
                                 dtype=tf.int64),
            inputs,
            w,
            b,
            wci=wci,
            wcf=wcf,
            wco=wco,
            cell_clip=0,
            use_peephole=True)

        sess.run([tf.initialize_all_variables()])
        block_outputs = sess.run(outputs)
        block_grads = sess.run(tf.gradients(outputs, inputs))
        block_wgrads = sess.run(tf.gradients(outputs, [w, b, wci, wcf, wco]))

      self.assertAllClose(basic_outputs, block_outputs)
      self.assertAllClose(basic_grads, block_grads)
      for basic, block in zip(basic_wgrads, block_wgrads):
        self.assertAllClose(basic, block, rtol=1e-2, atol=1e-2)


class LSTMFusedCellGpuTest(LSTMFusedCellTest):
  _use_gpu = True


if __name__ == "__main__":
  tf.test.main()
