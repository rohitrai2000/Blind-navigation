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

#include "tensorflow/core/graph/node_builder.h"

#include <vector>
#include "tensorflow/core/framework/node_def_util.h"
#include "tensorflow/core/lib/core/errors.h"

namespace tensorflow {

NodeBuilder::NodeOut::NodeOut(Node* n, int i)  // NOLINT(runtime/explicit)
    : node(n),
      error(false),
      name(node != nullptr ? node->name() : (error = true, "")),
      index(i),
      dt(SafeGetOutput(node, i, &error)) {}

NodeBuilder::NodeOut::NodeOut(StringPiece n, int i, DataType t)
    : node(nullptr), error(false), name(n.ToString()), index(i), dt(t) {}

NodeBuilder::NodeOut::NodeOut()
    : node(nullptr), error(true), index(0), dt(DT_FLOAT) {}

NodeBuilder::NodeBuilder(StringPiece name, StringPiece op_name,
                         const OpRegistryInterface* op_registry)
    : def_builder_(name, op_name, op_registry) {}

NodeBuilder::NodeBuilder(StringPiece name, const OpDef* op_def)
    : def_builder_(name, op_def) {}

NodeBuilder::NodeBuilder(const NodeDefBuilder& def_builder)
    : def_builder_(def_builder) {}

NodeBuilder& NodeBuilder::Input(Node* src_node, int src_index) {
  inputs_.emplace_back(src_node, src_index);
  DataType dt;
  if (GetOutputType(src_node, src_index, &dt)) {
    def_builder_.Input(src_node->name(), src_index, dt);
  }
  return *this;
}

NodeBuilder& NodeBuilder::Input(NodeOut src) {
  if (src.error) {
    AddIndexError(src.node, src.index);
  } else {
    inputs_.emplace_back(src.node, src.index);
    def_builder_.Input(src.name, src.index, src.dt);
  }
  return *this;
}

NodeBuilder& NodeBuilder::Input(gtl::ArraySlice<NodeOut> src_list) {
  std::vector<NodeDefBuilder::NodeOut> srcs;
  srcs.reserve(src_list.size());
  for (const auto& node_out : src_list) {
    if (node_out.error) {
      AddIndexError(node_out.node, node_out.index);
    } else {
      srcs.emplace_back(node_out.name, node_out.index, node_out.dt);
      inputs_.emplace_back(node_out.node, node_out.index);
    }
  }
  def_builder_.Input(srcs);
  return *this;
}

NodeBuilder& NodeBuilder::ControlInput(Node* src_node) {
  control_inputs_.emplace_back(src_node);
  def_builder_.ControlInput(src_node->name());
  return *this;
}

NodeBuilder& NodeBuilder::ControlInputs(gtl::ArraySlice<Node*> src_nodes) {
  control_inputs_.insert(control_inputs_.end(), src_nodes.begin(),
                         src_nodes.end());
  for (Node* src_node : src_nodes) {
    def_builder_.ControlInput(src_node->name());
  }
  return *this;
}

NodeBuilder& NodeBuilder::Device(StringPiece device_spec) {
  def_builder_.Device(device_spec);
  return *this;
}

Status NodeBuilder::Finalize(Graph* graph, Node** created_node) const {
  // In case of error, set *created_node to nullptr.
  if (created_node != nullptr) *created_node = nullptr;
  if (!errors_.empty()) {
    return errors::InvalidArgument(str_util::Join(errors_, "\n"));
  }

  NodeDef node_def;
  TF_RETURN_IF_ERROR(def_builder_.Finalize(&node_def));
  TF_RETURN_IF_ERROR(ValidateNodeDef(node_def, def_builder_.op_def()));
  TF_RETURN_IF_ERROR(
      CheckOpDeprecation(def_builder_.op_def(), graph->versions().producer()));
  Status status;
  Node* node = graph->AddNode(node_def, &status);
  if (!status.ok()) return status;

  for (size_t i = 0; i < inputs_.size(); ++i) {
    if (inputs_[i].node != nullptr) {  // Skip back edges.
      graph->AddEdge(inputs_[i].node, inputs_[i].index, node, i);
    }
  }
  for (Node* control_input : control_inputs_) {
    graph->AddControlEdge(control_input, node);
  }
  if (created_node != nullptr) *created_node = node;
  return Status::OK();
}

void NodeBuilder::AddIndexError(Node* node, int i) {
  if (node == nullptr) {
    errors_.emplace_back(
        strings::StrCat("Attempt to add nullptr Node to node with type",
                        def_builder_.op_def().name()));
  } else {
    errors_.emplace_back(
        strings::StrCat("Attempt to add output ", i, " of ", node->name(),
                        " not in range [0, ", node->num_outputs(),
                        ") to node with type ", def_builder_.op_def().name()));
  }
}

bool NodeBuilder::GetOutputType(Node* node, int i, DataType* dt) {
  bool error;
  *dt = SafeGetOutput(node, i, &error);
  if (error) AddIndexError(node, i);
  return !error;
}

}  // namespace tensorflow
