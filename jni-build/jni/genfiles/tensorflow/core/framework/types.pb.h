// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tensorflow/core/framework/types.proto

#ifndef PROTOBUF_tensorflow_2fcore_2fframework_2ftypes_2eproto__INCLUDED
#define PROTOBUF_tensorflow_2fcore_2fframework_2ftypes_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 3000000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 3000000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/generated_enum_reflection.h>
// @@protoc_insertion_point(includes)

namespace tensorflow {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_tensorflow_2fcore_2fframework_2ftypes_2eproto();
void protobuf_AssignDesc_tensorflow_2fcore_2fframework_2ftypes_2eproto();
void protobuf_ShutdownFile_tensorflow_2fcore_2fframework_2ftypes_2eproto();


enum DataType {
  DT_INVALID = 0,
  DT_FLOAT = 1,
  DT_DOUBLE = 2,
  DT_INT32 = 3,
  DT_UINT8 = 4,
  DT_INT16 = 5,
  DT_INT8 = 6,
  DT_STRING = 7,
  DT_COMPLEX64 = 8,
  DT_INT64 = 9,
  DT_BOOL = 10,
  DT_QINT8 = 11,
  DT_QUINT8 = 12,
  DT_QINT32 = 13,
  DT_BFLOAT16 = 14,
  DT_QINT16 = 15,
  DT_QUINT16 = 16,
  DT_UINT16 = 17,
  DT_COMPLEX128 = 18,
  DT_HALF = 19,
  DT_FLOAT_REF = 101,
  DT_DOUBLE_REF = 102,
  DT_INT32_REF = 103,
  DT_UINT8_REF = 104,
  DT_INT16_REF = 105,
  DT_INT8_REF = 106,
  DT_STRING_REF = 107,
  DT_COMPLEX64_REF = 108,
  DT_INT64_REF = 109,
  DT_BOOL_REF = 110,
  DT_QINT8_REF = 111,
  DT_QUINT8_REF = 112,
  DT_QINT32_REF = 113,
  DT_BFLOAT16_REF = 114,
  DT_QINT16_REF = 115,
  DT_QUINT16_REF = 116,
  DT_UINT16_REF = 117,
  DT_COMPLEX128_REF = 118,
  DT_HALF_REF = 119,
  DataType_INT_MIN_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32min,
  DataType_INT_MAX_SENTINEL_DO_NOT_USE_ = ::google::protobuf::kint32max
};
bool DataType_IsValid(int value);
const DataType DataType_MIN = DT_INVALID;
const DataType DataType_MAX = DT_HALF_REF;
const int DataType_ARRAYSIZE = DataType_MAX + 1;

const ::google::protobuf::EnumDescriptor* DataType_descriptor();
inline const ::std::string& DataType_Name(DataType value) {
  return ::google::protobuf::internal::NameOfEnum(
    DataType_descriptor(), value);
}
inline bool DataType_Parse(
    const ::std::string& name, DataType* value) {
  return ::google::protobuf::internal::ParseNamedEnum<DataType>(
    DataType_descriptor(), name, value);
}
// ===================================================================


// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace tensorflow

#ifndef SWIG
namespace google {
namespace protobuf {

template <> struct is_proto_enum< ::tensorflow::DataType> : ::google::protobuf::internal::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::tensorflow::DataType>() {
  return ::tensorflow::DataType_descriptor();
}

}  // namespace protobuf
}  // namespace google
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_tensorflow_2fcore_2fframework_2ftypes_2eproto__INCLUDED
