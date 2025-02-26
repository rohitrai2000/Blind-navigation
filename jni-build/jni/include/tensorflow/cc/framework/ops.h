/* Copyright 2016 The TensorFlow Authors. All Rights Reserved.

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

#ifndef THIRD_PARTY_TENSORFLOW_CC_FRAMEWORK_OPS_H_
#define THIRD_PARTY_TENSORFLOW_CC_FRAMEWORK_OPS_H_

#include <type_traits>

#include "tensorflow/core/framework/tensor.h"
// TBD(keveman): This is going to be moved to //third_party/tensorflow
// eventually. Remove the NOLINT comment when moving.
#include "tensorflow/core/framework/tensor.pb.h"  // NOLINT
#include "tensorflow/core/graph/graph.h"

namespace tensorflow {
namespace ops {

class Output;

// Represents a node in the computation graph.
class Operation {
 public:
  Operation() : node_(nullptr) {}
  explicit Operation(Node* n);

  int num_inputs() const { return node_->num_inputs(); }
  DataType input_type(int o) const { return node_->input_type(o); }
  Output input(int i) const;

  int num_outputs() const { return node_->num_outputs(); }
  DataType output_type(int o) const { return node_->output_type(o); }
  Output output(int i) const;

  Node* node() const { return node_; }

 private:
  typedef std::vector<std::pair<Node*, int64>> Inputs;
  static Inputs GetInputs(Node* node);

  Inputs inputs_;
  Node* node_;
};

// Represents a tensor value produced by an Operation.
class Output {
 public:
  Output() = default;
  explicit Output(Node* n) : op_(n) {}
  Output(Node* n, int64 index) : op_(n), index_(index) {}
  Output(const Operation& op, int64 index) : op_(op), index_(index) {}

  Operation op() const { return op_; }
  Node* node() const { return op().node(); }
  int64 index() const { return index_; }
  DataType type() const { return op_.output_type(index_); }

 private:
  Operation op_ = Operation(nullptr);
  int64 index_ = 0;
};

// Represents a tensor value that can be used as an operand to an Operation.
class Input {
 public:
  // Initializer enables constructing an Input object from various kinds of C++
  // constants such as simple primitive constants and nested initializer lists
  // representing a multi-dimensional array. Initializer constructors are all
  // templates, so the aforementioned kinds of C++ constants can be used to
  // construct an Initializer. Intializer stores the value it got constructed
  // with in a Tensor object.
  struct Initializer {
    // Construct from a scalar value of an arithmetic type or a type that can be
    // converted to a string (eg. a string literal).
    template <typename T, typename = typename std::enable_if<
                              std::is_arithmetic<T>::value ||
                              std::is_convertible<T, string>::value>::type>
    Initializer(const T& v) {  // NOLINT(runtime/explicit)
      typedef typename RealType<T>::type RealT;
      Tensor t(DataTypeToEnum<RealT>::v(), TensorShape());
      t.flat<T>()(0) = RealT(v);
      tensor = t;
    }

    Initializer(const Tensor& t) : tensor(t) {}  // NOLINT(runtime/explicit)

    // Construct from a scalar value and an explicit shape
    template <typename T, typename = typename std::enable_if<
                              std::is_arithmetic<T>::value ||
                              std::is_convertible<T, string>::value>::type>
    Initializer(const T& v, const TensorShape& shape) {
      typedef typename RealType<T>::type RealT;
      Tensor t(DataTypeToEnum<RealT>::v(), shape);
      for (int64 i = 0; i < t.NumElements(); ++i) {
        t.flat<T>()(i) = RealT(v);
      }
      tensor = t;
    }

    // Construct from a initializer list of scalars (a one-dimensional tensor).
    template <typename T, typename = typename std::enable_if<
                              std::is_arithmetic<T>::value ||
                              std::is_convertible<T, string>::value>::type>
    Initializer(
        const std::initializer_list<T>& v) {  // NOLINT(runtime/explicit)
      typedef typename RealType<T>::type RealT;
      Tensor t(DataTypeToEnum<RealT>::v(),
               TensorShape{static_cast<int>(v.size())});
      std::copy_n(v.begin(), v.size(), t.flat<RealT>().data());
      tensor = t;
    }

    // Construct from a initializer list of scalars and an explicit shape.
    template <typename T, typename = typename std::enable_if<
                              std::is_arithmetic<T>::value ||
                              std::is_convertible<T, string>::value>::type>
    Initializer(const std::initializer_list<T>& v, const TensorShape& shape) {
      typedef typename RealType<T>::type RealT;
      Tensor t(DataTypeToEnum<RealT>::v(), shape);
      if (t.NumElements() != v.size()) {
        status = errors::InvalidArgument(
            "Cannot construct a tensor with ", t.NumElements(),
            " from an initializer list with ", v.size(), " elements");
        return;
      }
      std::copy_n(v.begin(), v.size(), t.flat<RealT>().data());
      tensor = t;
    }

    // Construct a multi-dimensional tensor from a nested initializer list. Note
    // that C++ syntax allows nesting of arbitrarily typed intializer lists, so
    // such invalid initializers cannot be disallowed at compile time. This
    // function performs checks to make sure that the nested initializer list is
    // indeed a valid multi-dimensional tensor.
    Initializer(const std::initializer_list<Initializer>& v);

    template <typename T, bool = std::is_convertible<T, string>::value>
    struct RealType {
      typedef string type;
    };

    template <typename T>
    struct RealType<T, false> {
      typedef T type;
    };

    TensorProto AsTensorProto() {
      TensorProto tensor_proto;
      if (tensor.NumElements() > 1) {
        tensor.AsProtoTensorContent(&tensor_proto);
      } else {
        tensor.AsProtoField(&tensor_proto);
      }
      return tensor_proto;
    }

    Status status;
    Tensor tensor;
  };

  // All of Input's constructors are implicit. Input can be implicitly
  // constructed from the following objects :
  // * Output: This is so that the output of an Operation can be directly used
  //   as the input to a op wrapper, which takes Inputs.
  // * A scalar, or a multi-dimensional tensor specified as a recursive
  //   initializer list. This enables directly passing constants as
  //   inputs to op wrappers.
  Input(const Output& o) : output_(o) {}  // NOLINT(runtime/explicit)

  template <typename T, typename = typename std::enable_if<
                            std::is_arithmetic<T>::value ||
                            std::is_convertible<T, string>::value>::type>
  Input(const T& v)  // NOLINT(runtime/explicit)
      : Input(Initializer(v)) {}

  Input(const Initializer& init)  // NOLINT(runtime/explicit)
      : status_(init.status),
        tensor_(init.tensor) {}

  Input(const Tensor& t)  // NOLINT(runtime/explicit)
      : status_(Status::OK()),
        tensor_(t) {}

  Input(const std::initializer_list<Initializer>&
            init) {  // NOLINT(runtime/explicit)
    for (const auto& i : init) {
      if (!i.status.ok()) {
        status_ = i.status;
        return;
      }
    }
    tensor_ = Initializer(init).tensor;
  }

  // Constructor specifying a node name, index and datatype. This should only be
  // used for specifying a backward edge, needed by control flow.
  Input(const string& name, int i, DataType dt)
      : node_name_(name), index_(i), data_type_(dt) {}

  Node* node() const { return output_.node(); }
  string node_name() const { return node_name_; }
  int index() const { return node_name_.empty() ? output_.index() : index_; }
  DataType data_type() const { return data_type_; }
  Status status() const { return status_; }
  const Tensor& tensor() const { return tensor_; }

 private:
  Status status_;
  Output output_ = Output(Operation(nullptr), 0);
  Tensor tensor_;
  const string node_name_ = "";
  int index_ = 0;
  DataType data_type_ = DT_INVALID;
};

// A type for representing the output of ops that produce more than one output,
// or a list of tensors.
typedef std::vector<Output> OutputList;

// A type for representing the input to ops that require a list of tensors.
class InputList {
 public:
  // Implicitly convert a list of outputs to a list of inputs. This is useful to
  // write code such as tf.Concat(tf.Split(x, 4)).
  InputList(const OutputList& out) {  // NOLINT(runtime/explicit)
    for (auto const& x : out) {
      inputs_.push_back(x);
    }
  }

  InputList(
      const std::initializer_list<Input>& inputs)  // NOLINT(runtime/explicit)
      : inputs_(inputs.begin(), inputs.end()) {}

  InputList(const tensorflow::gtl::ArraySlice<Input>&
                inputs)  // NOLINT(runtime/explicit)
      : inputs_(inputs.begin(), inputs.end()) {}

  InputList(
      const std::initializer_list<Output>& out) {  // NOLINT(runtime/explicit)
    for (auto const& x : out) {
      inputs_.push_back(x);
    }
  }

  typename std::vector<Input>::iterator begin() { return inputs_.begin(); }
  typename std::vector<Input>::iterator end() { return inputs_.end(); }
  typename std::vector<Input>::const_iterator begin() const {
    return inputs_.begin();
  }
  typename std::vector<Input>::const_iterator end() const {
    return inputs_.end();
  }

 private:
  std::vector<Input> inputs_;
};

}  // namespace ops
}  // namespace tensorflow

#endif  // THIRD_PARTY_TENSORFLOW_CC_FRAMEWORK_OPS_H_
