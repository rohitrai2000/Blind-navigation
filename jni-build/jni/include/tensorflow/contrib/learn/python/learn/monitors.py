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

"""Monitors allow user instrumentation of the training process.

Monitors are useful to track training, report progress, request early
stopping and more. Monitors use the observer pattern and notify at the following
points:
 - when training begins
 - before a training step
 - after a training step
 - when training ends

Monitors are not intended to be reusable.

There are a few pre-defined monitors:
 - CaptureVariable: saves a variable's values
 - GraphDump: intended for debug only - saves all tensor values
 - PrintTensor: outputs one or more tensor values to log
 - SummarySaver: saves summaries to a summary writer
 - ValidationMonitor: runs model validation, by periodically calculating eval
     metrics on a separate data set; supports optional early stopping

For more specific needs, you can create custom monitors by extending one of the
following classes:
 - BaseMonitor: the base class for all monitors
 - EveryN: triggers a callback every N training steps

Example:

  class ExampleMonitor(monitors.BaseMonitor):
    def __init__(self):
      print 'Init'

    def begin(self, max_steps):
      print 'Starting run. Will train until step %d.' % max_steps

    def end(self):
      print 'Completed run.'

    def step_begin(self, step):
      print 'About to run step %d...' % step
      return ['loss_1:0']

    def step_end(self, step, outputs):
      print 'Done running step %d. The value of "loss" tensor: %s' % (
        step, outputs['loss_1:0'])

  linear_regressor = LinearRegressor()
  example_monitor = ExampleMonitor()
  linear_regressor.fit(
    x, y, steps=2, batch_size=1, monitors=[example_monitor])

@@get_default_monitors
@@BaseMonitor
@@CaptureVariable
@@CheckpointSaver
@@EveryN
@@ExportMonitor
@@GraphDump
@@LoggingTrainable
@@NanLoss
@@PrintTensor
@@StepCounter
@@StopAtStep
@@SummarySaver
@@ValidationMonitor
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import os
import time

import numpy as np
import six

from tensorflow.contrib.learn.python.learn.summary_writer_cache import SummaryWriterCache
from tensorflow.contrib.learn.python.learn.utils import export
from tensorflow.core.framework.summary_pb2 import Summary
from tensorflow.core.util.event_pb2 import SessionLog
from tensorflow.python.framework import ops
from tensorflow.python.platform import tf_logging as logging
from tensorflow.python.training import saver as saver_lib
from tensorflow.python.training import summary_io


# TODO(ptucker): Split each monitor class into a separate file.
# TODO(ptucker): Fail if epoch or step does not monotonically increase?
class BaseMonitor(object):
  """Base class for Monitors.

  Defines basic interfaces of Monitors.
  Monitors can either be run on all workers or, more commonly, restricted
  to run exclusively on the elected chief worker.
  """

  def __init__(self):
    self._begun = False
    self._current_epoch = None
    self._current_step = None
    self._max_steps = None
    self._estimator = None

  @property
  def run_on_all_workers(self):
    return False

  def set_estimator(self, estimator):
    """A setter called automatically by the target estimator.

    Args:
      estimator: the estimator that this monitor monitors.

    Raises:
      ValueError: if the estimator is None.
    """
    if estimator is None:
      raise ValueError("Missing estimator.")
    # TODO(mdan): This should fail if called twice with the same estimator.
    self._estimator = estimator

  def begin(self, max_steps=None):
    """Called at the beginning of training.

    When called, the default graph is the one we are executing.

    Args:
      max_steps: `int`, the maximum global step this training will run until.

    Raises:
      ValueError: if we've already begun a run.
    """
    if self._begun:
      raise ValueError("begin called twice without end.")
    self._max_steps = max_steps
    self._begun = True

  def end(self, session=None):
    """Callback at the end of training/evaluation.

    Args:
      session: A `tf.Session` object that can be used to run ops.

    Raises:
      ValueError: if we've not begun a run.
    """
    _ = session
    if not self._begun:
      raise ValueError("end called without begin.")
    self._max_steps = None
    self._begun = False

  def epoch_begin(self, epoch):
    """Begin epoch.

    Args:
      epoch: `int`, the epoch number.

    Raises:
      ValueError: if we've already begun an epoch, or `epoch` < 0.
    """
    if self._current_epoch is not None:
      raise ValueError("epoch_begin called twice without epoch_end.")
    if epoch < 0:
      raise ValueError("Invalid epoch %s." % epoch)
    self._current_epoch = epoch

  def epoch_end(self, epoch):
    """End epoch.

    Args:
      epoch: `int`, the epoch number.

    Raises:
      ValueError: if we've not begun an epoch, or `epoch` number does not match.
    """
    if self._current_epoch != epoch:
      raise ValueError(
          "epoch_end expected %s but got %s.", self._current_epoch, epoch)
    self._current_epoch = None

  def step_begin(self, step):
    """Callback before training step begins.

    You may use this callback to request evaluation of additional tensors
    in the graph.

    Args:
      step: `int`, the current value of the global step.

    Returns:
      List of `Tensor` objects or string tensor names to be run.

    Raises:
      ValueError: if we've already begun a step, or `step` < 0, or
          `step` > `max_steps`.
    """
    if (step < 0) or (
        (self._max_steps is not None) and (step > self._max_steps)):
      raise ValueError("Invalid step %s." % step)
    self._current_step = step
    return []

  def step_end(self, step, output):  # pylint: disable=unused-argument
    """Callback after training step finished.

    This callback provides access to the tensors/ops evaluated at this step,
    including the additional tensors for which evaluation was requested in
    `step_begin`.

    In addition, the callback has the opportunity to stop training by returning
    `True`. This is useful for early stopping, for example.

    Note that this method is not called if the call to `Session.run()` that
    followed the last call to `step_begin()` failed.

    Args:
      step: `int`, the current value of the global step.
      output: `dict` mapping `string` values representing tensor names to
        the value resulted from running these tensors. Values may be either
        scalars, for scalar tensors, or Numpy `array`, for non-scalar tensors.

    Returns:
      `bool`. True if training should stop.

    Raises:
      ValueError: if we've not begun a step, or `step` number does not match.
    """
    if self._current_step != step:
      raise ValueError(
          "step_end expected %s but got %s.", self._current_step, step)
    self._current_step = None
    return False

  def post_step(self, step, session):  # pylint: disable=unused-argument
    """Callback after the step is finished.

    Called after step_end and receives session to perform extra session.run
    calls. If failure occurred in the process, will be called as well.

    Args:
      step: `int`, global step of the model.
      session: `Session` object.
    """
    _ = step, session


def _extract_output(outputs, request):
  if request in outputs:
    return outputs[request]
  return outputs[request.name]


class EveryN(BaseMonitor):
  """Base class for monitors that execute callbacks every N steps.

  This class adds three new callbacks:
    - every_n_step_begin
    - every_n_step_end
    - every_n_pos_step

  The callbacks are executed every n steps, or optionally every step for the
  first m steps, where m and n can both be user-specified.

  When extending this class, note that if you wish to use any of the
  `BaseMonitor` callbacks, you must call their respective super implementation:

    def step_begin(self, step):
      super(ExampleMonitor, self).step_begin(step)
      return []

  Failing to call the super implementation will cause unpredictible behavior.

  The `every_n_post_step()` callback is also called after the last step if it
  was not already called through the regular conditions.  Note that
  `every_n_step_begin()` and `every_n_step_end()` do not receive that special
  treatment.

  """
  # TODO(ipolosukhin): Add also every n seconds.

  def __init__(self, every_n_steps=100, first_n_steps=1):
    """Initializes an `EveryN` monitor.

    Args:
      every_n_steps: `int`, the number of steps to allow between callbacks.
      first_n_steps: `int`, specifying the number of initial steps during
        which the callbacks will always be executed, regardless of the value
        of `every_n_steps`. Note that this value is relative to the global step
    """
    super(EveryN, self).__init__()
    self._every_n_steps = every_n_steps
    self._first_n_steps = first_n_steps
    # Last step in the model.
    self._last_successful_step = None
    # Last step at which we called one of the every_n methods
    self._last_active_step = 0
    self._every_n_step_begin_called = False

  def every_n_step_begin(self, step):  # pylint: disable=unused-argument
    """Callback before every n'th step begins.

    Args:
      step: `int`, the current value of the global step.

    Returns:
      A `list` of tensors that will be evaluated at this step.
    """
    return []

  def every_n_step_end(self, step, outputs):  # pylint: disable=unused-argument
    """Callback after every n'th step finished.

    This callback provides access to the tensors/ops evaluated at this step,
    including the additional tensors for which evaluation was requested in
    `step_begin`.

    In addition, the callback has the opportunity to stop training by returning
    `True`. This is useful for early stopping, for example.

    Args:
      step: `int`, the current value of the global step.
      outputs: `dict` mapping `string` values representing tensor names to
        the value resulted from running these tensors. Values may be either
        scalars, for scalar tensors, or Numpy `array`, for non-scalar tensors.

    Returns:
      `bool`. True if training should stop.
    """
    return False

  def every_n_post_step(self, step, session):
    """Callback after a step is finished or `end()` is called.

    Args:
      step: `int`, the current value of the global step.
      session: `Session` object.
    """
    pass

  def step_begin(self, step):
    """Overrides `BaseMonitor.step_begin`.

    When overriding this method, you must call the super implementation.

    Args:
      step: `int`, the current value of the global step.
    Returns:
      A `list`, the result of every_n_step_begin, if that was called this step,
      or an empty list otherwise.

    Raises:
      ValueError: if called more than once during a step.
    """
    super(EveryN, self).step_begin(step)
    if (step <= self._first_n_steps or
        step >= (self._every_n_steps + self._last_active_step) or
        step == self._max_steps):  # Note: max_steps can be None here.
      self._every_n_step_begin_called = True
      return self.every_n_step_begin(step)
    else:
      self._every_n_step_begin_called = False
    return []

  def step_end(self, step, output):
    """Overrides `BaseMonitor.step_end`.

    When overriding this method, you must call the super implementation.

    Args:
      step: `int`, the current value of the global step.
      output: `dict` mapping `string` values representing tensor names to
        the value resulted from running these tensors. Values may be either
        scalars, for scalar tensors, or Numpy `array`, for non-scalar tensors.
    Returns:
      `bool`, the result of every_n_step_end, if that was called this step,
      or `False` otherwise.
    """
    super(EveryN, self).step_end(step, output)
    if self._every_n_step_begin_called:
      return self.every_n_step_end(step, output)
    return False

  def post_step(self, step, session):
    super(EveryN, self).post_step(step, session)
    if self._every_n_step_begin_called:
      self.every_n_post_step(step, session)
      self._last_active_step = step
    self._last_successful_step = step

  def end(self, session=None):
    super(EveryN, self).end(session=session)
    if self._last_successful_step != self._last_active_step:
      self.every_n_post_step(self._last_successful_step, session)


class StopAtStep(BaseMonitor):
  """Monitor to request stop at a specified step."""

  def __init__(self, num_steps=None, last_step=None):
    """Create a StopAtStep monitor.

    This monitor requests stop after either a number of steps have been
    executed or a last step has been reached.  Only of the two options can be
    specified.

    if `num_steps` is specified, it indicates the number of steps to execute
    after `begin()` is called.  If instead `last_step` is specified, it
    indicates the last step we want to execute, as passed to the `step_begin()`
    call.

    Args:
      num_steps: Number of steps to execute.
      last_step: Step after which to stop.

    Raises:
      ValueError: If one of the arguments is invalid.
    """
    super(StopAtStep, self).__init__()
    if num_steps is None and last_step is None:
      raise ValueError("One of num_steps or last_step must be specified.")
    if num_steps is not None and last_step is not None:
      raise ValueError("Only one of num_steps or last_step can be specified.")
    self._num_steps = num_steps
    self._last_step = last_step

  @property
  def run_on_all_workers(self):
    return True

  def step_begin(self, step):
    super(StopAtStep, self).step_begin(step)
    if self._last_step is None:
      self._last_step = step + self._num_steps - 1
    return []

  def step_end(self, step, output):
    super(StopAtStep, self).step_end(step, output)
    return step >= self._last_step


# TODO(ptucker): Rename to LoggingTensor since it's not writing to stdout.
class PrintTensor(EveryN):
  """Prints given tensors every N steps.

  This is an `EveryN` monitor and has consistent semantic for `every_n`
  and `first_n`.

  The tensors will be printed to the log, with `INFO` severity.
  """

  def __init__(self, tensor_names, every_n=100, first_n=1):
    """Initializes a PrintTensor monitor.

    Args:
      tensor_names: `dict` of tag to tensor names or
          `iterable` of tensor names (strings).
      every_n: `int`, print every N steps. See `PrintN.`
      first_n: `int`, also print the first N steps. See `PrintN.`
    """
    super(PrintTensor, self).__init__(every_n, first_n)
    if not isinstance(tensor_names, dict):
      tensor_names = {item: item for item in tensor_names}
    self._tensor_names = tensor_names

  def every_n_step_begin(self, step):
    super(PrintTensor, self).every_n_step_begin(step)
    return list(self._tensor_names.values())

  def every_n_step_end(self, step, outputs):
    super(PrintTensor, self).every_n_step_end(step, outputs)
    stats = []
    for tag, tensor_name in six.iteritems(self._tensor_names):
      if tensor_name in outputs:
        stats.append("%s = %s" % (tag,
                                  str(_extract_output(outputs, tensor_name))))
    logging.info("Step %d: %s", step, ", ".join(stats))


class LoggingTrainable(EveryN):
  """Writes trainable variable values into log every N steps.

  Write the tensors in trainable variables `every_n` steps,
  starting with the `first_n`th step.

  """

  def __init__(self, scope=None, every_n=100, first_n=1):
    """Initializes LoggingTrainable monitor.

    Args:
      scope: An optional string to match variable names using re.match.
      every_n: Print every N steps.
      first_n: Print first N steps.
    """
    super(LoggingTrainable, self).__init__(every_n, first_n)
    self._scope = scope

  def every_n_step_begin(self, step):
    super(LoggingTrainable, self).every_n_step_begin(step)
    # Get a list of trainable variables at the begining of every N steps.
    # We cannot get this in __init__ because train_op has not been generated.
    trainables = ops.get_collection(ops.GraphKeys.TRAINABLE_VARIABLES,
                                    scope=self._scope)
    self._names = {}
    for var in trainables:
      self._names[var.name] = var.value().name
    return list(self._names.values())

  def every_n_step_end(self, step, outputs):
    super(LoggingTrainable, self).every_n_step_end(step, outputs)
    stats = []
    for tag, tensor_name in six.iteritems(self._names):
      if tensor_name in outputs:
        stats.append("%s = %s" % (tag,
                                  str(_extract_output(outputs, tensor_name))))
    logging.info("Logging Trainable: Step %d: %s", step, ", ".join(stats))


class SummarySaver(EveryN):
  """Saves summaries every N steps."""

  def __init__(self,
               summary_op,
               save_steps=100,
               output_dir=None,
               summary_writer=None,
               scaffold=None):
    """Initializes a `SummarySaver` monitor.

    Args:
      summary_op: `Tensor` of type `string`. A serialized `Summary` protocol
          buffer, as output by TF summary methods like `scalar_summary` or
          `merge_all_summaries`.
      save_steps: `int`, save summaries every N steps. See `EveryN`.
      output_dir: `string`, the directory to save the summaries to. Only used
          if no `summary_writer` is supplied.
      summary_writer: `SummaryWriter`. If `None` and an `output_dir` was passed,
          one will be created accordingly.
      scaffold: `Scaffold` to get summary_op if it's not provided.
    """
    # TODO(ipolosukhin): Implement every N seconds.
    super(SummarySaver, self).__init__(every_n_steps=save_steps)
    self._summary_op = summary_op
    self._summary_writer = summary_writer
    if summary_writer is None and output_dir:
      self._summary_writer = summary_io.SummaryWriter(output_dir)
    self._scaffold = scaffold
    # TODO(mdan): Throw an error if output_dir and summary_writer are None.

  def set_estimator(self, estimator):
    super(SummarySaver, self).set_estimator(estimator)
    # TODO(mdan): This line looks redundant.
    if self._summary_writer is None:
      self._summary_writer = summary_io.SummaryWriter(estimator.model_dir)

  def every_n_step_begin(self, step):
    super(SummarySaver, self).every_n_step_begin(step)
    if self._summary_op is None and self._scaffold is not None:
      self._summary_op = self._scaffold.summary_op
    if self._summary_op is not None:
      return [self._summary_op]
    return []

  def every_n_step_end(self, step, outputs):
    super(SummarySaver, self).every_n_step_end(step, outputs)
    if self._summary_op is not None:
      summary_strs = _extract_output(outputs, self._summary_op)
      if self._summary_writer:
        self._summary_writer.add_summary(summary_strs, step)
    return False

  def end(self, session=None):
    super(SummarySaver, self).end(session=session)
    if self._summary_writer:
      self._summary_writer.flush()


class ValidationMonitor(EveryN):
  """Runs evaluation of a given estimator, at most every N steps.

  Note that the evaluation is done based on the saved checkpoint, which will
  usually be older than the current step.

  Can do early stopping on validation metrics if `early_stopping_rounds` is
  provided.
  """

  def __init__(self, x=None, y=None, input_fn=None, batch_size=None,
               eval_steps=None,
               every_n_steps=100, metrics=None, early_stopping_rounds=None,
               early_stopping_metric="loss",
               early_stopping_metric_minimize=True, name=None):
    """Initializes a ValidationMonitor.

    Args:
      x: See `BaseEstimator.evaluate`.
      y: See `BaseEstimator.evaluate`.
      input_fn: See `BaseEstimator.evaluate`.
      batch_size: See `BaseEstimator.evaluate`.
      eval_steps: See `BaseEstimator.evaluate`.
      every_n_steps: Check for new checkpoints to evaluate every N steps. If a
          new checkpoint is found, it is evaluated. See `EveryN`.
      metrics: See `BaseEstimator.evaluate`.
      early_stopping_rounds: `int`. If the metric indicated by
          `early_stopping_metric` does not change according to
          `early_stopping_metric_minimize` for this many steps, then training
          will be stopped.
      early_stopping_metric: `string`, name of the metric to check for early
          stopping.
      early_stopping_metric_minimize: `bool`, True if `early_stopping_metric` is
          expected to decrease (thus early stopping occurs when this metric
          stops decreasing), False if `early_stopping_metric` is expected to
          increase. Typically, `early_stopping_metric_minimize` is True for
          loss metrics like mean squared error, and False for performance
          metrics like accuracy.
      name: See `BaseEstimator.evaluate`.

    Raises:
      ValueError: If both x and input_fn are provided.
    """
    super(ValidationMonitor, self).__init__(every_n_steps=every_n_steps,
                                            first_n_steps=-1)
    # TODO(mdan): Checks like this are already done by evaluate.
    if x is None and input_fn is None:
      raise ValueError("Either x or input_fn should be provided.")
    self.x = x
    self.y = y
    self.input_fn = input_fn
    self.batch_size = batch_size
    self.eval_steps = eval_steps
    self.metrics = metrics
    self.early_stopping_rounds = early_stopping_rounds
    self.early_stopping_metric = early_stopping_metric
    self.early_stopping_metric_minimize = early_stopping_metric_minimize
    self.name = name
    self._best_value_step = None
    self._best_value = None
    self._early_stopped = False
    self._latest_path = None
    self._latest_path_step = None

  @property
  def early_stopped(self):
    """Returns True if this monitor caused an early stop."""
    return self._early_stopped

  @property
  def best_step(self):
    """Returns the step at which the best early stopping metric was found."""
    return self._best_value_step

  @property
  def best_value(self):
    """Returns the best early stopping metric value found so far."""
    return self._best_value

  def every_n_step_end(self, step, outputs):
    super(ValidationMonitor, self).every_n_step_end(step, outputs)
    # TODO(mdan): The use of step below is probably misleading.
    # The code should probably use the step from the checkpoint, because
    # that's what is being evaluated.
    if self._estimator is None:
      raise ValueError("Missing call to set_estimator.")
    # Check that we are not running evaluation on the same checkpoint.
    latest_path = saver_lib.latest_checkpoint(self._estimator.model_dir)
    if latest_path is None:
      logging.info("Skipping evaluation since model has not been saved yet "
                   "at step %d.", step)
      return False
    if latest_path is not None and latest_path == self._latest_path:
      logging.info("Skipping evaluation due to same checkpoint %s for step %d "
                   "as for step %d.", latest_path, step, self._latest_path_step)
      return False
    self._latest_path = latest_path
    self._latest_path_step = step

    # Run evaluation and log it.
    outputs = self._estimator.evaluate(
        x=self.x, y=self.y, input_fn=self.input_fn, batch_size=self.batch_size,
        steps=self.eval_steps, metrics=self.metrics, name=self.name)
    stats = []
    for name in outputs:
      stats.append("%s = %s" % (name, str(outputs[name])))
    logging.info("Validation (step %d): %s", step, ", ".join(stats))

    # Early stopping logic.
    if self.early_stopping_rounds is not None:
      current_value = outputs[self.early_stopping_metric]
      if (self._best_value is None or (self.early_stopping_metric_minimize and
                                       (current_value < self._best_value)) or
          (not self.early_stopping_metric_minimize and
           (current_value > self._best_value))):
        self._best_value = current_value
        self._best_value_step = step
      stop_now = (step - self._best_value_step >= self.early_stopping_rounds)
      if stop_now:
        logging.info("Stopping. Best step: {} with {} = {}."
                     .format(self._best_value_step,
                             self.early_stopping_metric, self._best_value))
        self._early_stopped = True
        return True
    return False


# TODO(ptucker): This really reads any tensor, not just vars, and requires the
# ':0' suffix on var_name.
class CaptureVariable(EveryN):
  """Captures a variable's values into a collection.

  This monitor is useful for unit testing. You should exercise caution when
  using this monitor in production, since it never discards values.

  This is an `EveryN` monitor and has consistent semantic for `every_n`
  and `first_n`.
  """

  def __init__(self, var_name, every_n=100, first_n=1):
    """Initializes a CaptureVariable monitor.

    Args:
      var_name: `string`. The variable name, including suffix (typically ":0").
      every_n: `int`, print every N steps. See `PrintN.`
      first_n: `int`, also print the first N steps. See `PrintN.`
    """
    super(CaptureVariable, self).__init__(every_n, first_n)
    self._var_name = var_name
    self._var_values = {}

  @property
  def values(self):
    """Returns the values captured so far.

    Returns:
      `dict` mapping `int` step numbers to that values of the variable at the
          respective step.
    """
    return self._var_values

  def every_n_step_begin(self, step):
    super(CaptureVariable, self).every_n_step_begin(step)
    return [self._var_name]

  def every_n_step_end(self, step, outputs):
    super(CaptureVariable, self).every_n_step_end(step, outputs)
    self._var_values[step] = _extract_output(outputs, self._var_name)


def get_default_monitors(loss_op=None, summary_op=None, save_summary_steps=100,
                         output_dir=None, summary_writer=None):
  """Returns a default set of typically-used monitors.

  Args:
    loss_op: `Tensor`, the loss tensor. This will be printed using `PrintTensor`
        at the default interval.
    summary_op: See `SummarySaver`.
    save_summary_steps: See `SummarySaver`.
    output_dir:  See `SummarySaver`.
    summary_writer:  See `SummarySaver`.
  Returns:
    `list` of monitors.
  """

  monitors = []
  if loss_op is not None:
    monitors.append(PrintTensor(tensor_names={"loss": loss_op.name}))
  if summary_op is not None:
    monitors.append(SummarySaver(summary_op, save_steps=save_summary_steps,
                                 output_dir=output_dir,
                                 summary_writer=summary_writer))
  return monitors


class GraphDump(BaseMonitor):
  """Dumps almost all tensors in the graph at every step.

  Note, this is very expensive, prefer `PrintTensor` in production.
  """

  IGNORE_OPS = ["Const", "Assign", "Identity", "Placeholder",
                "RandomUniform", "Cast", "RestoreSlice"]

  def __init__(self, ignore_ops=None):
    """Initializes GraphDump monitor.

    Args:
      ignore_ops: `list` of `string`. Names of ops to ignore.
          If None, `GraphDump.IGNORE_OPS` is used.
    """
    super(GraphDump, self).__init__()
    self._ignore_ops = ignore_ops or GraphDump.IGNORE_OPS
    self._data = {}

  def begin(self, max_steps=None):
    super(GraphDump, self).begin(max_steps=max_steps)
    self._tensors = []
    graph = ops.get_default_graph()
    graph_def = graph.as_graph_def()
    for node in graph_def.node:
      if node.op in self._ignore_ops:
        continue
      logging.info("op=%s name=%s.", node.op, node.name)
      try:
        self._tensors.append(graph.get_tensor_by_name(node.name + ":0"))
      except KeyError:
        pass

  def step_begin(self, step):
    super(GraphDump, self).step_begin(step)
    return self._tensors

  def step_end(self, step, output):
    super(GraphDump, self).step_end(step, output)
    self._data[step] = output

  @property
  def data(self):
    return self._data

  # TODO(ptucker): Handle keys that are in one but not the other.
  def compare(self, other_dump, step, atol=1e-06):
    """Compares two `GraphDump` monitors and returns differences.

    Args:
      other_dump: Another `GraphDump` monitor.
      step: `int`, step to compare on.
      atol: `float`, absolute tolerance in comparison of floating arrays.

    Returns:
      Returns tuple:
        matched: `list` of keys that matched.
        non_matched: `dict` of keys to tuple of 2 mismatched values.

    Raises:
      ValueError: if a key in `data` is missing from `other_dump` at `step`.
    """
    non_matched = {}
    matched = []
    this_output = self.data[step] if step in self.data else {}
    other_output = other_dump.data[step] if step in other_dump.data else {}
    for key in this_output:
      if not isinstance(key, str) and not isinstance(key, unicode):
        continue
      if key not in other_output:
        raise ValueError("%s missing at step %s.", (key, step))
      value1 = _extract_output(this_output, key)
      value2 = _extract_output(other_output, key)
      if isinstance(value1, str):
        continue
      if isinstance(value1, np.ndarray):
        if not np.allclose(value1, value2, atol=atol):
          non_matched[key] = value1 - value2
        else:
          matched.append(key)
      else:
        if value1 != value2:
          non_matched[key] = (value1, value2)
        else:
          matched.append(key)
    return matched, non_matched


class ExportMonitor(EveryN):
  """Monitor that exports Estimator every N steps."""

  # TODO(philstahlfeld): Investigate switching export.export_estimator
  # configuration values to **kwargs so that updates to the export_estimator
  # function don't have to be reflected here.
  def __init__(self,
               every_n_steps,
               export_dir,
               exports_to_keep=5,
               signature_fn=None,
               default_batch_size=1):
    """Initializes ExportMonitor.

    Args:
      every_n_steps: Run monitor every N steps.
      export_dir: str, folder to export.
      exports_to_keep: int, number of exports to keep.
      signature_fn: Function that given `Tensor` of `Example` strings,
        `dict` of `Tensor`s for features and `dict` of `Tensor`s for predictions
        and returns default and named exporting signautres.
      default_batch_size: Default batch size of the `Example` placeholder.
    """
    super(ExportMonitor, self).__init__(every_n_steps=every_n_steps)
    self.export_dir = export_dir
    self.exports_to_keep = exports_to_keep
    self.signature_fn = signature_fn
    self._default_batch_size = default_batch_size

  def every_n_step_end(self, step, outputs):
    super(ExportMonitor, self).every_n_step_end(step, outputs)
    try:
      export.export_estimator(self._estimator,
                              self.export_dir,
                              exports_to_keep=self.exports_to_keep,
                              signature_fn=self.signature_fn,
                              default_batch_size=self._default_batch_size)
    except (RuntimeError, TypeError):
      # Currently we are not syncronized with saving checkpoints, which leads to
      # runtime errors when we are calling export on the same global step.
      logging.info("Skipping exporting for the same step. "
                   "Consider exporting less frequently.")

  def end(self, session=None):
    super(ExportMonitor, self).end(session=session)
    latest_path = saver_lib.latest_checkpoint(self._estimator.model_dir)
    if latest_path is None:
      logging.info("Skipping export at the end since model has not been saved "
                   "yet.")
      return
    export.export_estimator(self._estimator,
                            self.export_dir,
                            exports_to_keep=self.exports_to_keep,
                            signature_fn=self.signature_fn,
                            default_batch_size=self._default_batch_size)


class CheckpointSaver(BaseMonitor):
  """Saves checkpoints every N steps."""

  def __init__(self,
               checkpoint_dir,
               save_secs=None,
               save_steps=None,
               saver=None,
               checkpoint_basename="model.ckpt",
               scaffold=None):
    """Initialize CheckpointSaver monitor.

    Args:
      checkpoint_dir: `str`, base directory for the checkpoint files.
      save_secs: `int`, save every N secs.
      save_steps: `int`, save every N steps.
      saver: `Saver` object, used for saving.
      checkpoint_basename: `str`, base name for the checkpoint files.
      scaffold: `Scaffold`, use to get saver object.

    Raises:
      ValueError: If both `save_steps` and `save_secs` are not `None`.
      ValueError: If both `save_steps` and `save_secs` are `None`.
    """
    logging.info("Create CheckpointSaver")
    super(CheckpointSaver, self).__init__()
    self._saver = saver
    self._summary_writer = SummaryWriterCache.get(checkpoint_dir)
    self._save_path = os.path.join(checkpoint_dir, checkpoint_basename)
    self._scaffold = scaffold
    self._save_secs = save_secs
    self._save_steps = save_steps
    self._last_saved_time = None
    self._last_begin_step = None
    self._last_saved_step = None

    if save_steps is None and save_secs is None:
      raise ValueError("Either save_steps or save_secs should be provided")
    if (save_steps is not None) and (save_secs is not None):
      raise ValueError("Can not provide both save_steps and save_secs.")

  def begin(self, max_steps=None):
    super(CheckpointSaver, self).begin(max_steps)
    self._last_saved_time = None
    self._last_begin_step = None
    self._last_saved_step = None

  def step_begin(self, step):
    super(CheckpointSaver, self).step_begin(step)
    self._last_begin_step = step

  def post_step(self, step, session):
    super(CheckpointSaver, self).post_step(step, session)
    if self._last_saved_time is None:
      self._save(step, session)

    if self._save_steps is not None:
      if step >= self._last_saved_step + self._save_steps:
        self._save(step, session)

    if self._save_secs is not None:
      if time.time() >= self._last_saved_time + self._save_secs:
        self._save(step, session)

  def end(self, session=None):
    super(CheckpointSaver, self).end(session)
    self._save(self._last_begin_step, session)

  def _save(self, step, session):
    """Saves the latest checkpoint."""
    if step == self._last_saved_step:
      return
    logging.info("Saving checkpoints for %d into %s.", step, self._save_path)
    self._last_saved_time = time.time()
    self._last_saved_step = step
    if self._saver is None:
      self._scaffold.saver.save(session, self._save_path, global_step=step)
    else:
      self._saver.save(session, self._save_path, global_step=step)
    self._summary_writer.add_session_log(
        SessionLog(
            status=SessionLog.CHECKPOINT, checkpoint_path=self._save_path),
        step)


class StepCounter(EveryN):
  """Steps per second monitor."""

  def __init__(self, every_n_steps=100, output_dir=None,
               summary_writer=None):
    super(StepCounter, self).__init__(every_n_steps=every_n_steps)
    self._summary_tag = "global_step/sec"
    self._last_reported_step = None
    self._last_reported_time = None
    self._summary_writer = summary_writer
    if summary_writer is None and output_dir:
      self._summary_writer = SummaryWriterCache.get(output_dir)

  def set_estimator(self, estimator):
    super(StepCounter, self).set_estimator(estimator)
    if self._summary_writer is None:
      self._summary_writer = SummaryWriterCache.get(estimator.model_dir)

  def every_n_step_end(self, current_step, outputs):
    current_time = time.time()
    if self._last_reported_time is not None and self._summary_writer:
      added_steps = current_step - self._last_reported_step
      elapsed_time = current_time - self._last_reported_time
      steps_per_sec = added_steps / elapsed_time
      summary = Summary(value=[Summary.Value(tag=self._summary_tag,
                                             simple_value=steps_per_sec)])
      self._summary_writer.add_summary(summary, current_step)
    self._last_reported_step = current_step
    self._last_reported_time = current_time


class NanLossDuringTrainingError(RuntimeError):

  def __str__(self):
    return "NaN loss during training."


class NanLoss(EveryN):
  """NaN Loss monitor.

  Monitors loss and stops training if loss is NaN.
  Can either fail with exception or just stop training.
  """

  def __init__(self, loss_tensor, every_n_steps=100, fail_on_nan_loss=True):
    """Initializes NanLoss monitor.

    Args:
      loss_tensor: `Tensor`, the loss tensor.
      every_n_steps: `int`, run check every this many steps.
      fail_on_nan_loss: `bool`, whether to raise exception when loss is NaN.
    """
    super(NanLoss, self).__init__(every_n_steps=every_n_steps)
    self._loss_tensor = loss_tensor
    self._fail_on_nan_loss = fail_on_nan_loss

  def every_n_step_begin(self, step):
    super(NanLoss, self).every_n_step_begin(step)
    return [self._loss_tensor]

  def every_n_step_end(self, step, outputs):
    super(NanLoss, self).every_n_step_end(step, outputs)
    if np.isnan(_extract_output(outputs, self._loss_tensor)):
      failure_message = "Model diverged with loss = NaN."
      if self._fail_on_nan_loss:
        logging.error(failure_message)
        raise NanLossDuringTrainingError
      else:
        logging.warning(failure_message)
        # We don't raise an error but we return "should stop" so we stop, but
        # without an exception.
        return True
