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

#include "tensorflow/core/framework/graph_def_util.h"

#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "tensorflow/core/framework/node_def_util.h"
#include "tensorflow/core/framework/op_def_util.h"
#include "tensorflow/core/framework/versions.pb_text.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/strings/strcat.h"

namespace tensorflow {

string SummarizeGraphDef(const GraphDef& graph_def) {
  string ret;
  strings::StrAppend(&ret, "versions = ",
                     ProtoShortDebugString(graph_def.versions()), ";\n");
  for (const NodeDef& node : graph_def.node()) {
    strings::StrAppend(&ret, SummarizeNodeDef(node), ";\n");
  }
  return ret;
}

Status ValidateExternalGraphDefSyntax(const GraphDef& graph_def) {
  for (const NodeDef& node : graph_def.node()) {
    TF_RETURN_IF_ERROR(ValidateExternalNodeDefSyntax(node));
  }
  return Status::OK();
}

Status AddDefaultAttrsToGraphDef(GraphDef* graph_def,
                                 const OpRegistryInterface& op_registry,
                                 int node_offset) {
  if (node_offset > graph_def->node_size()) {
    return errors::InvalidArgument(
        "Tried to add default attrs to GraphDef "
        "starting at offset ",
        node_offset, " with total nodes in graph: ", graph_def->node_size());
  }

  for (int i = node_offset; i < graph_def->node_size(); ++i) {
    NodeDef* node_def = graph_def->mutable_node(i);
    const OpDef* op_def;
    TF_RETURN_IF_ERROR(op_registry.LookUpOpDef(node_def->op(), &op_def));
    AddDefaultsToNodeDef(*op_def, node_def);
  }

  return Status::OK();
}

Status RemoveNewDefaultAttrsFromGraphDef(
    GraphDef* graph_def, const OpRegistryInterface& consumer_op_registry,
    const OpRegistryInterface& producer_op_registry,
    std::set<std::pair<string, string>>* op_attr_removed) {
  Status s;
  std::vector<string> to_remove;
  for (int n = 0; n < graph_def->node_size(); ++n) {
    NodeDef* node_def = graph_def->mutable_node(n);
    const OpDef* producer_op_def;
    const OpDef* consumer_op_def;

    TF_RETURN_IF_ERROR(
        producer_op_registry.LookUpOpDef(node_def->op(), &producer_op_def));
    TF_RETURN_IF_ERROR(
        consumer_op_registry.LookUpOpDef(node_def->op(), &consumer_op_def));

    for (const auto& attr : node_def->attr()) {
      // If the attr is not in consumer_op_def and doesn't start with '_'...
      if (!StringPiece(attr.first).starts_with("_") &&
          FindAttr(attr.first, *consumer_op_def) == nullptr) {
        const OpDef::AttrDef* producer_attr_def =
            FindAttr(attr.first, *producer_op_def);
        if (producer_attr_def == nullptr) {
          return errors::InvalidArgument(
              "Attr '", attr.first, "' missing in producer's OpDef: ",
              SummarizeOpDef(*producer_op_def), " but found in node: ",
              SummarizeNodeDef(*node_def));
        }
        // ...and it has the same value as the default in producer,
        if (producer_attr_def->has_default_value() &&
            AreAttrValuesEqual(producer_attr_def->default_value(),
                               attr.second)) {
          // then we will remove it below.
          to_remove.emplace_back(attr.first);
        }
      }
    }
    // We separate identifying which attrs should be removed from
    // actually removing them to avoid invalidating the loop iterators
    // above.
    for (const string& attr_name : to_remove) {
      node_def->mutable_attr()->erase(attr_name);
      if (op_attr_removed != nullptr) {
        op_attr_removed->insert(std::make_pair(node_def->op(), attr_name));
      }
    }
    to_remove.clear();
  }

  return s;
}

void OpsUsedByGraph(const GraphDef& graph_def,
                    std::set<string>* ops_used_in_graph) {
  // Map function names to definitions.
  std::unordered_map<string, const FunctionDef*> name_to_function;
  for (const auto& function : graph_def.library().function()) {
    name_to_function.insert(
        std::make_pair(function.signature().name(), &function));
  }

  // Collect the sorted list of op names.  Since functions can reference
  // functions, we need a recursive traversal.
  std::set<string> used_ops;  // Includes both primitive ops and functions
  std::vector<const FunctionDef*> functions_to_process;  // A subset of used_ops
  // Collect the logic to mark an op in a lambda; it'll be used twice below.
  const auto mark_op_as_used = [&used_ops, &functions_to_process,
                                &name_to_function](const string& op) {
    if (used_ops.insert(op).second) {
      // If it's a function, we'll need to process further
      const auto it = name_to_function.find(op);
      if (it != name_to_function.end()) {
        functions_to_process.push_back(it->second);
      }
    }
  };
  for (const auto& node : graph_def.node()) {
    mark_op_as_used(node.op());
  }
  while (!functions_to_process.empty()) {
    const FunctionDef* fun = functions_to_process.back();
    functions_to_process.pop_back();
    for (const auto& node : fun->node()) {
      mark_op_as_used(node.op());
    }
  }

  // Filter out function names to produce output.
  // TODO(josh11b): Change the above code to produce this directly.
  ops_used_in_graph->clear();
  for (const string& op_name : used_ops) {
    if (name_to_function.find(op_name) == name_to_function.end()) {
      ops_used_in_graph->insert(op_name);
    }
  }
}

Status StrippedOpListForGraph(const GraphDef& graph_def,
                              const OpRegistryInterface& op_registry,
                              OpList* stripped_op_list) {
  std::set<string> used_ops;
  OpsUsedByGraph(graph_def, &used_ops);

  // Build the stripped op list in sorted order, ignoring functions.
  stripped_op_list->clear_op();
  for (const string& op_name : used_ops) {
    const OpDef* op_def;
    TF_RETURN_IF_ERROR(op_registry.LookUpOpDef(op_name, &op_def));
    OpDef* stripped_op = stripped_op_list->add_op();
    stripped_op->CopyFrom(*op_def);
    RemoveDescriptionsFromOpDef(stripped_op);
  }
  return Status::OK();
}

}  // namespace tensorflow
