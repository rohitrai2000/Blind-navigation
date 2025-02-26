<!-- This file is machine generated: DO NOT EDIT! -->

# Strings

Note: Functions taking `Tensor` arguments can also take anything accepted by
[`tf.convert_to_tensor`](framework.md#convert_to_tensor).

[TOC]

## Hashing

String hashing ops take a string input tensor and map each element to an
integer.

- - -

### `tf.string_to_hash_bucket_fast(input, num_buckets, name=None)` {#string_to_hash_bucket_fast}

Converts each string in the input Tensor to its hash mod by a number of buckets.

The hash function is deterministic on the content of the string within the
process and will never change. However, it is not suitable for cryptography.
This function may be used when CPU time is scarce and inputs are trusted or
unimportant. There is a risk of adversaries constructing inputs that all hash
to the same bucket. To prevent this problem, use a strong hash function with
`tf.string_to_hash_bucket_strong`.

##### Args:


*  <b>`input`</b>: A `Tensor` of type `string`. The strings to assign a hash bucket.
*  <b>`num_buckets`</b>: An `int` that is `>= 1`. The number of buckets.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `int64`.
  A Tensor of the same shape as the input `string_tensor`.


- - -

### `tf.string_to_hash_bucket_strong(input, num_buckets, key, name=None)` {#string_to_hash_bucket_strong}

Converts each string in the input Tensor to its hash mod by a number of buckets.

The hash function is deterministic on the content of the string within the
process. The hash function is a keyed hash function, where attribute `key`
defines the key of the hash function. `key` is an array of 2 elements.

A strong hash is important when inputs may be malicious, e.g. URLs with
additional components. Adversaries could try to make their inputs hash to the
same bucket for a denial-of-service attack or to skew the results. A strong
hash prevents this by making it dificult, if not infeasible, to compute inputs
that hash to the same bucket. This comes at a cost of roughly 4x higher compute
time than tf.string_to_hash_bucket_fast.

##### Args:


*  <b>`input`</b>: A `Tensor` of type `string`. The strings to assign a hash bucket.
*  <b>`num_buckets`</b>: An `int` that is `>= 1`. The number of buckets.
*  <b>`key`</b>: A list of `ints`.
    The key for the keyed hash function passed as a list of two uint64
    elements.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `int64`.
  A Tensor of the same shape as the input `string_tensor`.


- - -

### `tf.string_to_hash_bucket(string_tensor, num_buckets, name=None)` {#string_to_hash_bucket}

Converts each string in the input Tensor to its hash mod by a number of buckets.

The hash function is deterministic on the content of the string within the
process.

Note that the hash function may change from time to time.
This functionality will be deprecated and it's recommended to use
`tf.string_to_hash_bucket_fast()` or `tf.string_to_hash_bucket_strong()`.

##### Args:


*  <b>`string_tensor`</b>: A `Tensor` of type `string`.
*  <b>`num_buckets`</b>: An `int` that is `>= 1`. The number of buckets.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `int64`.
  A Tensor of the same shape as the input `string_tensor`.



## Joining

String joining ops concatenate elements of input string tensors to produce a new
string tensor.

- - -

### `tf.reduce_join(inputs, reduction_indices, keep_dims=None, separator=None, name=None)` {#reduce_join}

Joins a string Tensor across the given dimensions.

Computes the string join across dimensions in the given string Tensor of shape
`[d_0, d_1, ..., d_n-1]`.  Returns a new Tensor created by joining the input
strings with the given separator (default: empty string).  Negative indices are
counted backwards from the end, with `-1` being equivalent to `n - 1`.  Passing
an empty `reduction_indices` joins all strings in linear index order and outputs
a scalar string.


For example:
```
# tensor `a` is [["a", "b"], ["c", "d"]]
tf.reduce_join(a, 0) ==> ["ac", "bd"]
tf.reduce_join(a, 1) ==> ["ab", "cd"]
tf.reduce_join(a, -2) = tf.reduce_join(a, 0) ==> ["ac", "bd"]
tf.reduce_join(a, -1) = tf.reduce_join(a, 1) ==> ["ab", "cd"]
tf.reduce_join(a, 0, keep_dims=True) ==> [["ac", "bd"]]
tf.reduce_join(a, 1, keep_dims=True) ==> [["ab"], ["cd"]]
tf.reduce_join(a, 0, separator=".") ==> ["a.c", "b.d"]
tf.reduce_join(a, [0, 1]) ==> ["acbd"]
tf.reduce_join(a, [1, 0]) ==> ["abcd"]
tf.reduce_join(a, []) ==> ["abcd"]
```

##### Args:


*  <b>`inputs`</b>: A `Tensor` of type `string`.
    The input to be joined.  All reduced indices must have non-zero size.
*  <b>`reduction_indices`</b>: A `Tensor` of type `int32`.
    The dimensions to reduce over.  Dimensions are reduced in the
    order specified.  If `reduction_indices` has higher rank than `1`, it is
    flattened.  Omitting `reduction_indices` is equivalent to passing
    `[n-1, n-2, ..., 0]`.  Negative indices from `-n` to `-1` are supported.
*  <b>`keep_dims`</b>: An optional `bool`. Defaults to `False`.
    If `True`, retain reduced dimensions with length `1`.
*  <b>`separator`</b>: An optional `string`. Defaults to `""`.
    The separator to use when joining.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `string`.
  Has shape equal to that of the input with reduced dimensions removed or
  set to `1` depending on `keep_dims`.


- - -

### `tf.string_join(inputs, separator=None, name=None)` {#string_join}

Joins the strings in the given list of string tensors into one tensor;

with the given separator (default is an empty separator).

##### Args:


*  <b>`inputs`</b>: A list of at least 1 `Tensor` objects of type `string`.
    A list of string tensors.  The tensors must all have the same shape,
    or be scalars.  Scalars may be mixed in; these will be broadcast to the shape
    of non-scalar inputs.
*  <b>`separator`</b>: An optional `string`. Defaults to `""`.
    string, an optional join separator.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `string`.



## Conversion

- - -

### `tf.as_string(input, precision=None, scientific=None, shortest=None, width=None, fill=None, name=None)` {#as_string}

Converts each entry in the given tensor to strings.  Supports many numeric

types and boolean.

##### Args:


*  <b>`input`</b>: A `Tensor`. Must be one of the following types: `int32`, `int64`, `complex64`, `float32`, `float64`, `bool`, `int8`.
*  <b>`precision`</b>: An optional `int`. Defaults to `-1`.
    The post-decimal precision to use for floating point numbers.
    Only used if precision > -1.
*  <b>`scientific`</b>: An optional `bool`. Defaults to `False`.
    Use scientific notation for floating point numbers.
*  <b>`shortest`</b>: An optional `bool`. Defaults to `False`.
    Use shortest representation (either scientific or standard) for
    floating point numbers.
*  <b>`width`</b>: An optional `int`. Defaults to `-1`.
    Pad pre-decimal numbers to this width.
    Applies to both floating point and integer numbers.
    Only used if width > -1.
*  <b>`fill`</b>: An optional `string`. Defaults to `""`.
    The value to pad if width > -1.  If empty, pads with spaces.
    Another typical value is '0'.  String cannot be longer than 1 character.
*  <b>`name`</b>: A name for the operation (optional).

##### Returns:

  A `Tensor` of type `string`.


