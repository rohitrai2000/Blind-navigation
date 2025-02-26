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

"""Tests for supervisor.py."""
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import glob
import os
import shutil
import time

from six.moves import xrange  # pylint: disable=redefined-builtin
import tensorflow as tf


def _summary_iterator(test_dir):
  """Reads events from test_dir/events.

  Args:
    test_dir: Name of the test directory.

  Returns:
    A summary_iterator
  """
  event_paths = sorted(glob.glob(os.path.join(test_dir, "event*")))
  return tf.train.summary_iterator(event_paths[-1])


def _test_dir(test_name):
  test_dir = os.path.join(tf.test.get_temp_dir(), test_name)
  if os.path.exists(test_dir):
    shutil.rmtree(test_dir)
  return test_dir


class SupervisorTest(tf.test.TestCase):

  # This test does not test much.
  def testBasics(self):
    logdir = _test_dir("basics")
    with tf.Graph().as_default():
      my_op = tf.constant(1.0)
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      for _ in xrange(10):
        sess.run(my_op)
      sess.close()
      sv.stop()

  def testManagedSession(self):
    logdir = _test_dir("managed_session")
    with tf.Graph().as_default():
      my_op = tf.constant(1.0)
      sv = tf.train.Supervisor(logdir=logdir)
      with sv.managed_session("") as sess:
        for _ in xrange(10):
          sess.run(my_op)
      # Supervisor has been stopped.
      self.assertTrue(sv.should_stop())

  def testManagedSessionUserError(self):
    logdir = _test_dir("managed_user_error")
    with tf.Graph().as_default():
      my_op = tf.constant(1.0)
      sv = tf.train.Supervisor(logdir=logdir)
      last_step = None
      with self.assertRaisesRegexp(RuntimeError, "failing here"):
        with sv.managed_session("") as sess:
          for step in xrange(10):
            last_step = step
            if step == 1:
              raise RuntimeError("failing here")
            else:
              sess.run(my_op)
      # Supervisor has been stopped.
      self.assertTrue(sv.should_stop())
      self.assertEqual(1, last_step)

  def testManagedSessionIgnoreOutOfRangeError(self):
    logdir = _test_dir("managed_out_of_range")
    with tf.Graph().as_default():
      my_op = tf.constant(1.0)
      sv = tf.train.Supervisor(logdir=logdir)
      last_step = None
      with sv.managed_session("") as sess:
        for step in xrange(10):
          last_step = step
          if step == 3:
            raise tf.errors.OutOfRangeError(my_op.op.node_def, my_op.op,
                                            "all done")
          else:
            sess.run(my_op)
      # Supervisor has been stopped.  OutOfRangeError was not thrown.
      self.assertTrue(sv.should_stop())
      self.assertEqual(3, last_step)

  def testManagedSessionDoNotKeepSummaryWriter(self):
    logdir = _test_dir("managed_not_keep_summary_writer")
    with tf.Graph().as_default():
      summ = tf.scalar_summary(["c1", "c2", "c3"], tf.constant([1.0, 2.0, 3.0]))
      sv = tf.train.Supervisor(logdir=logdir, summary_op=None)
      with sv.managed_session("", close_summary_writer=True,
                              start_standard_services=False) as sess:
        sv.summary_computed(sess, sess.run(summ))
      # Sleep 1.2s to make sure that the next event file has a different name
      # than the current one.
      time.sleep(1.2)
      with sv.managed_session("", close_summary_writer=True,
                              start_standard_services=False) as sess:
        sv.summary_computed(sess, sess.run(summ))
    event_paths = sorted(glob.glob(os.path.join(logdir, "event*")))
    self.assertEquals(2, len(event_paths))
    # The two event files should have the same contents.
    for path in event_paths:
      # The summary iterator should report the summary once as we closed the
      # summary writer across the 2 sessions.
      rr = tf.train.summary_iterator(path)
      # The first event should list the file_version.
      ev = next(rr)
      self.assertEquals("brain.Event:2", ev.file_version)

      # The next one has the graph.
      ev = next(rr)
      self.assertTrue(ev.graph_def)

      # The next one should have the values from the summary.
      # But only once.
      ev = next(rr)
      self.assertProtoEquals("""
        value { tag: 'c1' simple_value: 1.0 }
        value { tag: 'c2' simple_value: 2.0 }
        value { tag: 'c3' simple_value: 3.0 }
        """, ev.summary)

      # The next one should be a stop message if we closed cleanly.
      ev = next(rr)
      self.assertEquals(tf.SessionLog.STOP, ev.session_log.status)

      # We should be done.
      with self.assertRaises(StopIteration):
        next(rr)

  def testManagedSessionKeepSummaryWriter(self):
    logdir = _test_dir("managed_keep_summary_writer")
    with tf.Graph().as_default():
      summ = tf.scalar_summary(["c1", "c2", "c3"], tf.constant([1.0, 2.0, 3.0]))
      sv = tf.train.Supervisor(logdir=logdir)
      with sv.managed_session("", close_summary_writer=False,
                              start_standard_services=False) as sess:
        sv.summary_computed(sess, sess.run(summ))
      with sv.managed_session("", close_summary_writer=False,
                              start_standard_services=False) as sess:
        sv.summary_computed(sess, sess.run(summ))
    # Now close the summary writer to flush the events.
    sv.summary_writer.close()
    # The summary iterator should report the summary twice as we reused
    # the same summary writer across the 2 sessions.
    rr = _summary_iterator(logdir)
    # The first event should list the file_version.
    ev = next(rr)
    self.assertEquals("brain.Event:2", ev.file_version)

    # The next one has the graph.
    ev = next(rr)
    self.assertTrue(ev.graph_def)

    # The next one should have the values from the summary.
    ev = next(rr)
    self.assertProtoEquals("""
      value { tag: 'c1' simple_value: 1.0 }
      value { tag: 'c2' simple_value: 2.0 }
      value { tag: 'c3' simple_value: 3.0 }
      """, ev.summary)

    # The next one should also have the values from the summary.
    ev = next(rr)
    self.assertProtoEquals("""
      value { tag: 'c1' simple_value: 1.0 }
      value { tag: 'c2' simple_value: 2.0 }
      value { tag: 'c3' simple_value: 3.0 }
      """, ev.summary)

    # We should be done.
    self.assertRaises(StopIteration, lambda: next(rr))

  def _csv_data(self, logdir):
    # Create a small data file with 3 CSV records.
    data_path = os.path.join(logdir, "data.csv")
    with open(data_path, "w") as f:
      f.write("1,2,3\n")
      f.write("4,5,6\n")
      f.write("7,8,9\n")
    return data_path

  def testManagedEndOfInputOneQueue(self):
    # Tests that the supervisor finishes without an error when using
    # a fixed number of epochs, reading from a single queue.
    logdir = _test_dir("managed_end_of_input_one_queue")
    os.makedirs(logdir)
    data_path = self._csv_data(logdir)
    with tf.Graph().as_default():
      # Create an input pipeline that reads the file 3 times.
      filename_queue = tf.train.string_input_producer([data_path], num_epochs=3)
      reader = tf.TextLineReader()
      _, csv = reader.read(filename_queue)
      rec = tf.decode_csv(csv, record_defaults=[[1], [1], [1]])
      sv = tf.train.Supervisor(logdir=logdir)
      with sv.managed_session("") as sess:
        while not sv.should_stop():
          sess.run(rec)

  def testManagedEndOfInputTwoQueues(self):
    # Tests that the supervisor finishes without an error when using
    # a fixed number of epochs, reading from two queues, the second
    # one producing a batch from the first one.
    logdir = _test_dir("managed_end_of_input_two_queues")
    os.makedirs(logdir)
    data_path = self._csv_data(logdir)
    with tf.Graph().as_default():
      # Create an input pipeline that reads the file 3 times.
      filename_queue = tf.train.string_input_producer([data_path], num_epochs=3)
      reader = tf.TextLineReader()
      _, csv = reader.read(filename_queue)
      rec = tf.decode_csv(csv, record_defaults=[[1], [1], [1]])
      shuff_rec = tf.train.shuffle_batch(rec, 1, 6, 4)
      sv = tf.train.Supervisor(logdir=logdir)
      with sv.managed_session("") as sess:
        while not sv.should_stop():
          sess.run(shuff_rec)

  def testManagedMainErrorTwoQueues(self):
    # Tests that the supervisor correctly raises a main loop
    # error even when using multiple queues for input.
    logdir = _test_dir("managed_main_error_two_queues")
    os.makedirs(logdir)
    data_path = self._csv_data(logdir)
    with self.assertRaisesRegexp(RuntimeError, "fail at step 3"):
      with tf.Graph().as_default():
        # Create an input pipeline that reads the file 3 times.
        filename_queue = tf.train.string_input_producer([data_path],
                                                        num_epochs=3)
        reader = tf.TextLineReader()
        _, csv = reader.read(filename_queue)
        rec = tf.decode_csv(csv, record_defaults=[[1], [1], [1]])
        shuff_rec = tf.train.shuffle_batch(rec, 1, 6, 4)
        sv = tf.train.Supervisor(logdir=logdir)
        with sv.managed_session("") as sess:
          for step in range(9):
            if sv.should_stop():
              break
            elif step == 3:
              raise RuntimeError("fail at step 3")
            else:
              sess.run(shuff_rec)

  def testSessionConfig(self):
    logdir = _test_dir("session_config")
    with tf.Graph().as_default():
      with tf.device("/cpu:1"):
        my_op = tf.constant([1.0])
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session(
          "", config=tf.ConfigProto(device_count={"CPU": 2}))
      for _ in xrange(10):
        sess.run(my_op)
      sess.close()
      sv.stop()

  def testChiefCanWriteEvents(self):
    logdir = _test_dir("can_write")
    with tf.Graph().as_default():
      summ = tf.scalar_summary(["c1", "c2", "c3"], tf.constant([1.0, 2.0, 3.0]))
      sv = tf.train.Supervisor(is_chief=True, logdir=logdir, summary_op=None)
      sess = sv.prepare_or_wait_for_session("")
      sv.summary_computed(sess, sess.run(summ))
      sess.close()
      # Wait to make sure everything is written to file before stopping.
      time.sleep(1)
      sv.stop()

    rr = _summary_iterator(logdir)

    # The first event should list the file_version.
    ev = next(rr)
    self.assertEquals("brain.Event:2", ev.file_version)

    # The next one has the graph.
    ev = next(rr)
    ev_graph = tf.GraphDef()
    ev_graph.ParseFromString(ev.graph_def)
    self.assertProtoEquals(sess.graph.as_graph_def(add_shapes=True), ev_graph)

    # The next one should have the values from the summary.
    ev = next(rr)
    self.assertProtoEquals("""
      value { tag: 'c1' simple_value: 1.0 }
      value { tag: 'c2' simple_value: 2.0 }
      value { tag: 'c3' simple_value: 3.0 }
      """, ev.summary)

    # The next one should be a stop message if we closed cleanly.
    ev = next(rr)
    self.assertEquals(tf.SessionLog.STOP, ev.session_log.status)

    # We should be done.
    self.assertRaises(StopIteration, lambda: next(rr))

  def testNonChiefCannotWriteEvents(self):

    def _summary_computed():
      with tf.Graph().as_default():
        sv = tf.train.Supervisor(is_chief=False)
        sess = sv.prepare_or_wait_for_session("")
        summ = tf.scalar_summary(["c1", "c2"], tf.constant([1.0, 2.0]))
        sv.summary_computed(sess, sess.run(summ))

    def _start_standard_services():
      with tf.Graph().as_default():
        sv = tf.train.Supervisor(is_chief=False)
        sess = sv.prepare_or_wait_for_session("")
        sv.start_standard_services(sess)

    self.assertRaises(RuntimeError, _summary_computed)
    self.assertRaises(RuntimeError, _start_standard_services)

  def testNoLogdirButWantSummary(self):
    with tf.Graph().as_default():
      const = tf.constant([1.0, 2.0, 3.0])
      summ = tf.scalar_summary(["c1", "c2", "c3"], const)
      sv = tf.train.Supervisor(logdir="", summary_op=None)
      sess = sv.prepare_or_wait_for_session("")
      with self.assertRaisesRegexp(RuntimeError, "requires a summary writer"):
        sv.summary_computed(sess, sess.run(summ))

  def testLogdirButExplicitlyNoSummaryWriter(self):
    logdir = _test_dir("explicit_no_summary_writer")
    with tf.Graph().as_default():
      tf.Variable([1.0], name="foo")
      const = tf.constant([1.0, 2.0, 3.0])
      summ = tf.scalar_summary(["c1", "c2", "c3"], const)
      sv = tf.train.Supervisor(logdir=logdir, summary_writer=None)
      sess = sv.prepare_or_wait_for_session("")
      # Check that a checkpoint is still be generated.
      self._wait_for_glob(sv.save_path, 3.0)
      # Check that we cannot write a summary
      with self.assertRaisesRegexp(RuntimeError, "requires a summary writer"):
        sv.summary_computed(sess, sess.run(summ))

  def testNoLogdirButExplicitSummaryWriter(self):
    logdir = _test_dir("explicit_summary_writer")
    with tf.Graph().as_default():
      const = tf.constant([1.0, 2.0, 3.0])
      summ = tf.scalar_summary(["c1", "c2", "c3"], const)
      sw = tf.train.SummaryWriter(logdir)
      sv = tf.train.Supervisor(logdir="", summary_op=None, summary_writer=sw)
      sess = sv.prepare_or_wait_for_session("")
      sv.summary_computed(sess, sess.run(summ))
      sess.close()
      # Wait to make sure everything is written to file before stopping.
      time.sleep(1)
      sv.stop()

    # Check the summary was written to 'logdir'
    rr = _summary_iterator(logdir)

    # The first event should list the file_version.
    ev = next(rr)
    self.assertEquals("brain.Event:2", ev.file_version)

    # The next one has the graph.
    ev = next(rr)
    ev_graph = tf.GraphDef()
    ev_graph.ParseFromString(ev.graph_def)
    self.assertProtoEquals(sess.graph.as_graph_def(add_shapes=True), ev_graph)

    # The next one should have the values from the summary.
    ev = next(rr)
    self.assertProtoEquals("""
      value { tag: 'c1' simple_value: 1.0 }
      value { tag: 'c2' simple_value: 2.0 }
      value { tag: 'c3' simple_value: 3.0 }
      """, ev.summary)

    # The next one should be a stop message if we closed cleanly.
    ev = next(rr)
    self.assertEquals(tf.SessionLog.STOP, ev.session_log.status)

    # We should be done.
    self.assertRaises(StopIteration, lambda: next(rr))

  def testNoLogdirSucceeds(self):
    with tf.Graph().as_default():
      tf.Variable([1.0, 2.0, 3.0])
      sv = tf.train.Supervisor(logdir="", summary_op=None)
      sess = sv.prepare_or_wait_for_session("")
      sess.close()
      sv.stop()

  def testUseSessionManager(self):
    with tf.Graph().as_default():
      tf.Variable([1.0, 2.0, 3.0])
      sm = tf.train.SessionManager()
      # Pass in session_manager. The additional init_op is ignored.
      sv = tf.train.Supervisor(logdir="", session_manager=sm)
      sv.prepare_or_wait_for_session("")

  def testInitOp(self):
    logdir = _test_dir("default_init_op")
    with tf.Graph().as_default():
      v = tf.Variable([1.0, 2.0, 3.0])
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      self.assertAllClose([1.0, 2.0, 3.0], sess.run(v))
      sv.stop()

  def testInitFn(self):
    logdir = _test_dir("default_init_op")
    with tf.Graph().as_default():
      v = tf.Variable([1.0, 2.0, 3.0])
      def _init_fn(sess):
        sess.run(v.initializer)
      sv = tf.train.Supervisor(logdir=logdir, init_op=None, init_fn=_init_fn)
      sess = sv.prepare_or_wait_for_session("")
      self.assertAllClose([1.0, 2.0, 3.0], sess.run(v))
      sv.stop()

  def testInitOpWithFeedDict(self):
    logdir = _test_dir("feed_dict_init_op")
    with tf.Graph().as_default():
      p = tf.placeholder(tf.float32, shape=(3,))
      v = tf.Variable(p, name="v")
      sv = tf.train.Supervisor(logdir=logdir,
                               init_op=tf.initialize_all_variables(),
                               init_feed_dict={p: [1.0, 2.0, 3.0]})
      sess = sv.prepare_or_wait_for_session("")
      self.assertAllClose([1.0, 2.0, 3.0], sess.run(v))
      sv.stop()

  def testLocalInitOp(self):
    logdir = _test_dir("default_local_init_op")
    with tf.Graph().as_default():
      # A local variable.
      v = tf.Variable([1.0, 2.0, 3.0],
                      trainable=False,
                      collections=[tf.GraphKeys.LOCAL_VARIABLES])

      # An entity which is initialized through a TABLE_INITIALIZER.
      w = tf.Variable([4, 5, 6], trainable=False, collections=[])
      tf.add_to_collection(tf.GraphKeys.TABLE_INITIALIZERS, w.initializer)

      # This shouldn't add a variable to the VARIABLES collection responsible
      # for variables that are saved/restored from checkpoints.
      self.assertEquals(len(tf.all_variables()), 0)

      # Suppress normal variable inits to make sure the local one is
      # initialized via local_init_op.
      sv = tf.train.Supervisor(logdir=logdir, init_op=None)
      sess = sv.prepare_or_wait_for_session("")
      self.assertAllClose([1.0, 2.0, 3.0], sess.run(v))
      self.assertAllClose([4, 5, 6], sess.run(w))
      sv.stop()

  def testLocalInitOpForNonChief(self):
    logdir = _test_dir("default_local_init_op_non_chief")
    with tf.Graph().as_default():
      with tf.device("/job:localhost"):
              # A local variable.
        v = tf.Variable([1.0, 2.0, 3.0],
                        trainable=False,
                        collections=[tf.GraphKeys.LOCAL_VARIABLES])
        # This shouldn't add a variable to the VARIABLES collection responsible
        # for variables that are saved/restored from checkpoints.
        self.assertEquals(len(tf.all_variables()), 0)

      # Suppress normal variable inits to make sure the local one is
      # initialized via local_init_op.
      sv = tf.train.Supervisor(logdir=logdir, init_op=None, is_chief=False)
      sess = sv.prepare_or_wait_for_session("")
      self.assertAllClose([1.0, 2.0, 3.0], sess.run(v))
      sv.stop()

  def testInitOpFails(self):
    server = tf.train.Server.create_local_server()
    logdir = _test_dir("default_init_op_fails")
    with tf.Graph().as_default():
      v = tf.Variable([1.0, 2.0, 3.0], name="v")
      tf.Variable([4.0, 5.0, 6.0], name="w")
      # w will not be initialized.
      sv = tf.train.Supervisor(logdir=logdir, init_op=v.initializer)
      with self.assertRaisesRegexp(RuntimeError,
                                   "Variables not initialized: w"):
        sv.prepare_or_wait_for_session(server.target)

  def testInitOpFailsForTransientVariable(self):
    server = tf.train.Server.create_local_server()
    logdir = _test_dir("default_init_op_fails_for_local_variable")
    with tf.Graph().as_default():
      v = tf.Variable([1.0, 2.0, 3.0], name="v",
                      collections=[tf.GraphKeys.LOCAL_VARIABLES])
      tf.Variable([1.0, 2.0, 3.0], name="w",
                  collections=[tf.GraphKeys.LOCAL_VARIABLES])
      # w will not be initialized.
      sv = tf.train.Supervisor(logdir=logdir, local_init_op=v.initializer)
      with self.assertRaisesRegexp(
          RuntimeError, "Variables not initialized: w"):
        sv.prepare_or_wait_for_session(server.target)

  def testSetupFail(self):
    logdir = _test_dir("setup_fail")
    with tf.Graph().as_default():
      tf.Variable([1.0, 2.0, 3.0], name="v")
      with self.assertRaisesRegexp(ValueError, "must have their device set"):
        tf.train.Supervisor(logdir=logdir, is_chief=False)
    with tf.Graph().as_default(), tf.device("/job:ps"):
      tf.Variable([1.0, 2.0, 3.0], name="v")
      tf.train.Supervisor(logdir=logdir, is_chief=False)

  def testDefaultGlobalStep(self):
    logdir = _test_dir("default_global_step")
    with tf.Graph().as_default():
      tf.Variable(287, name="global_step")
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      self.assertEquals(287, sess.run(sv.global_step))
      sv.stop()

  def testRestoreFromMetaGraph(self):
    logdir = _test_dir("restore_from_meta_graph")
    with tf.Graph().as_default():
      tf.Variable(1, name="v0")
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      filename = sv.saver.save(sess, sv.save_path)
      sv.stop()
    # Create a new Graph and Supervisor and recover.
    with tf.Graph().as_default():
      new_saver = tf.train.import_meta_graph(".".join([filename, "meta"]))
      self.assertIsNotNone(new_saver)
      sv2 = tf.train.Supervisor(logdir=logdir, saver=new_saver)
      sess = sv2.prepare_or_wait_for_session("")
      self.assertEquals(1, sess.run("v0:0"))
      sv2.saver.save(sess, sv2.save_path)
      sv2.stop()

  def _wait_for_glob(self, pattern, timeout_secs):
    """Wait for a checkpoint file to appear.

    Args:
      pattern: A string.
      timeout_secs: How long to wait for in seconds.
    """
    end_time = time.time() + timeout_secs
    while time.time() < end_time:
      if len(tf.gfile.Glob(pattern)) >= 1:
        return
      time.sleep(0.05)
    self.assertFalse(True, "Glob never matched any file: %s" % pattern)

  # This test is based on the fact that the standard services start
  # right away and get to run once before sv.stop() returns.
  # We still sleep a bit to make the test robust.
  def testStandardServicesWithoutGlobalStep(self):
    logdir = _test_dir("standard_services_without_global_step")
    # Create a checkpoint.
    with tf.Graph().as_default():
      v = tf.Variable([1.0], name="foo")
      tf.scalar_summary(["v"], v)
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      save_path = sv.save_path
      self._wait_for_glob(save_path, 3.0)
      self._wait_for_glob(os.path.join(logdir, "*events*"), 3.0)
      # Wait to make sure everything is written to file before stopping.
      time.sleep(1)
      sv.stop()
    # There should be an event file with a version number.
    rr = _summary_iterator(logdir)
    ev = next(rr)
    self.assertEquals("brain.Event:2", ev.file_version)
    ev = next(rr)
    ev_graph = tf.GraphDef()
    ev_graph.ParseFromString(ev.graph_def)
    self.assertProtoEquals(sess.graph.as_graph_def(add_shapes=True), ev_graph)
    ev = next(rr)
    self.assertProtoEquals("value { tag: 'v' simple_value: 1.0 }", ev.summary)
    ev = next(rr)
    self.assertEquals(tf.SessionLog.STOP, ev.session_log.status)

    self.assertRaises(StopIteration, lambda: next(rr))
    # There should be a checkpoint file with the variable "foo"
    with tf.Graph().as_default(), self.test_session() as sess:
      v = tf.Variable([10.10], name="foo")
      sav = tf.train.Saver([v])
      sav.restore(sess, save_path)
      self.assertEqual(1.0, v.eval()[0])

  # Same as testStandardServicesNoGlobalStep but with a global step.
  # We should get a summary about the step time.
  def testStandardServicesWithGlobalStep(self):
    logdir = _test_dir("standard_services_with_global_step")
    # Create a checkpoint.
    with tf.Graph().as_default():
      v = tf.Variable([123], name="global_step")
      sv = tf.train.Supervisor(logdir=logdir)
      sess = sv.prepare_or_wait_for_session("")
      # This is where the checkpoint will appear, with step number 123.
      save_path = "%s-123" % sv.save_path
      self._wait_for_glob(save_path, 3.0)
      self._wait_for_glob(os.path.join(logdir, "*events*"), 3.0)
      # Wait to make sure everything is written to file before stopping.
      time.sleep(1)
      sv.stop()
    # There should be an event file with a version number.
    rr = _summary_iterator(logdir)
    ev = next(rr)
    self.assertEquals("brain.Event:2", ev.file_version)
    ev = next(rr)
    ev_graph = tf.GraphDef()
    ev_graph.ParseFromString(ev.graph_def)
    self.assertProtoEquals(sess.graph.as_graph_def(add_shapes=True), ev_graph)
    ev = next(rr)
    # It is actually undeterministic whether SessionLog.START gets written
    # before the summary or the checkpoint, but this works when run 10000 times.
    self.assertEquals(123, ev.step)
    self.assertEquals(tf.SessionLog.START, ev.session_log.status)
    first = next(rr)
    second = next(rr)
    # It is undeterministic whether the value gets written before the checkpoint
    # since they are on separate threads, so we check for both conditions.
    if first.HasField("summary"):
      self.assertProtoEquals("""value { tag: 'global_step/sec'
                                        simple_value: 0.0 }""",
                             first.summary)
      self.assertEquals(123, second.step)
      self.assertEquals(tf.SessionLog.CHECKPOINT, second.session_log.status)
    else:
      self.assertEquals(123, first.step)
      self.assertEquals(tf.SessionLog.CHECKPOINT, first.session_log.status)
      self.assertProtoEquals("""value { tag: 'global_step/sec'
                                        simple_value: 0.0 }""",
                             second.summary)
    ev = next(rr)
    self.assertEquals(tf.SessionLog.STOP, ev.session_log.status)
    self.assertRaises(StopIteration, lambda: next(rr))
    # There should be a checkpoint file with the variable "foo"
    with tf.Graph().as_default(), self.test_session() as sess:
      v = tf.Variable([-12], name="global_step")
      sav = tf.train.Saver([v])
      sav.restore(sess, save_path)
      self.assertEqual(123, v.eval()[0])

  def testNoQueueRunners(self):
    with tf.Graph().as_default(), self.test_session() as sess:
      sv = tf.train.Supervisor(logdir=_test_dir("no_queue_runners"))
      self.assertEqual(0, len(sv.start_queue_runners(sess)))
      sv.stop()

  def testPrepareSessionAfterStopForChief(self):
    logdir = _test_dir("prepare_after_stop_chief")
    with tf.Graph().as_default():
      sv = tf.train.Supervisor(logdir=logdir, is_chief=True)

      # Create a first session and then stop.
      sess = sv.prepare_or_wait_for_session("")
      sv.stop()
      sess.close()
      self.assertTrue(sv.should_stop())

      # Now create a second session and test that we don't stay stopped, until
      # we ask to stop again.
      sess2 = sv.prepare_or_wait_for_session("")
      self.assertFalse(sv.should_stop())
      sv.stop()
      sess2.close()
      self.assertTrue(sv.should_stop())

  def testPrepareSessionAfterStopForNonChief(self):
    logdir = _test_dir("prepare_after_stop_nonchief")
    with tf.Graph().as_default():
      sv = tf.train.Supervisor(logdir=logdir, is_chief=False)

      # Create a first session and then stop.
      sess = sv.prepare_or_wait_for_session("")
      sv.stop()
      sess.close()
      self.assertTrue(sv.should_stop())

      # Now create a second session and test that we don't stay stopped, until
      # we ask to stop again.
      sess2 = sv.prepare_or_wait_for_session("")
      self.assertFalse(sv.should_stop())
      sv.stop()
      sess2.close()
      self.assertTrue(sv.should_stop())


if __name__ == "__main__":
  tf.test.main()
