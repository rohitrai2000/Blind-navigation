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

#include <string>

#include "tensorflow/core/common_runtime/session_factory.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/public/session.h"

namespace tensorflow {

Session::Session() {}

Session::~Session() {}

Status Session::Run(const RunOptions& run_options,
                    const std::vector<std::pair<string, Tensor> >& inputs,
                    const std::vector<string>& output_tensor_names,
                    const std::vector<string>& target_node_names,
                    std::vector<Tensor>* outputs, RunMetadata* run_metadata) {
  return errors::Unimplemented(
      "Run with options is not supported for this session.");
}

Status Session::PRunSetup(const std::vector<string>& input_names,
                          const std::vector<string>& output_names,
                          const std::vector<string>& target_nodes,
                          string* handle) {
  return errors::Unimplemented(
      "Partial run is not supported for this session.");
}

Status Session::PRun(const string& handle,
                     const std::vector<std::pair<string, Tensor> >& inputs,
                     const std::vector<string>& output_names,
                     std::vector<Tensor>* outputs) {
  return errors::Unimplemented(
      "Partial run is not supported for this session.");
}

Session* NewSession(const SessionOptions& options) {
  SessionFactory* factory;
  Status s = SessionFactory::GetFactory(options, &factory);
  if (!s.ok()) {
    LOG(ERROR) << s;
    return nullptr;
  }
  return factory->NewSession(options);
}

Status NewSession(const SessionOptions& options, Session** out_session) {
  SessionFactory* factory;
  Status s = SessionFactory::GetFactory(options, &factory);
  if (!s.ok()) {
    *out_session = nullptr;
    LOG(ERROR) << s;
    return s;
  }
  *out_session = factory->NewSession(options);
  if (!*out_session) {
    return errors::Internal("Failed to create session.");
  }
  return Status::OK();
}

Status Reset(const SessionOptions& options,
             const std::vector<string>& containers) {
  SessionFactory* factory;
  TF_RETURN_IF_ERROR(SessionFactory::GetFactory(options, &factory));
  return factory->Reset(options, containers);
}

}  // namespace tensorflow
