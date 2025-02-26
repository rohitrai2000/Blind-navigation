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

#ifndef TENSORFLOW_STREAM_EXECUTOR_LIB_INITIALIZE_H_
#define TENSORFLOW_STREAM_EXECUTOR_LIB_INITIALIZE_H_

#include "tensorflow/stream_executor/platform/port.h"

#if defined(PLATFORM_GOOGLE)
#else

#undef REGISTER_MODULE_INITIALIZER

namespace perftools {
namespace gputools {
namespace port {

class Initializer {
 public:
  typedef void (*InitializerFunc)();
  explicit Initializer(InitializerFunc func) { func(); }
};

}  // namespace port
}  // namespace gputools
}  // namespace perftools

#define REGISTER_INITIALIZER(type, name, body)                               \
  static void google_init_##type##_##name() { body; }                        \
  perftools::gputools::port::Initializer google_initializer_##type##_##name( \
      google_init_##type##_##name)

#define REGISTER_MODULE_INITIALIZER(name, body) \
  REGISTER_INITIALIZER(module, name, body)

#endif  // !defined(PLATFORM_GOOGLE)

#endif  // TENSORFLOW_STREAM_EXECUTOR_LIB_INITIALIZE_H_
