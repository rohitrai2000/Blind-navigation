// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tensorflow/core/framework/variable.proto

#ifndef PROTOBUF_tensorflow_2fcore_2fframework_2fvariable_2eproto__INCLUDED
#define PROTOBUF_tensorflow_2fcore_2fframework_2fvariable_2eproto__INCLUDED

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
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace tensorflow {

// Internal implementation detail -- do not call these.
void protobuf_AddDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
void protobuf_AssignDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
void protobuf_ShutdownFile_tensorflow_2fcore_2fframework_2fvariable_2eproto();

class SaveSliceInfoDef;
class VariableDef;

// ===================================================================

class VariableDef : public ::google::protobuf::Message {
 public:
  VariableDef();
  virtual ~VariableDef();

  VariableDef(const VariableDef& from);

  inline VariableDef& operator=(const VariableDef& from) {
    CopyFrom(from);
    return *this;
  }

  inline ::google::protobuf::Arena* GetArena() const { return GetArenaNoVirtual(); }
  inline void* GetMaybeArenaPointer() const {
    return MaybeArenaPtr();
  }
  static const ::google::protobuf::Descriptor* descriptor();
  static const VariableDef& default_instance();

  void UnsafeArenaSwap(VariableDef* other);
  void Swap(VariableDef* other);

  // implements Message ----------------------------------------------

  inline VariableDef* New() const { return New(NULL); }

  VariableDef* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const VariableDef& from);
  void MergeFrom(const VariableDef& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(VariableDef* other);
  protected:
  explicit VariableDef(::google::protobuf::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::google::protobuf::Arena* arena);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string variable_name = 1;
  void clear_variable_name();
  static const int kVariableNameFieldNumber = 1;
  const ::std::string& variable_name() const;
  void set_variable_name(const ::std::string& value);
  void set_variable_name(const char* value);
  void set_variable_name(const char* value, size_t size);
  ::std::string* mutable_variable_name();
  ::std::string* release_variable_name();
  void set_allocated_variable_name(::std::string* variable_name);
  ::std::string* unsafe_arena_release_variable_name();
  void unsafe_arena_set_allocated_variable_name(
      ::std::string* variable_name);

  // optional string initializer_name = 2;
  void clear_initializer_name();
  static const int kInitializerNameFieldNumber = 2;
  const ::std::string& initializer_name() const;
  void set_initializer_name(const ::std::string& value);
  void set_initializer_name(const char* value);
  void set_initializer_name(const char* value, size_t size);
  ::std::string* mutable_initializer_name();
  ::std::string* release_initializer_name();
  void set_allocated_initializer_name(::std::string* initializer_name);
  ::std::string* unsafe_arena_release_initializer_name();
  void unsafe_arena_set_allocated_initializer_name(
      ::std::string* initializer_name);

  // optional string snapshot_name = 3;
  void clear_snapshot_name();
  static const int kSnapshotNameFieldNumber = 3;
  const ::std::string& snapshot_name() const;
  void set_snapshot_name(const ::std::string& value);
  void set_snapshot_name(const char* value);
  void set_snapshot_name(const char* value, size_t size);
  ::std::string* mutable_snapshot_name();
  ::std::string* release_snapshot_name();
  void set_allocated_snapshot_name(::std::string* snapshot_name);
  ::std::string* unsafe_arena_release_snapshot_name();
  void unsafe_arena_set_allocated_snapshot_name(
      ::std::string* snapshot_name);

  // optional .tensorflow.SaveSliceInfoDef save_slice_info_def = 4;
  bool has_save_slice_info_def() const;
  void clear_save_slice_info_def();
  static const int kSaveSliceInfoDefFieldNumber = 4;
  private:
  void _slow_mutable_save_slice_info_def();
  void _slow_set_allocated_save_slice_info_def(
      ::google::protobuf::Arena* message_arena, ::tensorflow::SaveSliceInfoDef** save_slice_info_def);
  ::tensorflow::SaveSliceInfoDef* _slow_release_save_slice_info_def();
  public:
  const ::tensorflow::SaveSliceInfoDef& save_slice_info_def() const;
  ::tensorflow::SaveSliceInfoDef* mutable_save_slice_info_def();
  ::tensorflow::SaveSliceInfoDef* release_save_slice_info_def();
  void set_allocated_save_slice_info_def(::tensorflow::SaveSliceInfoDef* save_slice_info_def);
  ::tensorflow::SaveSliceInfoDef* unsafe_arena_release_save_slice_info_def();
  void unsafe_arena_set_allocated_save_slice_info_def(
      ::tensorflow::SaveSliceInfoDef* save_slice_info_def);

  // @@protoc_insertion_point(class_scope:tensorflow.VariableDef)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  friend class ::google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr variable_name_;
  ::google::protobuf::internal::ArenaStringPtr initializer_name_;
  ::google::protobuf::internal::ArenaStringPtr snapshot_name_;
  ::tensorflow::SaveSliceInfoDef* save_slice_info_def_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
  friend void protobuf_AssignDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
  friend void protobuf_ShutdownFile_tensorflow_2fcore_2fframework_2fvariable_2eproto();

  void InitAsDefaultInstance();
  static VariableDef* default_instance_;
};
// -------------------------------------------------------------------

class SaveSliceInfoDef : public ::google::protobuf::Message {
 public:
  SaveSliceInfoDef();
  virtual ~SaveSliceInfoDef();

  SaveSliceInfoDef(const SaveSliceInfoDef& from);

  inline SaveSliceInfoDef& operator=(const SaveSliceInfoDef& from) {
    CopyFrom(from);
    return *this;
  }

  inline ::google::protobuf::Arena* GetArena() const { return GetArenaNoVirtual(); }
  inline void* GetMaybeArenaPointer() const {
    return MaybeArenaPtr();
  }
  static const ::google::protobuf::Descriptor* descriptor();
  static const SaveSliceInfoDef& default_instance();

  void UnsafeArenaSwap(SaveSliceInfoDef* other);
  void Swap(SaveSliceInfoDef* other);

  // implements Message ----------------------------------------------

  inline SaveSliceInfoDef* New() const { return New(NULL); }

  SaveSliceInfoDef* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const SaveSliceInfoDef& from);
  void MergeFrom(const SaveSliceInfoDef& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  void InternalSwap(SaveSliceInfoDef* other);
  protected:
  explicit SaveSliceInfoDef(::google::protobuf::Arena* arena);
  private:
  static void ArenaDtor(void* object);
  inline void RegisterArenaDtor(::google::protobuf::Arena* arena);
  private:
  inline ::google::protobuf::Arena* GetArenaNoVirtual() const {
    return _internal_metadata_.arena();
  }
  inline void* MaybeArenaPtr() const {
    return _internal_metadata_.raw_arena_ptr();
  }
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // optional string full_name = 1;
  void clear_full_name();
  static const int kFullNameFieldNumber = 1;
  const ::std::string& full_name() const;
  void set_full_name(const ::std::string& value);
  void set_full_name(const char* value);
  void set_full_name(const char* value, size_t size);
  ::std::string* mutable_full_name();
  ::std::string* release_full_name();
  void set_allocated_full_name(::std::string* full_name);
  ::std::string* unsafe_arena_release_full_name();
  void unsafe_arena_set_allocated_full_name(
      ::std::string* full_name);

  // repeated int32 full_shape = 2;
  int full_shape_size() const;
  void clear_full_shape();
  static const int kFullShapeFieldNumber = 2;
  ::google::protobuf::int32 full_shape(int index) const;
  void set_full_shape(int index, ::google::protobuf::int32 value);
  void add_full_shape(::google::protobuf::int32 value);
  const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      full_shape() const;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_full_shape();

  // repeated int32 var_offset = 3;
  int var_offset_size() const;
  void clear_var_offset();
  static const int kVarOffsetFieldNumber = 3;
  ::google::protobuf::int32 var_offset(int index) const;
  void set_var_offset(int index, ::google::protobuf::int32 value);
  void add_var_offset(::google::protobuf::int32 value);
  const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      var_offset() const;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_var_offset();

  // repeated int32 var_shape = 4;
  int var_shape_size() const;
  void clear_var_shape();
  static const int kVarShapeFieldNumber = 4;
  ::google::protobuf::int32 var_shape(int index) const;
  void set_var_shape(int index, ::google::protobuf::int32 value);
  void add_var_shape(::google::protobuf::int32 value);
  const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
      var_shape() const;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
      mutable_var_shape();

  // @@protoc_insertion_point(class_scope:tensorflow.SaveSliceInfoDef)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  friend class ::google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr full_name_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > full_shape_;
  mutable int _full_shape_cached_byte_size_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > var_offset_;
  mutable int _var_offset_cached_byte_size_;
  ::google::protobuf::RepeatedField< ::google::protobuf::int32 > var_shape_;
  mutable int _var_shape_cached_byte_size_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
  friend void protobuf_AssignDesc_tensorflow_2fcore_2fframework_2fvariable_2eproto();
  friend void protobuf_ShutdownFile_tensorflow_2fcore_2fframework_2fvariable_2eproto();

  void InitAsDefaultInstance();
  static SaveSliceInfoDef* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// VariableDef

// optional string variable_name = 1;
inline void VariableDef::clear_variable_name() {
  variable_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& VariableDef::variable_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.VariableDef.variable_name)
  return variable_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void VariableDef::set_variable_name(const ::std::string& value) {
  
  variable_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.VariableDef.variable_name)
}
inline void VariableDef::set_variable_name(const char* value) {
  
  variable_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.VariableDef.variable_name)
}
inline void VariableDef::set_variable_name(const char* value,
    size_t size) {
  
  variable_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.VariableDef.variable_name)
}
inline ::std::string* VariableDef::mutable_variable_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.VariableDef.variable_name)
  return variable_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::release_variable_name() {
  // @@protoc_insertion_point(field_release:tensorflow.VariableDef.variable_name)
  
  return variable_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::unsafe_arena_release_variable_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.VariableDef.variable_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return variable_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void VariableDef::set_allocated_variable_name(::std::string* variable_name) {
  if (variable_name != NULL) {
    
  } else {
    
  }
  variable_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), variable_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.VariableDef.variable_name)
}
inline void VariableDef::unsafe_arena_set_allocated_variable_name(
    ::std::string* variable_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (variable_name != NULL) {
    
  } else {
    
  }
  variable_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      variable_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.VariableDef.variable_name)
}

// optional string initializer_name = 2;
inline void VariableDef::clear_initializer_name() {
  initializer_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& VariableDef::initializer_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.VariableDef.initializer_name)
  return initializer_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void VariableDef::set_initializer_name(const ::std::string& value) {
  
  initializer_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.VariableDef.initializer_name)
}
inline void VariableDef::set_initializer_name(const char* value) {
  
  initializer_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.VariableDef.initializer_name)
}
inline void VariableDef::set_initializer_name(const char* value,
    size_t size) {
  
  initializer_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.VariableDef.initializer_name)
}
inline ::std::string* VariableDef::mutable_initializer_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.VariableDef.initializer_name)
  return initializer_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::release_initializer_name() {
  // @@protoc_insertion_point(field_release:tensorflow.VariableDef.initializer_name)
  
  return initializer_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::unsafe_arena_release_initializer_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.VariableDef.initializer_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return initializer_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void VariableDef::set_allocated_initializer_name(::std::string* initializer_name) {
  if (initializer_name != NULL) {
    
  } else {
    
  }
  initializer_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), initializer_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.VariableDef.initializer_name)
}
inline void VariableDef::unsafe_arena_set_allocated_initializer_name(
    ::std::string* initializer_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (initializer_name != NULL) {
    
  } else {
    
  }
  initializer_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      initializer_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.VariableDef.initializer_name)
}

// optional string snapshot_name = 3;
inline void VariableDef::clear_snapshot_name() {
  snapshot_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& VariableDef::snapshot_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.VariableDef.snapshot_name)
  return snapshot_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void VariableDef::set_snapshot_name(const ::std::string& value) {
  
  snapshot_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.VariableDef.snapshot_name)
}
inline void VariableDef::set_snapshot_name(const char* value) {
  
  snapshot_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.VariableDef.snapshot_name)
}
inline void VariableDef::set_snapshot_name(const char* value,
    size_t size) {
  
  snapshot_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.VariableDef.snapshot_name)
}
inline ::std::string* VariableDef::mutable_snapshot_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.VariableDef.snapshot_name)
  return snapshot_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::release_snapshot_name() {
  // @@protoc_insertion_point(field_release:tensorflow.VariableDef.snapshot_name)
  
  return snapshot_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* VariableDef::unsafe_arena_release_snapshot_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.VariableDef.snapshot_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return snapshot_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void VariableDef::set_allocated_snapshot_name(::std::string* snapshot_name) {
  if (snapshot_name != NULL) {
    
  } else {
    
  }
  snapshot_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), snapshot_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.VariableDef.snapshot_name)
}
inline void VariableDef::unsafe_arena_set_allocated_snapshot_name(
    ::std::string* snapshot_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (snapshot_name != NULL) {
    
  } else {
    
  }
  snapshot_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      snapshot_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.VariableDef.snapshot_name)
}

// optional .tensorflow.SaveSliceInfoDef save_slice_info_def = 4;
inline bool VariableDef::has_save_slice_info_def() const {
  return !_is_default_instance_ && save_slice_info_def_ != NULL;
}
inline void VariableDef::clear_save_slice_info_def() {
  if (GetArenaNoVirtual() == NULL && save_slice_info_def_ != NULL) delete save_slice_info_def_;
  save_slice_info_def_ = NULL;
}
inline const ::tensorflow::SaveSliceInfoDef& VariableDef::save_slice_info_def() const {
  // @@protoc_insertion_point(field_get:tensorflow.VariableDef.save_slice_info_def)
  return save_slice_info_def_ != NULL ? *save_slice_info_def_ : *default_instance_->save_slice_info_def_;
}
inline ::tensorflow::SaveSliceInfoDef* VariableDef::mutable_save_slice_info_def() {
  
  if (save_slice_info_def_ == NULL) {
    _slow_mutable_save_slice_info_def();
  }
  // @@protoc_insertion_point(field_mutable:tensorflow.VariableDef.save_slice_info_def)
  return save_slice_info_def_;
}
inline ::tensorflow::SaveSliceInfoDef* VariableDef::release_save_slice_info_def() {
  // @@protoc_insertion_point(field_release:tensorflow.VariableDef.save_slice_info_def)
  
  if (GetArenaNoVirtual() != NULL) {
    return _slow_release_save_slice_info_def();
  } else {
    ::tensorflow::SaveSliceInfoDef* temp = save_slice_info_def_;
    save_slice_info_def_ = NULL;
    return temp;
  }
}
inline  void VariableDef::set_allocated_save_slice_info_def(::tensorflow::SaveSliceInfoDef* save_slice_info_def) {
  ::google::protobuf::Arena* message_arena = GetArenaNoVirtual();
  if (message_arena == NULL) {
    delete save_slice_info_def_;
  }
  if (save_slice_info_def != NULL) {
    _slow_set_allocated_save_slice_info_def(message_arena, &save_slice_info_def);
  }
  save_slice_info_def_ = save_slice_info_def;
  if (save_slice_info_def) {
    
  } else {
    
  }
  // @@protoc_insertion_point(field_set_allocated:tensorflow.VariableDef.save_slice_info_def)
}

// -------------------------------------------------------------------

// SaveSliceInfoDef

// optional string full_name = 1;
inline void SaveSliceInfoDef::clear_full_name() {
  full_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& SaveSliceInfoDef::full_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.SaveSliceInfoDef.full_name)
  return full_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void SaveSliceInfoDef::set_full_name(const ::std::string& value) {
  
  full_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.SaveSliceInfoDef.full_name)
}
inline void SaveSliceInfoDef::set_full_name(const char* value) {
  
  full_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.SaveSliceInfoDef.full_name)
}
inline void SaveSliceInfoDef::set_full_name(const char* value,
    size_t size) {
  
  full_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.SaveSliceInfoDef.full_name)
}
inline ::std::string* SaveSliceInfoDef::mutable_full_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.SaveSliceInfoDef.full_name)
  return full_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* SaveSliceInfoDef::release_full_name() {
  // @@protoc_insertion_point(field_release:tensorflow.SaveSliceInfoDef.full_name)
  
  return full_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* SaveSliceInfoDef::unsafe_arena_release_full_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.SaveSliceInfoDef.full_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return full_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void SaveSliceInfoDef::set_allocated_full_name(::std::string* full_name) {
  if (full_name != NULL) {
    
  } else {
    
  }
  full_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), full_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.SaveSliceInfoDef.full_name)
}
inline void SaveSliceInfoDef::unsafe_arena_set_allocated_full_name(
    ::std::string* full_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (full_name != NULL) {
    
  } else {
    
  }
  full_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      full_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.SaveSliceInfoDef.full_name)
}

// repeated int32 full_shape = 2;
inline int SaveSliceInfoDef::full_shape_size() const {
  return full_shape_.size();
}
inline void SaveSliceInfoDef::clear_full_shape() {
  full_shape_.Clear();
}
inline ::google::protobuf::int32 SaveSliceInfoDef::full_shape(int index) const {
  // @@protoc_insertion_point(field_get:tensorflow.SaveSliceInfoDef.full_shape)
  return full_shape_.Get(index);
}
inline void SaveSliceInfoDef::set_full_shape(int index, ::google::protobuf::int32 value) {
  full_shape_.Set(index, value);
  // @@protoc_insertion_point(field_set:tensorflow.SaveSliceInfoDef.full_shape)
}
inline void SaveSliceInfoDef::add_full_shape(::google::protobuf::int32 value) {
  full_shape_.Add(value);
  // @@protoc_insertion_point(field_add:tensorflow.SaveSliceInfoDef.full_shape)
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
SaveSliceInfoDef::full_shape() const {
  // @@protoc_insertion_point(field_list:tensorflow.SaveSliceInfoDef.full_shape)
  return full_shape_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
SaveSliceInfoDef::mutable_full_shape() {
  // @@protoc_insertion_point(field_mutable_list:tensorflow.SaveSliceInfoDef.full_shape)
  return &full_shape_;
}

// repeated int32 var_offset = 3;
inline int SaveSliceInfoDef::var_offset_size() const {
  return var_offset_.size();
}
inline void SaveSliceInfoDef::clear_var_offset() {
  var_offset_.Clear();
}
inline ::google::protobuf::int32 SaveSliceInfoDef::var_offset(int index) const {
  // @@protoc_insertion_point(field_get:tensorflow.SaveSliceInfoDef.var_offset)
  return var_offset_.Get(index);
}
inline void SaveSliceInfoDef::set_var_offset(int index, ::google::protobuf::int32 value) {
  var_offset_.Set(index, value);
  // @@protoc_insertion_point(field_set:tensorflow.SaveSliceInfoDef.var_offset)
}
inline void SaveSliceInfoDef::add_var_offset(::google::protobuf::int32 value) {
  var_offset_.Add(value);
  // @@protoc_insertion_point(field_add:tensorflow.SaveSliceInfoDef.var_offset)
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
SaveSliceInfoDef::var_offset() const {
  // @@protoc_insertion_point(field_list:tensorflow.SaveSliceInfoDef.var_offset)
  return var_offset_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
SaveSliceInfoDef::mutable_var_offset() {
  // @@protoc_insertion_point(field_mutable_list:tensorflow.SaveSliceInfoDef.var_offset)
  return &var_offset_;
}

// repeated int32 var_shape = 4;
inline int SaveSliceInfoDef::var_shape_size() const {
  return var_shape_.size();
}
inline void SaveSliceInfoDef::clear_var_shape() {
  var_shape_.Clear();
}
inline ::google::protobuf::int32 SaveSliceInfoDef::var_shape(int index) const {
  // @@protoc_insertion_point(field_get:tensorflow.SaveSliceInfoDef.var_shape)
  return var_shape_.Get(index);
}
inline void SaveSliceInfoDef::set_var_shape(int index, ::google::protobuf::int32 value) {
  var_shape_.Set(index, value);
  // @@protoc_insertion_point(field_set:tensorflow.SaveSliceInfoDef.var_shape)
}
inline void SaveSliceInfoDef::add_var_shape(::google::protobuf::int32 value) {
  var_shape_.Add(value);
  // @@protoc_insertion_point(field_add:tensorflow.SaveSliceInfoDef.var_shape)
}
inline const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&
SaveSliceInfoDef::var_shape() const {
  // @@protoc_insertion_point(field_list:tensorflow.SaveSliceInfoDef.var_shape)
  return var_shape_;
}
inline ::google::protobuf::RepeatedField< ::google::protobuf::int32 >*
SaveSliceInfoDef::mutable_var_shape() {
  // @@protoc_insertion_point(field_mutable_list:tensorflow.SaveSliceInfoDef.var_shape)
  return &var_shape_;
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS
// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace tensorflow

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_tensorflow_2fcore_2fframework_2fvariable_2eproto__INCLUDED
