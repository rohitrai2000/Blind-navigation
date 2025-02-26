// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: tensorflow/core/protobuf/queue_runner.proto

#ifndef PROTOBUF_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto__INCLUDED
#define PROTOBUF_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto__INCLUDED

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
void protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();
void protobuf_AssignDesc_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();
void protobuf_ShutdownFile_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();

class QueueRunnerDef;

// ===================================================================

class QueueRunnerDef : public ::google::protobuf::Message {
 public:
  QueueRunnerDef();
  virtual ~QueueRunnerDef();

  QueueRunnerDef(const QueueRunnerDef& from);

  inline QueueRunnerDef& operator=(const QueueRunnerDef& from) {
    CopyFrom(from);
    return *this;
  }

  inline ::google::protobuf::Arena* GetArena() const { return GetArenaNoVirtual(); }
  inline void* GetMaybeArenaPointer() const {
    return MaybeArenaPtr();
  }
  static const ::google::protobuf::Descriptor* descriptor();
  static const QueueRunnerDef& default_instance();

  void UnsafeArenaSwap(QueueRunnerDef* other);
  void Swap(QueueRunnerDef* other);

  // implements Message ----------------------------------------------

  inline QueueRunnerDef* New() const { return New(NULL); }

  QueueRunnerDef* New(::google::protobuf::Arena* arena) const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const QueueRunnerDef& from);
  void MergeFrom(const QueueRunnerDef& from);
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
  void InternalSwap(QueueRunnerDef* other);
  protected:
  explicit QueueRunnerDef(::google::protobuf::Arena* arena);
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

  // optional string queue_name = 1;
  void clear_queue_name();
  static const int kQueueNameFieldNumber = 1;
  const ::std::string& queue_name() const;
  void set_queue_name(const ::std::string& value);
  void set_queue_name(const char* value);
  void set_queue_name(const char* value, size_t size);
  ::std::string* mutable_queue_name();
  ::std::string* release_queue_name();
  void set_allocated_queue_name(::std::string* queue_name);
  ::std::string* unsafe_arena_release_queue_name();
  void unsafe_arena_set_allocated_queue_name(
      ::std::string* queue_name);

  // repeated string enqueue_op_name = 2;
  int enqueue_op_name_size() const;
  void clear_enqueue_op_name();
  static const int kEnqueueOpNameFieldNumber = 2;
  const ::std::string& enqueue_op_name(int index) const;
  ::std::string* mutable_enqueue_op_name(int index);
  void set_enqueue_op_name(int index, const ::std::string& value);
  void set_enqueue_op_name(int index, const char* value);
  void set_enqueue_op_name(int index, const char* value, size_t size);
  ::std::string* add_enqueue_op_name();
  void add_enqueue_op_name(const ::std::string& value);
  void add_enqueue_op_name(const char* value);
  void add_enqueue_op_name(const char* value, size_t size);
  const ::google::protobuf::RepeatedPtrField< ::std::string>& enqueue_op_name() const;
  ::google::protobuf::RepeatedPtrField< ::std::string>* mutable_enqueue_op_name();

  // optional string close_op_name = 3;
  void clear_close_op_name();
  static const int kCloseOpNameFieldNumber = 3;
  const ::std::string& close_op_name() const;
  void set_close_op_name(const ::std::string& value);
  void set_close_op_name(const char* value);
  void set_close_op_name(const char* value, size_t size);
  ::std::string* mutable_close_op_name();
  ::std::string* release_close_op_name();
  void set_allocated_close_op_name(::std::string* close_op_name);
  ::std::string* unsafe_arena_release_close_op_name();
  void unsafe_arena_set_allocated_close_op_name(
      ::std::string* close_op_name);

  // optional string cancel_op_name = 4;
  void clear_cancel_op_name();
  static const int kCancelOpNameFieldNumber = 4;
  const ::std::string& cancel_op_name() const;
  void set_cancel_op_name(const ::std::string& value);
  void set_cancel_op_name(const char* value);
  void set_cancel_op_name(const char* value, size_t size);
  ::std::string* mutable_cancel_op_name();
  ::std::string* release_cancel_op_name();
  void set_allocated_cancel_op_name(::std::string* cancel_op_name);
  ::std::string* unsafe_arena_release_cancel_op_name();
  void unsafe_arena_set_allocated_cancel_op_name(
      ::std::string* cancel_op_name);

  // @@protoc_insertion_point(class_scope:tensorflow.QueueRunnerDef)
 private:

  ::google::protobuf::internal::InternalMetadataWithArena _internal_metadata_;
  friend class ::google::protobuf::Arena;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  bool _is_default_instance_;
  ::google::protobuf::internal::ArenaStringPtr queue_name_;
  ::google::protobuf::RepeatedPtrField< ::std::string> enqueue_op_name_;
  ::google::protobuf::internal::ArenaStringPtr close_op_name_;
  ::google::protobuf::internal::ArenaStringPtr cancel_op_name_;
  mutable int _cached_size_;
  friend void  protobuf_AddDesc_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();
  friend void protobuf_AssignDesc_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();
  friend void protobuf_ShutdownFile_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto();

  void InitAsDefaultInstance();
  static QueueRunnerDef* default_instance_;
};
// ===================================================================


// ===================================================================

#if !PROTOBUF_INLINE_NOT_IN_HEADERS
// QueueRunnerDef

// optional string queue_name = 1;
inline void QueueRunnerDef::clear_queue_name() {
  queue_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& QueueRunnerDef::queue_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.QueueRunnerDef.queue_name)
  return queue_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void QueueRunnerDef::set_queue_name(const ::std::string& value) {
  
  queue_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.QueueRunnerDef.queue_name)
}
inline void QueueRunnerDef::set_queue_name(const char* value) {
  
  queue_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.QueueRunnerDef.queue_name)
}
inline void QueueRunnerDef::set_queue_name(const char* value,
    size_t size) {
  
  queue_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.QueueRunnerDef.queue_name)
}
inline ::std::string* QueueRunnerDef::mutable_queue_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.QueueRunnerDef.queue_name)
  return queue_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::release_queue_name() {
  // @@protoc_insertion_point(field_release:tensorflow.QueueRunnerDef.queue_name)
  
  return queue_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::unsafe_arena_release_queue_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.QueueRunnerDef.queue_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return queue_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void QueueRunnerDef::set_allocated_queue_name(::std::string* queue_name) {
  if (queue_name != NULL) {
    
  } else {
    
  }
  queue_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), queue_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.QueueRunnerDef.queue_name)
}
inline void QueueRunnerDef::unsafe_arena_set_allocated_queue_name(
    ::std::string* queue_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (queue_name != NULL) {
    
  } else {
    
  }
  queue_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      queue_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.QueueRunnerDef.queue_name)
}

// repeated string enqueue_op_name = 2;
inline int QueueRunnerDef::enqueue_op_name_size() const {
  return enqueue_op_name_.size();
}
inline void QueueRunnerDef::clear_enqueue_op_name() {
  enqueue_op_name_.Clear();
}
inline const ::std::string& QueueRunnerDef::enqueue_op_name(int index) const {
  // @@protoc_insertion_point(field_get:tensorflow.QueueRunnerDef.enqueue_op_name)
  return enqueue_op_name_.Get(index);
}
inline ::std::string* QueueRunnerDef::mutable_enqueue_op_name(int index) {
  // @@protoc_insertion_point(field_mutable:tensorflow.QueueRunnerDef.enqueue_op_name)
  return enqueue_op_name_.Mutable(index);
}
inline void QueueRunnerDef::set_enqueue_op_name(int index, const ::std::string& value) {
  // @@protoc_insertion_point(field_set:tensorflow.QueueRunnerDef.enqueue_op_name)
  enqueue_op_name_.Mutable(index)->assign(value);
}
inline void QueueRunnerDef::set_enqueue_op_name(int index, const char* value) {
  enqueue_op_name_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:tensorflow.QueueRunnerDef.enqueue_op_name)
}
inline void QueueRunnerDef::set_enqueue_op_name(int index, const char* value, size_t size) {
  enqueue_op_name_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:tensorflow.QueueRunnerDef.enqueue_op_name)
}
inline ::std::string* QueueRunnerDef::add_enqueue_op_name() {
  // @@protoc_insertion_point(field_add_mutable:tensorflow.QueueRunnerDef.enqueue_op_name)
  return enqueue_op_name_.Add();
}
inline void QueueRunnerDef::add_enqueue_op_name(const ::std::string& value) {
  enqueue_op_name_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:tensorflow.QueueRunnerDef.enqueue_op_name)
}
inline void QueueRunnerDef::add_enqueue_op_name(const char* value) {
  enqueue_op_name_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:tensorflow.QueueRunnerDef.enqueue_op_name)
}
inline void QueueRunnerDef::add_enqueue_op_name(const char* value, size_t size) {
  enqueue_op_name_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:tensorflow.QueueRunnerDef.enqueue_op_name)
}
inline const ::google::protobuf::RepeatedPtrField< ::std::string>&
QueueRunnerDef::enqueue_op_name() const {
  // @@protoc_insertion_point(field_list:tensorflow.QueueRunnerDef.enqueue_op_name)
  return enqueue_op_name_;
}
inline ::google::protobuf::RepeatedPtrField< ::std::string>*
QueueRunnerDef::mutable_enqueue_op_name() {
  // @@protoc_insertion_point(field_mutable_list:tensorflow.QueueRunnerDef.enqueue_op_name)
  return &enqueue_op_name_;
}

// optional string close_op_name = 3;
inline void QueueRunnerDef::clear_close_op_name() {
  close_op_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& QueueRunnerDef::close_op_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.QueueRunnerDef.close_op_name)
  return close_op_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void QueueRunnerDef::set_close_op_name(const ::std::string& value) {
  
  close_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.QueueRunnerDef.close_op_name)
}
inline void QueueRunnerDef::set_close_op_name(const char* value) {
  
  close_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.QueueRunnerDef.close_op_name)
}
inline void QueueRunnerDef::set_close_op_name(const char* value,
    size_t size) {
  
  close_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.QueueRunnerDef.close_op_name)
}
inline ::std::string* QueueRunnerDef::mutable_close_op_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.QueueRunnerDef.close_op_name)
  return close_op_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::release_close_op_name() {
  // @@protoc_insertion_point(field_release:tensorflow.QueueRunnerDef.close_op_name)
  
  return close_op_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::unsafe_arena_release_close_op_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.QueueRunnerDef.close_op_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return close_op_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void QueueRunnerDef::set_allocated_close_op_name(::std::string* close_op_name) {
  if (close_op_name != NULL) {
    
  } else {
    
  }
  close_op_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), close_op_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.QueueRunnerDef.close_op_name)
}
inline void QueueRunnerDef::unsafe_arena_set_allocated_close_op_name(
    ::std::string* close_op_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (close_op_name != NULL) {
    
  } else {
    
  }
  close_op_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      close_op_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.QueueRunnerDef.close_op_name)
}

// optional string cancel_op_name = 4;
inline void QueueRunnerDef::clear_cancel_op_name() {
  cancel_op_name_.ClearToEmpty(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline const ::std::string& QueueRunnerDef::cancel_op_name() const {
  // @@protoc_insertion_point(field_get:tensorflow.QueueRunnerDef.cancel_op_name)
  return cancel_op_name_.Get(&::google::protobuf::internal::GetEmptyStringAlreadyInited());
}
inline void QueueRunnerDef::set_cancel_op_name(const ::std::string& value) {
  
  cancel_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), value, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set:tensorflow.QueueRunnerDef.cancel_op_name)
}
inline void QueueRunnerDef::set_cancel_op_name(const char* value) {
  
  cancel_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(value),
              GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_char:tensorflow.QueueRunnerDef.cancel_op_name)
}
inline void QueueRunnerDef::set_cancel_op_name(const char* value,
    size_t size) {
  
  cancel_op_name_.Set(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), ::std::string(
      reinterpret_cast<const char*>(value), size), GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_pointer:tensorflow.QueueRunnerDef.cancel_op_name)
}
inline ::std::string* QueueRunnerDef::mutable_cancel_op_name() {
  
  // @@protoc_insertion_point(field_mutable:tensorflow.QueueRunnerDef.cancel_op_name)
  return cancel_op_name_.Mutable(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::release_cancel_op_name() {
  // @@protoc_insertion_point(field_release:tensorflow.QueueRunnerDef.cancel_op_name)
  
  return cancel_op_name_.Release(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), GetArenaNoVirtual());
}
inline ::std::string* QueueRunnerDef::unsafe_arena_release_cancel_op_name() {
  // @@protoc_insertion_point(field_unsafe_arena_release:tensorflow.QueueRunnerDef.cancel_op_name)
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  
  return cancel_op_name_.UnsafeArenaRelease(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      GetArenaNoVirtual());
}
inline void QueueRunnerDef::set_allocated_cancel_op_name(::std::string* cancel_op_name) {
  if (cancel_op_name != NULL) {
    
  } else {
    
  }
  cancel_op_name_.SetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(), cancel_op_name,
      GetArenaNoVirtual());
  // @@protoc_insertion_point(field_set_allocated:tensorflow.QueueRunnerDef.cancel_op_name)
}
inline void QueueRunnerDef::unsafe_arena_set_allocated_cancel_op_name(
    ::std::string* cancel_op_name) {
  GOOGLE_DCHECK(GetArenaNoVirtual() != NULL);
  if (cancel_op_name != NULL) {
    
  } else {
    
  }
  cancel_op_name_.UnsafeArenaSetAllocated(&::google::protobuf::internal::GetEmptyStringAlreadyInited(),
      cancel_op_name, GetArenaNoVirtual());
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:tensorflow.QueueRunnerDef.cancel_op_name)
}

#endif  // !PROTOBUF_INLINE_NOT_IN_HEADERS

// @@protoc_insertion_point(namespace_scope)

}  // namespace tensorflow

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_tensorflow_2fcore_2fprotobuf_2fqueue_5frunner_2eproto__INCLUDED
