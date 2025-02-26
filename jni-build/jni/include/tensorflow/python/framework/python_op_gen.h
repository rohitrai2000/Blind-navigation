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

#ifndef TENSORFLOW_PYTHON_FRAMEWORK_PYTHON_OP_GEN_H_
#define TENSORFLOW_PYTHON_FRAMEWORK_PYTHON_OP_GEN_H_

#include <string>
#include "tensorflow/core/framework/op_def.pb.h"
#include "tensorflow/core/platform/types.h"

namespace tensorflow {

// hidden_ops should be a comma-separated
// list of Op names that should get a leading _ in the output.
// The Print* version prints the output to stdout, Get* version returns the
// output as a string.
void PrintPythonOps(const OpList& ops, const string& hidden_ops,
                    bool require_shapes);
string GetPythonOps(const OpList& ops, const string& hidden_ops,
                    bool require_shapes);

// Get the python wrappers for a list of ops in a OpList.
// buf should be a pointer to a buffer containing the binary encoded OpList
// proto, and len should be the length of that buffer.
string GetPythonWrappers(const char* op_wrapper_buf, size_t op_wrapper_len);

}  // namespace tensorflow

#endif  // TENSORFLOW_PYTHON_FRAMEWORK_PYTHON_OP_GEN_H_
