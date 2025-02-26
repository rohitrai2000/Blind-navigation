# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

"""Contains the definition of a Dataset.

A Dataset is a collection of several components: (1) a list of data sources
(2) a Reader class that can read those sources and returns possibly encoded
samples of data (3) a decoder that decodes each sample of data provided by the
reader (4) the total number of samples and (5) an optional dictionary mapping
the list of items returns to a description of those items.

Data can be loaded from a dataset specification using a dataset_data_provider:

  dataset = CreateMyDataset(...)
  provider = dataset_data_provider.DatasetDataProvider(
      dataset, shuffle=False)
  image, label = provider.get(['image', 'label'])

See slim.data.dataset_data_provider for additional examples.
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import collections


Dataset = collections.namedtuple('Dataset',
                                 ['data_sources',
                                  'reader',
                                  'decoder',
                                  'num_samples',
                                  'items_to_descriptions'])
