/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_CORE_KERNELS_LOOKUP_UTIL_H_
#define TENSORFLOW_CORE_KERNELS_LOOKUP_UTIL_H_

#include "tensorflow/core/framework/lookup_interface.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/kernels/initializable_lookup_table.h"

namespace tensorflow {
namespace lookup {

// Gets the LookupTable stored in the ctx->resource_manager() with key
// passed by attribute with name input_name, returns null if the table
// doesn't exist.
Status GetLookupTable(const string& input_name, OpKernelContext* ctx,
                      LookupInterface** table);

// Gets the InitializableLookupTable stored in the
// ctx->resource_manager() with key passed by attribute with name
// input_name, returns null if the table doesn't exist.
Status GetInitializableLookupTable(const string& input_name,
                                   OpKernelContext* ctx,
                                   InitializableLookupTable** table);

// Verify that the given key_dtype and value_dtype matches the corresponding
// table's data types.
Status CheckTableDataTypes(const LookupInterface& table, DataType key_dtype,
                           DataType value_dtype, const string& table_name);
}  // namespace lookup
}  // namespace tensorflow

#endif  // TENSORFLOW_CORE_KERNELS_LOOKUP_UTIL_H_
