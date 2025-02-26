Class wrapping dynamic-sized, per-time-step, write-once Tensor arrays.

This class is meant to be used with dynamic iteration primitives such as
`while_loop` and `map_fn`.  It supports gradient back-propagation via special
"flow" control flow dependencies.

- - -

#### `tf.TensorArray.handle` {#TensorArray.handle}

The reference to the TensorArray.


- - -

#### `tf.TensorArray.flow` {#TensorArray.flow}

The flow `Tensor` forcing ops leading to this TensorArray state.



- - -

#### `tf.TensorArray.read(index, name=None)` {#TensorArray.read}

Read the value at location `index` in the TensorArray.

##### Args:


*  <b>`index`</b>: 0-D.  int32 tensor with the index to read from.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  The tensor at index `index`.


- - -

#### `tf.TensorArray.unpack(value, name=None)` {#TensorArray.unpack}

Pack the values of a `Tensor` in the TensorArray.

##### Args:


*  <b>`value`</b>: (N+1)-D.  Tensor of type `dtype`.  The Tensor to unpack.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A new TensorArray object with flow that ensures the unpack occurs.
  Use this object all for subsequent operations.

##### Raises:


*  <b>`ValueError`</b>: if the shape inference fails.


- - -

#### `tf.TensorArray.split(value, lengths, name=None)` {#TensorArray.split}

Split the values of a `Tensor` into the TensorArray.

##### Args:


*  <b>`value`</b>: (N+1)-D.  Tensor of type `dtype`.  The Tensor to split.
*  <b>`lengths`</b>: 1-D.  int32 vector with the lengths to use when splitting
    `value` along its first dimension.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A new TensorArray object with flow that ensures the split occurs.
  Use this object all for subsequent operations.

##### Raises:


*  <b>`ValueError`</b>: if the shape inference fails.



- - -

#### `tf.TensorArray.write(index, value, name=None)` {#TensorArray.write}

Write `value` into index `index` of the TensorArray.

##### Args:


*  <b>`index`</b>: 0-D.  int32 scalar with the index to write to.
*  <b>`value`</b>: N-D.  Tensor of type `dtype`.  The Tensor to write to this index.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A new TensorArray object with flow that ensures the write occurs.
  Use this object all for subsequent operations.

##### Raises:


*  <b>`ValueError`</b>: if there are more writers than specified.


- - -

#### `tf.TensorArray.pack(name=None)` {#TensorArray.pack}

Return the values in the TensorArray as a packed `Tensor`.

All of the values must have been written and their shapes must all match.

##### Args:


*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  All the tensors in the TensorArray packed into one tensor.


- - -

#### `tf.TensorArray.concat(name=None)` {#TensorArray.concat}

Return the values in the TensorArray as a concatenated `Tensor`.

All of the values must have been written, their ranks must match, and
and their shapes must all match for all dimensions except the first.

##### Args:


*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  All the tensors in the TensorArray concatenated into one tensor.



- - -

#### `tf.TensorArray.grad(source, flow=None, name=None)` {#TensorArray.grad}





#### Other Methods
- - -

#### `tf.TensorArray.__init__(dtype, size=None, dynamic_size=None, clear_after_read=None, tensor_array_name=None, handle=None, flow=None, infer_shape=True, name=None)` {#TensorArray.__init__}

Construct a new TensorArray or wrap an existing TensorArray handle.

A note about the parameter `name`:

The name of the `TensorArray` (even if passed in) is uniquified: each time
a new `TensorArray` is created at runtime it is assigned its own name for
the duration of the run.  This avoids name collissions if a `TensorArray`
is created within a `while_loop`.

##### Args:


*  <b>`dtype`</b>: (required) data type of the TensorArray.
*  <b>`size`</b>: (optional) int32 scalar `Tensor`: the size of the TensorArray.
    Required if handle is not provided.
*  <b>`dynamic_size`</b>: (optional) Python bool: If true, writes to the TensorArray
    can grow the TensorArray past its initial size.  Default: False.
*  <b>`clear_after_read`</b>: Boolean (optional, default: True).  If True, clear
    TensorArray values after reading them.  This disables read-many
    semantics, but allows early release of memory.
*  <b>`tensor_array_name`</b>: (optional) Python string: the name of the TensorArray.
    This is used when creating the TensorArray handle.  If this value is
    set, handle should be None.
*  <b>`handle`</b>: (optional) A `Tensor` handle to an existing TensorArray.  If this
    is set, tensor_array_name should be None.
*  <b>`flow`</b>: (optional) A float `Tensor` scalar coming from an existing
    `TensorArray.flow`.
*  <b>`infer_shape`</b>: (optional, default: True) If True, shape inference
    is enabled.  In this case, all elements must have the same shape.
*  <b>`name`</b>: A name for the operation (optional).

##### Raises:


*  <b>`ValueError`</b>: if both handle and tensor_array_name are provided.
*  <b>`TypeError`</b>: if handle is provided but is not a Tensor.


- - -

#### `tf.TensorArray.close(name=None)` {#TensorArray.close}

Close the current TensorArray.


- - -

#### `tf.TensorArray.dtype` {#TensorArray.dtype}

The data type of this TensorArray.


- - -

#### `tf.TensorArray.size(name=None)` {#TensorArray.size}

Return the size of the TensorArray.


