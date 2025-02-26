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

"""A client interface for TensorFlow."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import re
import threading

import numpy as np

from tensorflow.core.protobuf import config_pb2
from tensorflow.python import pywrap_tensorflow as tf_session
from tensorflow.python.framework import errors
from tensorflow.python.framework import ops
from tensorflow.python.ops import session_ops
from tensorflow.python.platform import tf_logging as logging
from tensorflow.python.util import compat
from tensorflow.python.util import nest


class SessionInterface(object):
  """Base class for implementations of TensorFlow client sessions."""

  @property
  def graph(self):
    """The underlying TensorFlow graph, to be used in building Operations."""
    raise NotImplementedError('graph')

  @property
  def sess_str(self):
    """The TensorFlow process to which this session will connect."""
    raise NotImplementedError('sess_str')

  def run(self, fetches, feed_dict=None, options=None, run_metadata=None):
    """Runs operations in the session. See `Session.run()` for details."""
    raise NotImplementedError('run')

  def partial_run_setup(self, fetches, feeds=None):
    """Sets up the feeds and fetches for partial runs in the session."""
    raise NotImplementedError('partial_run_setup')

  def partial_run(self, handle, fetches, feed_dict=None):
    """Continues the execution with additional feeds and fetches."""
    raise NotImplementedError('partial_run')

def _get_indexed_slices_value_from_fetches(fetched_vals):
  return ops.IndexedSlicesValue(fetched_vals[0], fetched_vals[1],
                                fetched_vals[2]
                                if len(fetched_vals) == 3 else None)


def _get_feeds_for_indexed_slices(feed, feed_val):
  return list(zip([feed.values, feed.indices] if feed.dense_shape is None else
                  [feed.values, feed.indices, feed.dense_shape], feed_val))


# List of extensions supported to convert run arguments into actual fetches and
# feeds.
#
# Each element in the list is a tuple of (Type, fetch_fn, feed_fn1, feed_fn2),
# where the function signatures are:
#   fetch_fn : Type -> (list of Tensors,
#                       lambda: list of fetched np.ndarray -> TypeVal)
#   feed_fn1 : Type, TypeVal -> list of (Tensor, value)
#   feed_fn2 : Type -> list of Tensors
#
# `fetch_fn` describes how to expand fetch into its
# component Tensors and how to contract the fetched results back into
# a single return value.
#
# Each feed function describes how to unpack a single fed value and map it to
# feeds of one or more tensors and their corresponding values: `feed_fn1` is
# used to feed a run, `feed_fn2` to set up a partial run.
#
# TODO(touts): We could reimplement these as specialized _FeedMapper
# implementations after we refactor the feed handling code to use them.
#
# Eventually, this registration could be opened up to support custom Tensor
# expansions.
# pylint: disable=g-long-lambda
_REGISTERED_EXPANSIONS = [
    # SparseTensors are fetched as SparseTensorValues. They can be fed
    # SparseTensorValues or normal tuples.
    (ops.SparseTensor,
     lambda fetch: (
         [fetch.indices, fetch.values, fetch.shape],
         lambda fetched_vals: ops.SparseTensorValue(*fetched_vals)),
     lambda feed, feed_val: list(zip(
         [feed.indices, feed.values, feed.shape], feed_val)),
     lambda feed: [feed.indices, feed.values, feed.shape]),
    # IndexedSlices are fetched as IndexedSlicesValues. They can be fed
    # IndexedSlicesValues or normal tuples.
    (ops.IndexedSlices,
     lambda fetch: (
         [fetch.values, fetch.indices] if fetch.dense_shape is None
         else [fetch.values, fetch.indices, fetch.dense_shape],
         _get_indexed_slices_value_from_fetches),
     _get_feeds_for_indexed_slices,
     lambda feed: [feed.values, feed.indices] if feed.dense_shape is None
                  else [feed.values, feed.indices, feed.dense_shape]),
    # The default catches all other types and performs no expansions.
    (object,
     lambda fetch: ([fetch], lambda fetched_vals: fetched_vals[0]),
     lambda feed, feed_val: [(feed, feed_val)],
     lambda feed: [feed])]
# pylint: enable=g-long-lambda


class _FetchMapper(object):
  """Definition of the interface provided by fetch mappers.

  Fetch mappers are utility classes used by the _FetchHandler to handle
  arbitrary structures for the `fetch` argument to `Session.run()`.

  The `fetch` argument can be of various shapes: single tensor or op, list of
  fetches, tuple of fetches, namedtuple of fetches, or dict of fetches.  The
  structures can be arbitrarily nested.

  The low level run() API only wants a list of tensor or op names.  The various
  `_FetchMapper` subclasses below take care of handling the different shapes:
  uniquifying the fetches, and constructing results with the original shape.
  """

  def unique_fetches(self):
    """Return the list of unique tensors or ops needed by this fetch mapper.

    Returns:
      A list of tensors or ops.
    """
    raise NotImplementedError('Must be implemented by subclasses')

  def build_results(self, values):
    """Build results that match the original shape of the fetch.

    Args:
      values: List of values returned by run(). The values correspond
        exactly to the list tensors or ops returned by unique_fetches().

    Returns:
      A struct of the same shape as the original fetch object handled by
      this fetch mapper.  In the returned struct, the original fetches are
      replaced by their fetched values.
    """
    raise NotImplementedError('Must be implemented by subclasses')

  @staticmethod
  def for_fetch(fetch):
    """Creates fetch mapper that handles the structure of `fetch`.

    The default graph must be the one from which we want to fetch values when
    this function is called.

    Args:
      fetch: An arbitrary fetch structure: singleton, list, tuple,
        namedtuple, or dict.

    Returns:
      An instance of a subclass of `_FetchMapper` that handles the shape.
    """
    if fetch is None:
      raise TypeError('Fetch argument %r has invalid type %r' %
                      (fetch, type(fetch)))
    elif isinstance(fetch, (list, tuple)):
      # NOTE(touts): This is also the code path for namedtuples.
      return _ListFetchMapper(fetch)
    elif isinstance(fetch, dict):
      return _DictFetchMapper(fetch)
    else:
      # Look for a handler in the registered expansions.
      for tensor_type, fetch_fn, _, _ in _REGISTERED_EXPANSIONS:
        if isinstance(fetch, tensor_type):
          fetches, contraction_fn = fetch_fn(fetch)
          return _ElementFetchMapper(fetches, contraction_fn)
    # Did not find anything.
    raise TypeError('Fetch argument %r has invalid type %r' %
                    (fetch, type(fetch)))


class _ElementFetchMapper(_FetchMapper):
  """Fetch mapper for singleton tensors and ops."""

  def __init__(self, fetches, contraction_fn):
    """Creates an _ElementFetchMapper.

    This is the fetch mapper used for leaves in the fetch struct.  Because of
    the expansions mechanism, a leaf can actually fetch more than one tensor.

    Also note that the fetches here can be just strings (tensor or op names) or
    any other object that the graph knows how to convert to a tensor, such as a
    Variable.  So we have to run each fetch through `as_graph_element()` to get
    the corresponding tensor or op.

    Args:
      fetches: List of objects, as returned by a fetch_fn defined
        in _REGISTERED_EXPANSIONS.
      contraction_fn: Callable as returned by a fetch_fn.
    """
    self._unique_fetches = []
    for fetch in fetches:
      try:
        self._unique_fetches.append(ops.get_default_graph().as_graph_element(
            fetch, allow_tensor=True, allow_operation=True))
      except TypeError as e:
        raise TypeError('Fetch argument %r has invalid type %r, '
                        'must be a string or Tensor. (%s)'
                        % (fetch, type(fetch), str(e)))
      except ValueError as e:
        raise ValueError('Fetch argument %r cannot be interpreted as a '
                         'Tensor. (%s)' % (fetch, str(e)))
      except KeyError as e:
        raise ValueError('Fetch argument %r cannot be interpreted as a '
                         'Tensor. (%s)' % (fetch, str(e)))
    self._contraction_fn = contraction_fn

  def unique_fetches(self):
    return self._unique_fetches

  def build_results(self, values):
    if not values:
      # 'Operation' case
      return None
    else:
      return self._contraction_fn(values)


def _uniquify_fetches(fetch_mappers):
  """Uniquifies fetches from a list of fetch_mappers.

  This is a utility function used by _ListFetchMapper and _DictFetchMapper.  It
  gathers all the unique fetches from a list of mappers and builds a list
  containing all of them but without duplicates (unique_fetches).

  It also returns a 2-D list of integers (values_indices) indicating at which
  index in unique_fetches the fetches of the mappers are located.

  This list is as follows:
    values_indices[mapper_index][mapper_fetch_index] = unique_fetches_index

  Args:
    fetch_mappers: list of fetch mappers.

  Returns:
    A list of fetches.
    A 2-D list of integers.
  """
  unique_fetches = []
  value_indices = []
  seen_fetches = {}
  for m in fetch_mappers:
    m_value_indices = []
    for f in m.unique_fetches():
      j = seen_fetches.get(f)
      if j is None:
        j = len(seen_fetches)
        seen_fetches[f] = j
        unique_fetches.append(f)
      m_value_indices.append(j)
    value_indices.append(m_value_indices)
  return unique_fetches, value_indices


class _ListFetchMapper(_FetchMapper):
  """Fetch mapper for lists, tuples, and namedtuples."""

  def __init__(self, fetches):
    """Creates a _ListFetchMapper.

    Args:
      fetches: List, tuple, or namedtuple of fetches.
    """
    self._fetch_type = type(fetches)
    self._mappers = [_FetchMapper.for_fetch(fetch) for fetch in fetches]
    self._unique_fetches, self._value_indices = _uniquify_fetches(self._mappers)

  def unique_fetches(self):
    return self._unique_fetches

  def build_results(self, values):
    # Create the list of results for each mapper.
    results = []
    for m, vi in zip(self._mappers, self._value_indices):
      results.append(m.build_results([values[j] for j in vi]))
    # Return a value of the original type of the fetches.
    if self._fetch_type == list:
      return results
    elif self._fetch_type == tuple:
      return tuple(results)
    else:
      # This is the code path for namedtuple.
      return self._fetch_type(*results)


class _DictFetchMapper(_FetchMapper):
  """Fetch mapper for dicts."""

  def __init__(self, fetches):
    """Creates a _DictFetchMapper.

    Args:
      fetches: Dict of fetches.
    """
    self._keys = fetches.keys()
    self._mappers = [_FetchMapper.for_fetch(fetch)
                     for fetch in fetches.values()]
    self._unique_fetches, self._value_indices = _uniquify_fetches(self._mappers)

  def unique_fetches(self):
    return self._unique_fetches

  def build_results(self, values):
    results = {}
    for k, m, vi in zip(self._keys, self._mappers, self._value_indices):
      results[k] = m.build_results([values[j] for j in vi])
    return results


class _FetchHandler(object):
  """Handler for structured fetches.

  Given a graph, a user-provided structure for fetches, and a feed dict, this
  class takes care of generating a list of tensor names to fetch and op names
  to run for a low level `run()` call.

  Given the results of the low level run call, this class can also rebuild a
  result structure matching the user-provided structure for fetches, but
  containing the corresponding results.
  """
  # TODO(touts): Make this class also take care of destructuring the feed
  # dict instead of doing it in the callers.

  def __init__(self, graph, fetches, feeds):
    """Creates a fetch handler.

    Args:
      graph: Graph of the fetches.   Used to check for fetchability
        and to convert all fetches to tensors or ops as needed.
      fetches: An arbitrary fetch structure: singleton, list, tuple,
        namedtuple, or dict.
      feeds: A feed dict where keys are fully resolved tensor names.
    """
    with graph.as_default():
      self._fetch_mapper = _FetchMapper.for_fetch(fetches)
    self._fetches = []
    self._targets = []
    self._feeds = feeds
    self._ops = []
    self._fetch_handles = {}
    for fetch in self._fetch_mapper.unique_fetches():
      fetch_name = compat.as_bytes(fetch.name)
      if isinstance(fetch, ops.Operation):
        self._assert_fetchable(graph, fetch)
        self._targets.append(fetch_name)
        self._ops.append(True)
      else:
        self._assert_fetchable(graph, fetch.op)
        self._fetches.append(fetch_name)
        self._ops.append(False)
      # Remember the fetch if it is for a tensor handle.
      if isinstance(fetch, ops.Tensor) and fetch.op.type == 'GetSessionHandle':
        self._fetch_handles[fetch_name] = fetch.op.inputs[0].dtype
    self._final_fetches = [x for x in self._fetches if x not in feeds]

  def _assert_fetchable(self, graph, op):
    if not graph.is_fetchable(op):
      raise ValueError(
          'Operation %r has been marked as not fetchable.' % op.name)

  def fetches(self):
    """Return the unique names of tensors to fetch.

    Returns:
      A list of strings.
    """
    return self._final_fetches

  def targets(self):
    """Return the unique names of ops to run.

    Returns:
      A list of strings.
    """
    return self._targets

  def build_results(self, session, tensor_values):
    """Build results matching the original fetch shape.

    `tensor_values` must be a list of the same length as
    the one returned by `fetches()`, and holding the requested
    fetch values.

    This method builds a struct with the same shape as the original `fetches`
    passed to the constructor, in which the fetches are replaced by their
    fetched value.

    Args:
      session: The enclosing session.  Used for tensor handles.
      tensor_values: List of values matching the list returned
        by fetches().

    Returns:
      A structure of the same shape as the original `fetches` argument but
        containing tensors or None (for fetched ops).
    """
    full_values = []
    assert len(self._final_fetches) == len(tensor_values)
    i = 0
    j = 0
    for is_op in self._ops:
      if is_op:
        full_values.append(None)
      else:
        # If the fetch was in the feeds, use the fed value, otherwise
        # use the returned value.
        value = self._feeds.get(self._fetches[i])
        if value is None:
          value = tensor_values[j]
          j += 1
        dtype = self._fetch_handles.get(self._fetches[i])
        if dtype:
          full_values.append(session_ops.TensorHandle(value, dtype, session))
        else:
          full_values.append(value)
        i += 1
    assert j == len(tensor_values)
    return self._fetch_mapper.build_results(full_values)


class BaseSession(SessionInterface):
  """A class for interacting with a TensorFlow computation.

  The BaseSession enables incremental graph building with inline
  execution of Operations and evaluation of Tensors.
  """

  def __init__(self, target='', graph=None, config=None):
    """Constructs a new TensorFlow session.

    Args:
      target: (Optional) The TensorFlow execution engine to connect to.
      graph: (Optional) The graph to be used. If this argument is None,
        the default graph will be used.
      config: (Optional) ConfigProto proto used to configure the session.

    Raises:
      tf.errors.OpError: Or one of its subclasses if an error occurs while
        creating the TensorFlow session.
      TypeError: If one of the arguments has the wrong type.
    """
    if graph is None:
      self._graph = ops.get_default_graph()
    else:
      if not isinstance(graph, ops.Graph):
        raise TypeError('graph must be a tf.Graph, but got %s' % type(graph))
      self._graph = graph

    self._opened = False
    self._closed = False

    self._current_version = 0
    self._extend_lock = threading.Lock()
    if target is not None:
      try:
        self._target = compat.as_bytes(target)
      except TypeError:
        raise TypeError('target must be a string, but got %s' % type(target))
    else:
      self._target = None

    self._delete_lock = threading.Lock()
    self._dead_handles = []

    if config is not None:
      if not isinstance(config, config_pb2.ConfigProto):
        raise TypeError('config must be a tf.ConfigProto, but got %s'
                        % type(config))
      self._config = config
      self._add_shapes = config.graph_options.infer_shapes
    else:
      self._config = None
      self._add_shapes = False

    self._session = None
    opts = tf_session.TF_NewSessionOptions(target=self._target, config=config)
    try:
      with errors.raise_exception_on_not_ok_status() as status:
        self._session = tf_session.TF_NewSession(opts, status)
    finally:
      tf_session.TF_DeleteSessionOptions(opts)

  def close(self):
    """Closes this session.

    Calling this method frees all resources associated with the session.

    Raises:
      tf.errors.OpError: Or one of its subclasses if an error occurs while
        closing the TensorFlow session.
    """
    with self._extend_lock:
      if self._opened and not self._closed:
        self._closed = True
        with errors.raise_exception_on_not_ok_status() as status:
          tf_session.TF_CloseSession(self._session, status)

  def __del__(self):
    self.close()
    if self._session is not None:
      with errors.raise_exception_on_not_ok_status() as status:
        tf_session.TF_DeleteSession(self._session, status)
      self._session = None

  @property
  def graph(self):
    """The graph that was launched in this session."""
    return self._graph

  @property
  def graph_def(self):
    """A serializable version of the underlying TensorFlow graph.

    Returns:
      A graph_pb2.GraphDef proto containing nodes for all of the Operations in
      the underlying TensorFlow graph.
    """
    return self._graph.as_graph_def(add_shapes=self._add_shapes)

  @property
  def sess_str(self):
    return self._target

  def as_default(self):
    """Returns a context manager that makes this object the default session.

    Use with the `with` keyword to specify that calls to
    [`Operation.run()`](../../api_docs/python/framework.md#Operation.run) or
    [`Tensor.eval()`](../../api_docs/python/framework.md#Tensor.eval) should be
    executed in this session.

    ```python
    c = tf.constant(..)
    sess = tf.Session()

    with sess.as_default():
      assert tf.get_default_session() is sess
      print(c.eval())
    ```

    To get the current default session, use
    [`tf.get_default_session()`](#get_default_session).


    *N.B.* The `as_default` context manager *does not* close the
    session when you exit the context, and you must close the session
    explicitly.

    ```python
    c = tf.constant(...)
    sess = tf.Session()
    with sess.as_default():
      print(c.eval())
    # ...
    with sess.as_default():
      print(c.eval())

    sess.close()
    ```

    Alternatively, you can use `with tf.Session():` to create a
    session that is automatically closed on exiting the context,
    including when an uncaught exception is raised.

    *N.B.* The default graph is a property of the current thread. If you
    create a new thread, and wish to use the default session in that
    thread, you must explicitly add a `with sess.as_default():` in that
    thread's function.

    Returns:
      A context manager using this session as the default session.

    """
    return ops.default_session(self)

  def run(self, fetches, feed_dict=None, options=None, run_metadata=None):
    """Runs operations and evaluates tensors in `fetches`.

    This method runs one "step" of TensorFlow computation, by
    running the necessary graph fragment to execute every `Operation`
    and evaluate every `Tensor` in `fetches`, substituting the values in
    `feed_dict` for the corresponding input values.

    The `fetches` argument may be a single graph element, or an arbitrarily
    nested list, tuple, namedtuple, or dict containing graph elements at its
    leaves.  A graph element can be one of the following types:

    * An [`Operation`](../../api_docs/python/framework.md#Operation).
      The corresponding fetched value will be `None`.
    * A [`Tensor`](../../api_docs/python/framework.md#Tensor).
      The corresponding fetched value will be a numpy ndarray containing the
      value of that tensor.
    * A [`SparseTensor`](../../api_docs/python/sparse_ops.md#SparseTensor).
      The corresponding fetched value will be a
      [`SparseTensorValue`](../../api_docs/python/sparse_ops.md#SparseTensorValue)
      containing the value of that sparse tensor.
    * A `get_tensor_handle` op.  The corresponding fetched value will be a
      numpy ndarray containing the handle of that tensor.
    * A `string` which is the name of a tensor or operation in the graph.

    The value returned by `run()` has the same shape as the `fetches` argument,
    where the leaves are replaced by the corresponding values returned by
    TensorFlow.

    Example:

    ```python
       a = tf.constant([10, 20])
       b = tf.constant([1.0, 2.0])
       # 'fetches' can be a singleton
       v = session.run(a)
       # v is the numpy array [10, 20]
       # 'fetches' can be a list.
       v = session.run([a, b])
       # v a Python list with 2 numpy arrays: the numpy array [10, 20] and the
       # 1-D array [1.0, 2.0]
       # 'fetches' can be arbitrary lists, tuples, namedtuple, dicts:
       MyData = collections.namedtuple('MyData', ['a', 'b'])
       v = session.run({'k1': MyData(a, b), 'k2': [b, a]})
       # v is a dict with
       # v['k1'] is a MyData namedtuple with 'a' the numpy array [10, 20] and
       # 'b' the numpy array [1.0, 2.0]
       # v['k2'] is a list with the numpy array [1.0, 2.0] and the numpy array
       # [10, 20].
    ```

    The optional `feed_dict` argument allows the caller to override
    the value of tensors in the graph. Each key in `feed_dict` can be
    one of the following types:

    * If the key is a [`Tensor`](../../api_docs/python/framework.md#Tensor), the
      value may be a Python scalar, string, list, or numpy ndarray
      that can be converted to the same `dtype` as that
      tensor. Additionally, if the key is a
      [placeholder](../../api_docs/python/io_ops.md#placeholder), the shape of
      the value will be checked for compatibility with the placeholder.
    * If the key is a
      [`SparseTensor`](../../api_docs/python/sparse_ops.md#SparseTensor),
      the value should be a
      [`SparseTensorValue`](../../api_docs/python/sparse_ops.md#SparseTensorValue).
    * If the key is a nested tuple of `Tensor`s or `SparseTensor`s, the value
      should be a nested tuple with the same structure that maps to their
      corresponding values as above.

    Each value in `feed_dict` must be convertible to a numpy array of the dtype
    of the corresponding key.

    The optional `options` argument expects a [`RunOptions`] proto. The options
    allow controlling the behavior of this particular step (e.g. turning tracing
    on).

    The optional `run_metadata` argument expects a [`RunMetadata`] proto. When
    appropriate, the non-Tensor output of this step will be collected there. For
    example, when users turn on tracing in `options`, the profiled info will be
    collected into this argument and passed back.

    Args:
      fetches: A single graph element, a list of graph elements,
        or a dictionary whose values are graph elements or lists of graph
        elements (described above).
      feed_dict: A dictionary that maps graph elements to values
        (described above).
      options: A [`RunOptions`] protocol buffer
      run_metadata: A [`RunMetadata`] protocol buffer

    Returns:
      Either a single value if `fetches` is a single graph element, or
      a list of values if `fetches` is a list, or a dictionary with the
      same keys as `fetches` if that is a dictionary (described above).

    Raises:
      RuntimeError: If this `Session` is in an invalid state (e.g. has been
        closed).
      TypeError: If `fetches` or `feed_dict` keys are of an inappropriate type.
      ValueError: If `fetches` or `feed_dict` keys are invalid or refer to a
        `Tensor` that doesn't exist.
    """
    run_metadata_ptr = tf_session.TF_NewBuffer()
    if options:
      options_ptr = tf_session.TF_NewBufferFromString(
          compat.as_bytes(options.SerializeToString()))
    else:
      options_ptr = None

    try:
      result = self._run(None, fetches, feed_dict, options_ptr,
                         run_metadata_ptr)
      if run_metadata:
        proto_data = tf_session.TF_GetBuffer(run_metadata_ptr)
        run_metadata.ParseFromString(compat.as_bytes(proto_data))
    finally:
      tf_session.TF_DeleteBuffer(run_metadata_ptr)
      if options:
        tf_session.TF_DeleteBuffer(options_ptr)
    return result

  def partial_run(self, handle, fetches, feed_dict=None):
    """Continues the execution with more feeds and fetches.

    This is EXPERIMENTAL and subject to change.

    To use partial execution, a user first calls `partial_run_setup()` and
    then a sequence of `partial_run()`. `partial_run_setup` specifies the
    list of feeds and fetches that will be used in the subsequent
    `partial_run` calls.

    The optional `feed_dict` argument allows the caller to override
    the value of tensors in the graph. See run() for more information.

    Below is a simple example:

    ```python
    a = array_ops.placeholder(dtypes.float32, shape=[])
    b = array_ops.placeholder(dtypes.float32, shape=[])
    c = array_ops.placeholder(dtypes.float32, shape=[])
    r1 = math_ops.add(a, b)
    r2 = math_ops.mul(r1, c)

    h = sess.partial_run_setup([r1, r2], [a, b, c])
    res = sess.partial_run(h, r1, feed_dict={a: 1, b: 2})
    res = sess.partial_run(h, r2, feed_dict={c: res})
    ```

    Args:
      handle: A handle for a sequence of partial runs.
      fetches: A single graph element, a list of graph elements,
        or a dictionary whose values are graph elements or lists of graph
        elements (see documentation for `run`).
      feed_dict: A dictionary that maps graph elements to values
        (described above).

    Returns:
      Either a single value if `fetches` is a single graph element, or
      a list of values if `fetches` is a list, or a dictionary with the
      same keys as `fetches` if that is a dictionary
      (see documentation for `run`).

    Raises:
      tf.errors.OpError: Or one of its subclasses on error.
    """
    # TODO(touts): Support feeding and fetching the same tensor.
    return self._run(handle, fetches, feed_dict, None, None)

  def partial_run_setup(self, fetches, feeds=None):
    """Sets up a graph with feeds and fetches for partial run.

    This is EXPERIMENTAL and subject to change.

    Note that contrary to `run`, `feeds` only specifies the graph elements.
    The tensors will be supplied by the subsequent `partial_run` calls.

    Args:
      fetches: A single graph element, or a list of graph elements.
      feeds: A single graph element, or a list of graph elements.

    Returns:
      A handle for partial run.

    Raises:
      RuntimeError: If this `Session` is in an invalid state (e.g. has been
        closed).
      TypeError: If `fetches` or `feed_dict` keys are of an inappropriate type.
      tf.errors.OpError: Or one of its subclasses if a TensorFlow error happens.
    """
    def _feed_fn(feed):
      for tensor_type, _, _, feed_fn in _REGISTERED_EXPANSIONS:
        if isinstance(feed, tensor_type):
          return feed_fn(feed)
      raise TypeError('Feed argument %r has invalid type %r'
                      % (feed, type(feed)))

    # Check session.
    if self._closed:
      raise RuntimeError('Attempted to use a closed Session.')
    if self.graph.version == 0:
      raise RuntimeError('The Session graph is empty.  Add operations to the '
                         'graph before calling run().')

    # Create request.
    feed_list = []

    # Validate and process feed_list.
    is_list_feed = isinstance(feeds, (list, tuple))
    if not is_list_feed:
      feeds = [feeds]
    for feed in feeds:
      for subfeed in _feed_fn(feed):
        try:
          subfeed_t = self.graph.as_graph_element(subfeed, allow_tensor=True,
                                                  allow_operation=False)
          feed_list.append(compat.as_bytes(subfeed_t.name))
        except Exception as e:
          e.message = ('Cannot interpret feed_list key as Tensor: '
                       + e.message)
          e.args = (e.message,)
          raise e

    # Validate and process fetches.
    # TODO(touts): Support feeding and fetching the same tensor.
    fetch_handler = _FetchHandler(self._graph, fetches, {})

    # Set up a graph with feeds and fetches for partial run.
    def _setup_fn(session, feed_list, fetch_list, target_list):
      self._extend_graph()
      with errors.raise_exception_on_not_ok_status() as status:
        return tf_session.TF_PRunSetup(session, feed_list, fetch_list,
                                       target_list, status)

    return self._do_call(_setup_fn, self._session, feed_list,
                         fetch_handler.fetches(), fetch_handler.targets())

  def _run(self, handle, fetches, feed_dict, options, run_metadata):
    """Perform either run or partial_run, depending the presence of `handle`."""
    def _feed_fn(feed, feed_val):
      for tensor_type, _, feed_fn, _ in _REGISTERED_EXPANSIONS:
        if isinstance(feed, tensor_type):
          return feed_fn(feed, feed_val)
      raise TypeError('Feed argument %r has invalid type %r'
                      % (feed, type(feed)))

    # Check session.
    if self._closed:
      raise RuntimeError('Attempted to use a closed Session.')
    if self.graph.version == 0:
      raise RuntimeError('The Session graph is empty.  Add operations to the '
                         'graph before calling run().')

    # Create request.
    feed_dict_string = {}
    feed_map = {}

    # Validate and process feed_dict.
    if feed_dict:
      feed_dict = nest.flatten_dict_items(feed_dict)
      for feed, feed_val in feed_dict.items():
        for subfeed, subfeed_val in _feed_fn(feed, feed_val):
          try:
            subfeed_t = self.graph.as_graph_element(subfeed, allow_tensor=True,
                                                    allow_operation=False)
          except Exception as e:
            raise TypeError('Cannot interpret feed_dict key as Tensor: '
                            + e.args[0])

          if isinstance(subfeed_val, ops.Tensor):
            raise TypeError('The value of a feed cannot be a tf.Tensor object. '
                            'Acceptable feed values include Python scalars, '
                            'strings, lists, or numpy ndarrays.')

          subfeed_dtype = subfeed_t.dtype.as_numpy_dtype
          if isinstance(subfeed_val,
                        int) and subfeed_dtype(subfeed_val) != subfeed_val:
            raise TypeError(
                'Type of feed value ' + str(subfeed_val) + ' is not'
                ' compatible with Tensor type ' + str(subfeed_dtype) + '.'
                ' Try explicitly setting the type of the feed tensor'
                ' to a larger type (e.g. int64).')

          np_val = np.asarray(subfeed_val, dtype=subfeed_dtype)

          if not subfeed_t.get_shape().is_compatible_with(np_val.shape):
            raise ValueError(
                'Cannot feed value of shape %r for Tensor %r, '
                'which has shape %r'
                % (np_val.shape, subfeed_t.name, str(subfeed_t.get_shape())))
          if not self.graph.is_feedable(subfeed_t):
            raise ValueError('Tensor %s may not be fed.' % subfeed_t)
          subfeed_name = compat.as_bytes(subfeed_t.name)
          feed_dict_string[subfeed_name] = np_val
          feed_map[subfeed_name] = (subfeed_t, subfeed_val)

    # Create a fetch handler to take care of the structure of fetches.
    fetch_handler = _FetchHandler(self._graph, fetches, feed_dict_string)

    # Run request and get response.
    # We need to keep the movers alive for the following _do_run().
    # These movers are no longer needed when _do_run() completes, and
    # are deleted when `movers` goes out of scope when this _run() ends.
    # TODO(yuanbyu, keveman): Revisit whether we should just treat feeding
    # of a handle from a different device as an error.
    movers = self._update_with_movers(feed_dict_string, feed_map)
    final_fetches = fetch_handler.fetches()
    final_targets = fetch_handler.targets()
    if final_fetches or final_targets:
      results = self._do_run(handle, final_targets, final_fetches,
                             feed_dict_string, options, run_metadata)
    else:
      results = []
    return fetch_handler.build_results(self, results)

  # Captures the name of a node in an error status.
  _NODEDEF_NAME_RE = re.compile(r'\[\[Node: ([^ ]*?) =')

  def _do_run(self, handle, target_list, fetch_list, feed_dict,
              options, run_metadata):
    """Runs a step based on the given fetches and feeds.

    Args:
      handle: a handle for partial_run. None if this is just a call to run().
      target_list: A list of byte arrays corresponding to names of tensors
        or operations to be run to, but not fetched.
      fetch_list: A list of byte arrays corresponding to names of tensors to
        be fetched and operations to be run.
      feed_dict: A dictionary that maps tensor names (as byte arrays) to
        numpy ndarrays.
      options: A (pointer to a) [`RunOptions`] protocol buffer, or None
      run_metadata: A (pointer to a) [`RunMetadata`] protocol buffer, or None

    Returns:
      A list of numpy ndarrays, corresponding to the elements of
      `fetch_list`.  If the ith element of `fetch_list` contains the
      name of an operation, the first Tensor output of that operation
      will be returned for that element.

    Raises:
      tf.errors.OpError: Or one of its subclasses on error.
    """
    def _run_fn(session, feed_dict, fetch_list, target_list, options,
                run_metadata):
      # Ensure any changes to the graph are reflected in the runtime.
      self._extend_graph()
      with errors.raise_exception_on_not_ok_status() as status:
        return tf_session.TF_Run(session, options,
                                 feed_dict, fetch_list, target_list,
                                 status, run_metadata)

    def _prun_fn(session, handle, feed_dict, fetch_list):
      if target_list:
        raise RuntimeError('partial_run() requires empty target_list.')
      with errors.raise_exception_on_not_ok_status() as status:
        return tf_session.TF_PRun(session, handle, feed_dict, fetch_list,
                                  status)

    if handle is None:
      return self._do_call(_run_fn, self._session, feed_dict, fetch_list,
                           target_list, options, run_metadata)
    else:
      return self._do_call(_prun_fn, self._session, handle, feed_dict,
                           fetch_list)

  def _do_call(self, fn, *args):
    try:
      return fn(*args)
    except errors.OpError as e:
      message = compat.as_text(e.message)
      m = BaseSession._NODEDEF_NAME_RE.search(message)
      node_def = None
      op = None
      if m is not None:
        node_name = m.group(1)
        try:
          op = self._graph.get_operation_by_name(node_name)
          node_def = op.node_def
        except KeyError:
          pass
      raise type(e)(node_def, op, message)

  def _extend_graph(self):
    # Ensure any changes to the graph are reflected in the runtime.
    with self._extend_lock:
      if self._graph.version > self._current_version:
        # pylint: disable=protected-access
        graph_def, self._current_version = self._graph._as_graph_def(
            from_version=self._current_version,
            add_shapes=self._add_shapes)
        # pylint: enable=protected-access

        with errors.raise_exception_on_not_ok_status() as status:
          tf_session.TF_ExtendGraph(
              self._session, graph_def.SerializeToString(), status)
        self._opened = True

  # The threshold to run garbage collection to delete dead tensors.
  _DEAD_HANDLES_THRESHOLD = 10

  def _register_dead_handle(self, handle):
    # Register a dead handle in the session. Delete the dead tensors when
    # the number of dead tensors exceeds certain threshold.
    tensors_to_delete = None
    with self._delete_lock:
      self._dead_handles.append(handle)
      if len(self._dead_handles) == BaseSession._DEAD_HANDLES_THRESHOLD:
        tensors_to_delete = self._dead_handles
        self._dead_handles = []
    # Delete the dead tensors.
    # TODO(yuanbyu): For now we use a sequence of runs to minimize the graph
    # size and the overhead of graph construction/partitioning.
    if tensors_to_delete:
      for tensor_handle in tensors_to_delete:
        feeds = {}
        fetches = []
        holder, deleter = session_ops._get_handle_deleter(self.graph,
                                                          tensor_handle)
        feeds[holder] = tensor_handle
        fetches.append(deleter)
        self.run(fetches, feed_dict=feeds)

  def _update_with_movers(self, feed_dict, feed_map):
    # If a tensor handle that is fed to a device incompatible placeholder,
    # we move the tensor to the right device, generate a new tensor handle,
    # and update `feed_dict` to use the new handle.
    handle_movers = []
    for feed_name, val in feed_map.items():
      mover = session_ops._get_handle_mover(self.graph, *val)
      if mover:
        handle_movers.append((feed_name, val[1], mover))
    # Transfer a tensor to the right device if needed.
    if not handle_movers:
      return []
    else:
      feeds = {}
      fetches = []
      for _, handle, mover in handle_movers:
        feeds[mover[0]] = handle
        fetches.append(mover[1])
      handles = self.run(fetches, feed_dict=feeds)
      for handle_mover, handle in zip(handle_movers, handles):
        np_val = np.array(handle.handle, dtype=np.object)
        feed_dict[handle_mover[0]] = np_val
      return handles


class Session(BaseSession):
  """A class for running TensorFlow operations.

  A `Session` object encapsulates the environment in which `Operation`
  objects are executed, and `Tensor` objects are evaluated. For
  example:

  ```python
  # Build a graph.
  a = tf.constant(5.0)
  b = tf.constant(6.0)
  c = a * b

  # Launch the graph in a session.
  sess = tf.Session()

  # Evaluate the tensor `c`.
  print(sess.run(c))
  ```

  A session may own resources, such as
  [variables](../../api_docs/python/state_ops.md#Variable), [queues](../../api_docs/python/io_ops.md#QueueBase),
  and [readers](../../api_docs/python/io_ops.md#ReaderBase). It is important to release
  these resources when they are no longer required. To do this, either
  invoke the [`close()`](#Session.close) method on the session, or use
  the session as a context manager. The following two examples are
  equivalent:

  ```python
  # Using the `close()` method.
  sess = tf.Session()
  sess.run(...)
  sess.close()

  # Using the context manager.
  with tf.Session() as sess:
    sess.run(...)
  ```

  The [`ConfigProto`]
  (https://www.tensorflow.org/code/tensorflow/core/protobuf/config.proto)
  protocol buffer exposes various configuration options for a
  session. For example, to create a session that uses soft constraints
  for device placement, and log the resulting placement decisions,
  create a session as follows:

  ```python
  # Launch the graph in a session that allows soft device placement and
  # logs the placement decisions.
  sess = tf.Session(config=tf.ConfigProto(allow_soft_placement=True,
                                          log_device_placement=True))
  ```

  @@__init__
  @@run
  @@close

  @@graph

  @@as_default

  @@reset

  """

  def __init__(self, target='', graph=None, config=None):
    """Creates a new TensorFlow session.

    If no `graph` argument is specified when constructing the session,
    the default graph will be launched in the session. If you are
    using more than one graph (created with `tf.Graph()` in the same
    process, you will have to use different sessions for each graph,
    but each graph can be used in multiple sessions. In this case, it
    is often clearer to pass the graph to be launched explicitly to
    the session constructor.

    Args:
      target: (Optional.) The execution engine to connect to.
        Defaults to using an in-process engine. See [Distributed Tensorflow]
        (https://www.tensorflow.org/how_tos/distributed/index.html)
        for more examples.
      graph: (Optional.) The `Graph` to be launched (described above).
      config: (Optional.) A [`ConfigProto`](https://www.tensorflow.org/code/tensorflow/core/protobuf/config.proto)
        protocol buffer with configuration options for the session.

    """
    super(Session, self).__init__(target, graph, config=config)
    self._context_managers = [self.graph.as_default(), self.as_default()]

  def __enter__(self):
    for context_manager in self._context_managers:
      context_manager.__enter__()
    return self

  def __exit__(self, exec_type, exec_value, exec_tb):
    if exec_type is errors.OpError:
      logging.error('Session closing due to OpError: %s', (exec_value,))

    for context_manager in reversed(self._context_managers):
      context_manager.__exit__(exec_type, exec_value, exec_tb)

    self.close()

  @staticmethod
  def reset(target, containers=None, config=None):
    """Resets resource containers on `target`, and close all connected sessions.

    A resource container is distributed across all workers in the
    same cluster as `target`.  When a resource container on `target`
    is reset, resources associated with that container will be cleared.
    In particular, all Variables in the container will become undefined:
    they lose their values and shapes.

    NOTE:
    (i) reset() is currently only implemented for distributed sessions.
    (ii) Any sessions on the master named by `target` will be closed.

    If no resource containers are provided, all containers are reset.

    Args:
      target: The execution engine to connect to.
      containers: A list of resource container name strings, or `None` if all of
        all the containers are to be reset.
      config: (Optional.) Protocol buffer with configuration options.

    Raises:
      tf.errors.OpError: Or one of its subclasses if an error occurs while
        resetting containers.
    """
    if target is not None:
      target = compat.as_bytes(target)
    if containers is not None:
      containers = [compat.as_bytes(c) for c in containers]
    else:
      containers = []
    tf_session.TF_Reset(target, containers, config)


class InteractiveSession(BaseSession):
  """A TensorFlow `Session` for use in interactive contexts, such as a shell.

  The only difference with a regular `Session` is that an `InteractiveSession`
  installs itself as the default session on construction.
  The methods [`Tensor.eval()`](../../api_docs/python/framework.md#Tensor.eval)
  and [`Operation.run()`](../../api_docs/python/framework.md#Operation.run)
  will use that session to run ops.

  This is convenient in interactive shells and [IPython
  notebooks](http://ipython.org), as it avoids having to pass an explicit
  `Session` object to run ops.

  For example:

  ```python
  sess = tf.InteractiveSession()
  a = tf.constant(5.0)
  b = tf.constant(6.0)
  c = a * b
  # We can just use 'c.eval()' without passing 'sess'
  print(c.eval())
  sess.close()
  ```

  Note that a regular session installs itself as the default session when it
  is created in a `with` statement.  The common usage in non-interactive
  programs is to follow that pattern:

  ```python
  a = tf.constant(5.0)
  b = tf.constant(6.0)
  c = a * b
  with tf.Session():
    # We can also use 'c.eval()' here.
    print(c.eval())
  ```

  @@__init__
  @@close
  """

  def __init__(self, target='', graph=None, config=None):
    """Creates a new interactive TensorFlow session.

    If no `graph` argument is specified when constructing the session,
    the default graph will be launched in the session. If you are
    using more than one graph (created with `tf.Graph()` in the same
    process, you will have to use different sessions for each graph,
    but each graph can be used in multiple sessions. In this case, it
    is often clearer to pass the graph to be launched explicitly to
    the session constructor.

    Args:
      target: (Optional.) The execution engine to connect to.
        Defaults to using an in-process engine.
      graph: (Optional.) The `Graph` to be launched (described above).
      config: (Optional) `ConfigProto` proto used to configure the session.
    """
    if not config:
      config = config_pb2.ConfigProto()
    # Interactive sessions always place pruned graphs.
    config.graph_options.place_pruned_graph = True

    super(InteractiveSession, self).__init__(target, graph, config)
    self._default_session = self.as_default()
    self._default_session.enforce_nesting = False
    self._default_session.__enter__()
    self._explicit_graph = graph
    if self._explicit_graph is not None:
      self._default_graph = graph.as_default()
      self._default_graph.enforce_nesting = False
      self._default_graph.__enter__()

  def close(self):
    """Closes an `InteractiveSession`."""
    super(InteractiveSession, self).close()
    if self._explicit_graph is not None:
      self._default_graph.__exit__(None, None, None)
    self._default_session.__exit__(None, None, None)
