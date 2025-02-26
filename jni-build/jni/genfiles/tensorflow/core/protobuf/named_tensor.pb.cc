// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tensorflow/core/protobuf/named_tensor.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "tensorflow/core/protobuf/named_tensor.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace tensorflow {

namespace {

const ::google::protobuf::Descriptor* NamedTensorProto_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  NamedTensorProto_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto() {
  protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "tensorflow/core/protobuf/named_tensor.proto");
  GOOGLE_CHECK(file != NULL);
  NamedTensorProto_descriptor_ = file->message_type(0);
  static const int NamedTensorProto_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(NamedTensorProto, name_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(NamedTensorProto, tensor_),
  };
  NamedTensorProto_reflection_ =
    ::google::protobuf::internal::GeneratedMessageReflection::NewGeneratedMessageReflection(
      NamedTensorProto_descriptor_,
      NamedTensorProto::default_instance_,
      NamedTensorProto_offsets_,
      -1,
      -1,
      -1,
      sizeof(NamedTensorProto),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(NamedTensorProto, _internal_metadata_),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(NamedTensorProto, _is_default_instance_));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
      NamedTensorProto_descriptor_, &NamedTensorProto::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto() {
  delete NamedTensorProto::default_instance_;
  delete NamedTensorProto_reflection_;
}

void protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::tensorflow::protobuf_AddDesc_tensorflow_2fcore_2fframework_2ftensor_2eproto();
  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n+tensorflow/core/protobuf/named_tensor."
    "proto\022\ntensorflow\032&tensorflow/core/frame"
    "work/tensor.proto\"I\n\020NamedTensorProto\022\014\n"
    "\004name\030\001 \001(\t\022\'\n\006tensor\030\002 \001(\0132\027.tensorflow"
    ".TensorProtoB2\n\030org.tensorflow.framework"
    "B\021NamedTensorProtosP\001\370\001\001b\006proto3", 232);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "tensorflow/core/protobuf/named_tensor.proto", &protobuf_RegisterTypes);
  NamedTensorProto::default_instance_ = new NamedTensorProto();
  NamedTensorProto::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto {
  StaticDescriptorInitializer_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto() {
    protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto();
  }
} static_descriptor_initializer_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto_;

namespace {

static void MergeFromFail(int line) GOOGLE_ATTRIBUTE_COLD;
static void MergeFromFail(int line) {
  GOOGLE_CHECK(false) << __FILE__ << ":" << line;
}

}  // namespace


// ===================================================================

void NamedTensorProto::_slow_mutable_tensor() {
  tensor_ = ::google::protobuf::Arena::CreateMessage< ::tensorflow::TensorProto >(
      GetArenaNoVirtual());
}
::tensorflow::TensorProto* NamedTensorProto::_slow_release_tensor() {
  if (tensor_ == NULL) {
    return NULL;
  } else {
    ::tensorflow::TensorProto* temp = new ::tensorflow::TensorProto;
    temp->MergeFrom(*tensor_);
    tensor_ = NULL;
    return temp;
  }
}
::tensorflow::TensorProto* NamedTensorProto::unsafe_arena_release_tensor() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.NamedTensorProto.tensor)
  
  ::tensorflow::TensorProto* temp = tensor_;
  tensor_ = NULL;
  return temp;
}
void NamedTensorProto::_slow_set_allocated_tensor(
    ::google::protobuf::Arena* message_arena, ::tensorflow::TensorProto** tensor) {
    if (message_arena != NULL && 
        ::google::protobuf::Arena::GetArena(*tensor) == NULL) {
      message_arena->Own(*tensor);
    } else if (message_arena !=
               ::google::protobuf::Arena::GetArena(*tensor)) {
      ::tensorflow::TensorProto* new_tensor = 
            ::google::protobuf::Arena::CreateMessage< ::tensorflow::TensorProto >(
            message_arena);
      new_tensor->CopyFrom(**tensor);
      *tensor = new_tensor;
    }
}
void NamedTensorProto::unsafe_arena_set_allocated_tensor(
    ::tensorflow::TensorProto* tensor) {
  if (GetArenaNoVirtual() == NULL) {
    delete tensor_;
  }
  tensor_ = tensor;
  if (tensor) {
    
  } else {
    
  }
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.NamedTensorProto.tensor)
}
#if !defined(_MSC_VER) || _MSC_VER >= 1900
const int NamedTensorProto::kNameFieldNumber;
const int NamedTensorProto::kTensorFieldNumber;
#endif  // !defined(_MSC_VER) || _MSC_VER >= 1900

NamedTensorProto::NamedTensorProto()
  : ::google::protobuf::Message(), _internal_metadata_(NULL) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:tensorflow.NamedTensorProto)
}

NamedTensorProto::NamedTensorProto(::google::protobuf::Arena* arena)
  : ::google::protobuf::Message(),
  _internal_metadata_(arena) {
  SharedCtor();
  RegisterArenaDtor(arena);
  // @@protoc_insertion_point(arena_constructor:tensorflow.NamedTensorProto)
}

void NamedTensorProto::InitAsDefaultInstance() {
  _is_default_instance_ = true;
  tensor_ = const_cast< ::tensorflow::TensorProto*>(&::tensorflow::TensorProto::default_instance());
}

NamedTensorProto::NamedTensorProto(const NamedTensorProto& from)
  : ::google::protobuf::Message(),
    _internal_metadata_(NULL) {
  SharedCtor();
  MergeFrom(from);
  // @@protoc_insertion_point(copy_constructor:tensorflow.NamedTensorProto)
}

void NamedTensorProto::SharedCtor() {
    _is_default_instance_ = false;
  ::google::protobuf::internal::GetEmptyString();
  _cached_size_ = 0;
  name_.UnsafeSetDefault(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
  tensor_ = NULL;
}

NamedTensorProto::~NamedTensorProto() {
  // @@protoc_insertion_point(destructor:tensorflow.NamedTensorProto)
  SharedDtor();
}

void NamedTensorProto::SharedDtor() {
  if (GetArenaNoVirtual() != NULL) {
    return;
  }

  name_.Destroy(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
  if (this != default_instance_) {
    delete tensor_;
  }
}

void NamedTensorProto::ArenaDtor(void* object) {
  NamedTensorProto* _this = reinterpret_cast< NamedTensorProto* >(object);
  (void)_this;
}
void NamedTensorProto::RegisterArenaDtor(::google::protobuf::Arena* arena) {
}
void NamedTensorProto::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* NamedTensorProto::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return NamedTensorProto_descriptor_;
}

const NamedTensorProto& NamedTensorProto::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fnamed_5ftensor_2eproto();
  return *default_instance_;
}

NamedTensorProto* NamedTensorProto::default_instance_ = NULL;

NamedTensorProto* NamedTensorProto::New(::google::protobuf::Arena* arena) const {
  return ::google::protobuf::Arena::CreateMessage<NamedTensorProto>(arena);
}

void NamedTensorProto::Clear() {
// @@protoc_insertion_point(message_clear_start:tensorflow.NamedTensorProto)
  name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
  if (GetArenaNoVirtual() == NULL && tensor_ != NULL) delete tensor_;
  tensor_ = NULL;
}

bool NamedTensorProto::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) goto failure
  ::google::protobuf::uint32 tag;
  // @@protoc_insertion_point(parse_start:tensorflow.NamedTensorProto)
  for (;;) {
    ::std::pair< ::google::protobuf::uint32, bool> p = input->ReadTagWithCutoff(127);
    tag = p.first;
    if (!p.second) goto handle_unusual;
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string name = 1;
      case 1: {
        if (tag == 10) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_name()));
          DO_(::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
            this->name().data(), this->name().length(),
            ::google::protobuf::internal::WireFormatLite::PARSE,
            "tensorflow.NamedTensorProto.name"));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectTag(18)) goto parse_tensor;
        break;
      }

      // optional .tensorflow.TensorProto tensor = 2;
      case 2: {
        if (tag == 18) {
         parse_tensor:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_tensor()));
        } else {
          goto handle_unusual;
        }
        if (input->ExpectAtEnd()) goto success;
        break;
      }

      default: {
      handle_unusual:
        if (tag == 0 ||
            ::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          goto success;
        }
        DO_(::google::protobuf::internal::WireFormatLite::SkipField(input, tag));
        break;
      }
    }
  }
success:
  // @@protoc_insertion_point(parse_success:tensorflow.NamedTensorProto)
  return true;
failure:
  // @@protoc_insertion_point(parse_failure:tensorflow.NamedTensorProto)
  return false;
#undef DO_
}

void NamedTensorProto::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // @@protoc_insertion_point(serialize_start:tensorflow.NamedTensorProto)
  // optional string name = 1;
  if (this->name().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->name().data(), this->name().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tensorflow.NamedTensorProto.name");
    ::google::protobuf::internal::WireFormatLite::WriteStringMaybeAliased(
      1, this->name(), output);
  }

  // optional .tensorflow.TensorProto tensor = 2;
  if (this->has_tensor()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      2, *this->tensor_, output);
  }

  // @@protoc_insertion_point(serialize_end:tensorflow.NamedTensorProto)
}

::google::protobuf::uint8* NamedTensorProto::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // @@protoc_insertion_point(serialize_to_array_start:tensorflow.NamedTensorProto)
  // optional string name = 1;
  if (this->name().size() > 0) {
    ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
      this->name().data(), this->name().length(),
      ::google::protobuf::internal::WireFormatLite::SERIALIZE,
      "tensorflow.NamedTensorProto.name");
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->name(), target);
  }

  // optional .tensorflow.TensorProto tensor = 2;
  if (this->has_tensor()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        2, *this->tensor_, target);
  }

  // @@protoc_insertion_point(serialize_to_array_end:tensorflow.NamedTensorProto)
  return target;
}

int NamedTensorProto::ByteSize() const {
// @@protoc_insertion_point(message_byte_size_start:tensorflow.NamedTensorProto)
  int total_size = 0;

  // optional string name = 1;
  if (this->name().size() > 0) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::StringSize(
        this->name());
  }

  // optional .tensorflow.TensorProto tensor = 2;
  if (this->has_tensor()) {
    total_size += 1 +
      ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
        *this->tensor_);
  }

  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void NamedTensorProto::MergeFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:tensorflow.NamedTensorProto)
  if (GOOGLE_PREDICT_FALSE(&from == this)) MergeFromFail(__LINE__);
  const NamedTensorProto* source = 
      ::google::protobuf::internal::DynamicCastToGenerated<const NamedTensorProto>(
          &from);
  if (source == NULL) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:tensorflow.NamedTensorProto)
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:tensorflow.NamedTensorProto)
    MergeFrom(*source);
  }
}

void NamedTensorProto::MergeFrom(const NamedTensorProto& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:tensorflow.NamedTensorProto)
  if (GOOGLE_PREDICT_FALSE(&from == this)) MergeFromFail(__LINE__);
  if (from.name().size() > 0) {
    set_name(from.name());
  }
  if (from.has_tensor()) {
    mutable_tensor()->::tensorflow::TensorProto::MergeFrom(from.tensor());
  }
}

void NamedTensorProto::CopyFrom(const ::google::protobuf::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:tensorflow.NamedTensorProto)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void NamedTensorProto::CopyFrom(const NamedTensorProto& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:tensorflow.NamedTensorProto)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool NamedTensorProto::IsInitialized() const {

  return true;
}

void NamedTensorProto::Swap(NamedTensorProto* other) {
  if (other == this) return;
  if (GetArenaNoVirtual() == other->GetArenaNoVirtual()) {
    InternalSwap(other);
  } else {
    NamedTensorProto temp;
    temp.MergeFrom(*this);
    CopyFrom(*other);
    other->CopyFrom(temp);
  }
}
void NamedTensorProto::UnsafeArenaSwap(NamedTensorProto* other) {
  if (other == this) return;
  GOOGLE_DCHECK(GetArenaNoVirtual() == other->GetArenaNoVirtual());
  InternalSwap(other);
}
void NamedTensorProto::InternalSwap(NamedTensorProto* other) {
  name_.Swap(&other->name_);
  std::swap(tensor_, other->tensor_);
  _internal_metadata_.Swap(&other->_internal_metadata_);
  std::swap(_cached_size_, other->_cached_size_);
}

::google::protobuf::Metadata NamedTensorProto::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = NamedTensorProto_descriptor_;
  metadata.reflection = NamedTensorProto_reflection_;
  return metadata;
}

#if PROTOBUF_INLINE_NOT_IN_HEADERS
// NamedTensorProto

// optional string name = 1;
void NamedTensorProto::clear_name() {
  name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
 const ::std::string& NamedTensorProto::name() const {
  // @@protoc_insertion_point(field_get:tensorflow.NamedTensorProto.name)
  return name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
 void NamedTensorProto::set_name(const ::std::string& value) {
  
  name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.NamedTensorProto.name)
}
 void NamedTensorProto::set_name(const char* value) {
  
  name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.NamedTensorProto.name)
}
 void NamedTensorProto::set_name(const char* value,
    size_t size) {
  
  name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.NamedTensorProto.name)
}
 ::std::string* NamedTensorProto::mutable_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.NamedTensorProto.name)
  return name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
 ::std::string* NamedTensorProto::release_name() {
  // @@protoc_insertion_point(field_release:tensorflow.NamedTensorProto.name)
  
  return name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
 ::std::string* NamedTensorProto::unsafe_arena_release_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.NamedTensorProto.name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
 void NamedTensorProto::set_allocated_name(::std::string* name) {
  if (name != NULL) {
    
  } else {
    
  }
  name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.NamedTensorProto.name)
}
 void NamedTensorProto::unsafe_arena_set_allocated_name(
    ::std::string* name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (name != NULL) {
    
  } else {
    
  }
  name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.NamedTensorProto.name)
}

// optional .tensorflow.TensorProto tensor = 2;
bool NamedTensorProto::has_tensor() const {
  return !_is_default_instance_ && tensor_ != NULL;
}
void NamedTensorProto::clear_tensor() {
  if (GetArenaNoVirtual() == NULL && tensor_ != NULL) delete tensor_;
  tensor_ = NULL;
}
const ::tensorflow::TensorProto& NamedTensorProto::tensor() const {
  // @@protoc_insertion_point(field_get:tensorflow.NamedTensorProto.tensor)
  return tensor_ != NULL ? *tensor_ : *default_instance_->tensor_;
}
::tensorflow::TensorProto* NamedTensorProto::mutable_tensor() {
  
  if (tensor_ == NULL) {
    _slow_mutable_tensor();
  }
  // @@protoc_insertion_point(field_mutable:tensorflow.NamedTensorProto.tensor)
  return tensor_;
}
::tensorflow::TensorProto* NamedTensorProto::release_tensor() {
  // @@protoc_insertion_point(field_release:tensorflow.NamedTensorProto.tensor)
  
  if (GetArenaNoVirtual() != NULL) {
    return _slow_release_tensor();
  } else {
    ::tensorflow::TensorProto* temp = tensor_;
    tensor_ = NULL;
    return temp;
  }
}
 void NamedTensorProto::set_allocated_tensor(::tensorflow::TensorProto* tensor) {
  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();
  if (message_arena == NULL) {
    delete tensor_;
  }
  if (tensor != NULL) {
    _slow_set_allocated_tensor(message_arena, &tensor);
  }
  tensor_ = tensor;
  if (tensor) {
    
  } else {
    
  }
  // @@protoc_insertion_point(field_set_allocated:tensorflow.NamedTensorProto.tensor)
}

#endif  // PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace tensorflow

// @@protoc_insertion_point(global_scope)
