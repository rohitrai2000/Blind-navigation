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

#include "tensorflow/core/distributed_runtime/tensor_coding.h"

namespace tensorflow {

TensorResponse::TensorResponse(Allocator* allocator) : allocator_(allocator) {}

Status TensorResponse::ParseFrom(Source* source) {
  if (already_used_) {
    Clear();
  }
  already_used_ = true;
  if (ParseFast(source)) return Status::OK();
  meta_.Clear();
  if (ParseSlow(source)) return Status::OK();
  return errors::InvalidArgument("Cannot parse tensor from response");
}

// Define some helper routines for decoding protocol buffer wire format data
namespace {
// We only need some of the wiretype values for this code
enum WireType {
  WIRETYPE_VARINT = 0,
  WIRETYPE_LENGTH_DELIMITED = 2,
};
inline int GetTagFieldNumber(uint32 tag) { return tag >> 3; }
inline WireType GetTagWireType(uint32 tag) {
  return static_cast<WireType>(tag & 0x7);
}

bool ReadVarintSizeAsInt(protobuf::io::CodedInputStream* input, int* result) {
  uint64 v;
  if (input->ReadVarint64(&v) && v <= static_cast<uint64>(INT_MAX)) {
    *result = static_cast<int>(v);
    return true;
  } else {
    return false;
  }
}

bool ReadNestedMessage(protobuf::io::CodedInputStream* input,
                       protobuf::Message* value) {
  int length;
  if (!ReadVarintSizeAsInt(input, &length)) return false;
  std::pair<protobuf::io::CodedInputStream::Limit, int> p =
      input->IncrementRecursionDepthAndPushLimit(length);
  if (p.second < 0 || !value->MergePartialFromCodedStream(input)) return false;
  // Make sure that parsing stopped when the limit was hit, not at an endgroup
  // tag.
  return input->DecrementRecursionDepthAndPopLimit(p.first);
}

}  // namespace

bool TensorResponse::ParseTensorSubmessage(
    protobuf::io::CodedInputStream* input, TensorProto* tensor_meta) {
  bool seen_tensor_content = false;
  while (true) {
    auto p = input->ReadTagWithCutoff(127);
    int tag = GetTagFieldNumber(p.first);
    WireType wt = GetTagWireType(p.first);
    if (!p.second) {
      bool ok = (tag == 0);
      if (ok && !seen_tensor_content) {
        // No tensor content: could be because it's a zero-length tensor
        TensorShape shape(tensor_meta->tensor_shape());
        Tensor t(allocator_, tensor_meta->dtype(), shape);
        tensor_ = std::move(t);
      }
      return ok;
    }
    switch (tag) {
      case TensorProto::kDtypeFieldNumber: {
        uint32 v;
        if ((wt != WIRETYPE_VARINT) || !input->ReadVarint32(&v)) return false;
        if (seen_tensor_content) return false;
        tensor_meta->set_dtype(static_cast<DataType>(static_cast<int>(v)));
        if (!DataTypeCanUseMemcpy(tensor_meta->dtype())) return false;
        break;
      }
      case TensorProto::kTensorShapeFieldNumber: {
        if ((wt != WIRETYPE_LENGTH_DELIMITED) ||
            !ReadNestedMessage(input, tensor_meta->mutable_tensor_shape()))
          return false;
        if (seen_tensor_content) return false;
        break;
      }
      case TensorProto::kVersionNumberFieldNumber: {
        uint32 v;
        if ((wt != WIRETYPE_VARINT) || !input->ReadVarint32(&v)) return false;
        if (seen_tensor_content) return false;
        tensor_meta->set_version_number(static_cast<int32>(v));
        break;
      }
      case TensorProto::kTensorContentFieldNumber: {
        // If we haven't seen the dtype and tensor_shape data first, we can't
        // deal with this in the fast path.
        if (seen_tensor_content) return false;
        if (wt != WIRETYPE_LENGTH_DELIMITED ||
            !tensor_meta->has_tensor_shape()) {
          return false;
        }
        int num_bytes;
        if (!ReadVarintSizeAsInt(input, &num_bytes)) return false;
        seen_tensor_content = true;
        TensorShape shape(tensor_meta->tensor_shape());
        Tensor t(allocator_, tensor_meta->dtype(), shape);
        StringPiece buf = t.tensor_data();
        if (num_bytes != buf.size()) return false;
        // TODO(jeff,sanjay): Figure out a way to avoid this copy if
        // the underlying ZeroCopyInputStream data is properly aligned
        // and compatible with what allocator_ wants.
        if (!input->ReadRaw(const_cast<char*>(buf.data()), num_bytes))
          return false;
        tensor_ = std::move(t);
        break;
      }
      default: {
        // Some other tag our fast path code is not prepared to handle.
        // return false.
        return false;
      }
    }
  }
}

bool TensorResponse::ParseFast(Source* source) {
  protobuf::io::CodedInputStream input(source->contents());
  while (true) {
    auto p = input.ReadTagWithCutoff(127);
    int tag = GetTagFieldNumber(p.first);
    WireType wt = GetTagWireType(p.first);
    if (!p.second) {
      return (tag == 0);
    }
    switch (tag) {
      case RecvTensorResponse::kTensorFieldNumber: {
        if (wt != WIRETYPE_LENGTH_DELIMITED) return false;

        int length;
        if (!ReadVarintSizeAsInt(&input, &length)) return false;
        std::pair<protobuf::io::CodedInputStream::Limit, int> p =
            input.IncrementRecursionDepthAndPushLimit(length);
        if (p.second < 0 ||
            !ParseTensorSubmessage(&input, meta_.mutable_tensor())) {
          return false;
        }
        if (!input.DecrementRecursionDepthAndPopLimit(p.first)) {
          return false;
        }
        break;
      }
      case RecvTensorResponse::kIsDeadFieldNumber: {
        uint32 v;
        if ((wt != WIRETYPE_VARINT) || !input.ReadVarint32(&v)) return false;
        meta_.set_is_dead((v != 0) ? true : false);
        break;
      }
      case RecvTensorResponse::kSendStartMicrosFieldNumber: {
        uint64 v;
        if ((wt != WIRETYPE_VARINT) || !input.ReadVarint64(&v)) return false;
        meta_.set_send_start_micros(static_cast<int64>(v));
        break;
      }
      case RecvTensorResponse::kTransportOptionsFieldNumber: {
        if ((wt != WIRETYPE_LENGTH_DELIMITED) ||
            !ReadNestedMessage(&input, meta_.mutable_transport_options()))
          return false;
        break;
      }
      default: {
        // Unknown tag, so don't handle we can't handle on the fast path
        return false;
      }
    }
  }

  return false;
}

bool TensorResponse::ParseSlow(Source* source) {
  if (!meta_.ParseFromZeroCopyStream(source->contents())) {
    return false;
  }

  Tensor parsed(meta_.tensor().dtype());
  if (!parsed.FromProto(allocator_, meta_.tensor())) {
    return false;
  }
  tensor_ = std::move(parsed);

  // Reduce memory usage for big tensors.
  {
    TensorProto empty;
    meta_.mutable_tensor()->Swap(&empty);
  }
  meta_.clear_tensor();

  return true;
}

void TensorResponse::Clear() {
  meta_.Clear();
  tensor_ = Tensor();
}

}  // namespace tensorflow
