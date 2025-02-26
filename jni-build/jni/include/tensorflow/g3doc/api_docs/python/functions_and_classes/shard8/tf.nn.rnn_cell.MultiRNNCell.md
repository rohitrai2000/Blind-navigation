RNN cell composed sequentially of multiple simple cells.
- - -

#### `tf.nn.rnn_cell.MultiRNNCell.__init__(cells, state_is_tuple=False)` {#MultiRNNCell.__init__}

Create a RNN cell composed sequentially of a number of RNNCells.

##### Args:


*  <b>`cells`</b>: list of RNNCells that will be composed in this order.
*  <b>`state_is_tuple`</b>: If True, accepted and returned states are n-tuples, where
    `n = len(cells)`.  By default (False), the states are all
    concatenated along the column axis.

##### Raises:


*  <b>`ValueError`</b>: if cells is empty (not allowed), or at least one of the cells
    returns a state tuple but the flag `state_is_tuple` is `False`.


- - -

#### `tf.nn.rnn_cell.MultiRNNCell.output_size` {#MultiRNNCell.output_size}




- - -

#### `tf.nn.rnn_cell.MultiRNNCell.state_size` {#MultiRNNCell.state_size}




- - -

#### `tf.nn.rnn_cell.MultiRNNCell.zero_state(batch_size, dtype)` {#MultiRNNCell.zero_state}

Return zero-filled state tensor(s).

##### Args:


*  <b>`batch_size`</b>: int, float, or unit Tensor representing the batch size.
*  <b>`dtype`</b>: the data type to use for the state.

##### Returns:

  If `state_size` is an int or TensorShape, then the return value is a
  `N-D` tensor of shape `[batch_size x state_size]` filled with zeros.

  If `state_size` is a nested list or tuple, then the return value is
  a nested list or tuple (of the same structure) of `2-D` tensors with
the shapes `[batch_size x s]` for each s in `state_size`.


