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

#include "tensorflow/stream_executor/lib/process_state.h"

#include <unistd.h>

#include <memory>

namespace perftools {
namespace gputools {
namespace port {

string Hostname() {
  char hostname[1024];
  gethostname(hostname, sizeof hostname);
  hostname[sizeof hostname - 1] = 0;
  return hostname;
}

bool GetCurrentDirectory(string* dir) {
  size_t len = 128;
  std::unique_ptr<char[]> a(new char[len]);
  for (;;) {
    char* p = getcwd(a.get(), len);
    if (p != NULL) {
      *dir = p;
      return true;
    } else if (errno == ERANGE) {
      len += len;
      a.reset(new char[len]);
    } else {
      return false;
    }
  }
}

}  // namespace port
}  // namespace gputools
}  // namespace perftools
