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
"""The Chi2 distribution class."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from tensorflow.contrib.distributions.python.ops import gamma
from tensorflow.python.framework import constant_op
from tensorflow.python.framework import ops


class Chi2(gamma.Gamma):
  """The Chi2 distribution with degrees of freedom df.

  The PDF of this distribution is:

  ```pdf(x) = (x^(df/2 - 1)e^(-x/2))/(2^(df/2)Gamma(df/2)), x > 0```

  Note that the Chi2 distribution is a special case of the Gamma distribution,
  with Chi2(df) = Gamma(df/2, 1/2).
  """

  def __init__(self,
               df,
               validate_args=True,
               allow_nan_stats=False,
               name="Chi2"):
    """Construct Chi2 distributions with parameter `df`.

    Args:
      df: Floating point tensor, the degrees of freedom of the
        distribution(s).  `df` must contain only positive values.
      validate_args: Whether to assert that `df > 0`, and that `x > 0` in the
        methods `prob(x)` and `log_prob(x)`. If `validate_args` is `False`
        and the inputs are invalid, correct behavior is not guaranteed.
      allow_nan_stats:  Boolean, default `False`.  If `False`, raise an
        exception if a statistic (e.g. mean/mode/etc...) is undefined for any
        batch member.  If `True`, batch members with valid parameters leading to
        undefined statistics will return NaN for this statistic.
      name: The name to prepend to all ops created by this distribution.
    """
    # Even though all stats of chi2 are defined for valid parameters, this is
    # not true in the parent class "gamma."  therefore, passing
    # allow_nan_stats=False
    # through to the parent class results in unnecessary asserts.
    with ops.op_scope([df], name):
      df = ops.convert_to_tensor(df)
      self._df = df
      super(Chi2, self).__init__(alpha=df / 2,
                                 beta=constant_op.constant(0.5, dtype=df.dtype),
                                 validate_args=validate_args,
                                 allow_nan_stats=allow_nan_stats)

  @property
  def df(self):
    return self._df
