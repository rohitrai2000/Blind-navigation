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

#ifndef TENSORFLOW_STREAM_EXECUTOR_LIB_ENV_H_
#define TENSORFLOW_STREAM_EXECUTOR_LIB_ENV_H_

#include "tensorflow/core/platform/env.h"
#include "tensorflow/stream_executor/lib/stringpiece.h"
#include "tensorflow/stream_executor/platform/port.h"

namespace perftools {
namespace gputools {
namespace port {

using tensorflow::Env;
using tensorflow::ReadFileToString;
using tensorflow::Thread;
using tensorflow::WriteStringToFile;

inline bool FileExists(const string& filename) {
  return Env::Default()->FileExists(filename);
}

inline bool FileExists(const port::StringPiece& filename) {
  return Env::Default()->FileExists(filename.ToString());
}

}  // namespace port
}  // namespace gputools
}  // namespace perftools

#endif  // TENSORFLOW_STREAM_EXECUTOR_LIB_ENV_H_
