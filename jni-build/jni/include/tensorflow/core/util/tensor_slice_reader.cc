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

#include "tensorflow/core/util/tensor_slice_reader.h"

#include <vector>
#include "tensorflow/core/framework/types.pb_text.h"
#include "tensorflow/core/framework/versions.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/gtl/stl_util.h"
#include "tensorflow/core/lib/io/iterator.h"
#include "tensorflow/core/lib/io/match.h"
#include "tensorflow/core/lib/io/table.h"
#include "tensorflow/core/lib/io/table_options.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/public/version.h"
#include "tensorflow/core/util/saved_tensor_slice_util.h"
#include "tensorflow/core/util/tensor_slice_util.h"

namespace tensorflow {

namespace checkpoint {

TensorSliceReader::Table::~Table() {}

namespace {
class TensorSliceReaderTable : public TensorSliceReader::Table {
 public:
  // Takes ownership of 'f'.
  explicit TensorSliceReaderTable(RandomAccessFile* f, table::Table* t)
      : file_(f), table_(t) {}

  ~TensorSliceReaderTable() override {
    delete table_;
    delete file_;
  }

  bool Get(const string& key, string* value) override {
    std::unique_ptr<table::Iterator> iter(table_->NewIterator());
    iter->Seek(key);
    if (iter->Valid() && iter->key() == key) {
      StringPiece v = iter->value();
      value->assign(v.data(), v.size());
      return true;
    } else {
      return false;
    }
  }

 private:
  RandomAccessFile* file_;  // Owns.
  table::Table* table_;
};
}  // namespace

Status OpenTableTensorSliceReader(const string& fname,
                                  TensorSliceReader::Table** result) {
  *result = nullptr;
  Env* env = Env::Default();
  std::unique_ptr<RandomAccessFile> f;
  Status s = env->NewRandomAccessFile(fname, &f);
  if (s.ok()) {
    uint64 file_size;
    s = env->GetFileSize(fname, &file_size);
    if (s.ok()) {
      table::Options options;
      table::Table* table;
      s = table::Table::Open(options, f.get(), file_size, &table);
      if (s.ok()) {
        *result = new TensorSliceReaderTable(f.release(), table);
        return Status::OK();
      } else {
        s = Status(s.code(),
                   strings::StrCat(s.error_message(),
                                   ": perhaps your file is in a different "
                                   "file format and you need to use a "
                                   "different restore operator?"));
      }
    }
  }
  LOG(WARNING) << "Could not open " << fname << ": " << s;
  return s;
}

TensorSliceReader::TensorSliceReader(const string& filepattern)
    : TensorSliceReader(filepattern, OpenTableTensorSliceReader,
                        kLoadAllShards) {}

TensorSliceReader::TensorSliceReader(const string& filepattern,
                                     OpenTableFunction open_function)
    : TensorSliceReader(filepattern, open_function, kLoadAllShards) {}

TensorSliceReader::TensorSliceReader(const string& filepattern,
                                     OpenTableFunction open_function,
                                     int preferred_shard)
    : filepattern_(filepattern), open_function_(open_function) {
  VLOG(1) << "TensorSliceReader for " << filepattern;
  Status s = io::GetMatchingFiles(Env::Default(), filepattern, &fnames_);
  if (!s.ok()) {
    status_ = errors::InvalidArgument(
        "Unsuccessful TensorSliceReader constructor: "
        "Failed to get matching files on ",
        filepattern, ": ", s.ToString());
    return;
  }
  if (fnames_.empty()) {
    status_ = errors::NotFound(
        "Unsuccessful TensorSliceReader constructor: "
        "Failed to find any matching files for ",
        filepattern);
    return;
  }
  sss_.resize(fnames_.size());
  for (size_t shard = 0; shard < fnames_.size(); ++shard) {
    fname_to_index_.insert(std::make_pair(fnames_[shard], shard));
  }
  if (preferred_shard == kLoadAllShards || fnames_.size() == 1 ||
      static_cast<size_t>(preferred_shard) >= fnames_.size()) {
    LoadAllShards();
  } else {
    VLOG(1) << "Loading shard " << preferred_shard << " for " << filepattern_;
    LoadShard(preferred_shard);
  }
}

void TensorSliceReader::LoadShard(int shard) const {
  CHECK_LT(shard, sss_.size());
  if (sss_[shard] || !status_.ok()) {
    return;  // Already loaded, or invalid.
  }
  string value;
  SavedTensorSlices sts;
  const string fname = fnames_[shard];
  VLOG(1) << "Reading meta data from file " << fname << "...";
  Table* table;
  Status s = open_function_(fname, &table);
  if (!s.ok()) {
    status_ = errors::DataLoss("Unable to open table file ", fname, ": ",
                               s.ToString());
    return;
  }
  sss_[shard].reset(table);
  if (!(table->Get(kSavedTensorSlicesKey, &value) &&
        ParseProtoUnlimited(&sts, value))) {
    status_ = errors::Internal(
        "Failed to find the saved tensor slices at the beginning of the "
        "checkpoint file: ",
        fname);
    return;
  }
  status_ = CheckVersions(sts.meta().versions(), TF_CHECKPOINT_VERSION,
                          TF_CHECKPOINT_VERSION_MIN_PRODUCER, "Checkpoint",
                          "checkpoint");
  if (!status_.ok()) return;
  for (const SavedSliceMeta& ssm : sts.meta().tensor()) {
    TensorShape ssm_shape(ssm.shape());
    for (const TensorSliceProto& tsp : ssm.slice()) {
      TensorSlice ss_slice(tsp);
      RegisterTensorSlice(ssm.name(), ssm_shape, ssm.type(), fname, ss_slice);
    }
  }
}

void TensorSliceReader::LoadAllShards() const {
  VLOG(1) << "Loading all shards for " << filepattern_;
  for (size_t i = 0; i < fnames_.size() && status_.ok(); ++i) {
    LoadShard(i);
  }
  all_shards_loaded_ = true;
}

const TensorSliceSet* TensorSliceReader::FindTensorSlice(
    const string& name, const TensorSlice& slice,
    std::vector<std::pair<TensorSlice, string>>* details) const {
  const TensorSliceSet* tss = gtl::FindPtrOrNull(tensors_, name);
  if (tss && !tss->QueryMeta(slice, details)) {
    return nullptr;
  }
  return tss;
}

TensorSliceReader::~TensorSliceReader() { gtl::STLDeleteValues(&tensors_); }

void TensorSliceReader::RegisterTensorSlice(const string& name,
                                            const TensorShape& shape,
                                            DataType type, const string& tag,
                                            const TensorSlice& slice) const {
  TensorSliceSet* tss = gtl::FindPtrOrNull(tensors_, name);
  // Create a tensor slice set if needed
  if (!tss) {
    tss = new TensorSliceSet(shape, type);
    tensors_.insert(std::make_pair(name, tss));
  } else {
    // Check if the shapes match
    TensorShape tss_shape(tss->shape());
    if (!shape.IsSameSize(tss_shape)) {
      status_ =
          errors::Internal("Incompatible tensor shapes detected for tensor ",
                           name, ": existing = ", tss_shape.DebugString(),
                           ", new = ", shape.DebugString());
      return;
    }
    if (type != tss->type()) {
      status_ =
          errors::Internal("Incompatible tensor types detected for tensor ",
                           name, ": existing = ", DataTypeString(tss->type()),
                           ", new = ", DataTypeString(type));
      return;
    }
  }
  // Register the tensor slices without the actual data.
  Status s = tss->Register(slice, tag, nullptr);
  if (!s.ok()) {
    status_ = s;
  }
}

bool TensorSliceReader::HasTensor(const string& name, TensorShape* shape,
                                  DataType* type) const {
  mutex_lock l(mu_);
  const TensorSliceSet* tss = gtl::FindPtrOrNull(tensors_, name);
  if (!tss && !all_shards_loaded_) {
    VLOG(1) << "Did not find tensor in preferred shard, loading all shards: "
            << name;
    LoadAllShards();
    tss = gtl::FindPtrOrNull(tensors_, name);
  }
  if (tss) {
    if (shape) {
      *shape = tss->shape();
    }
    if (type) {
      *type = tss->type();
    }
    return true;
  } else {
    return false;
  }
}

Status TensorSliceReader::GetTensor(
    const string& name, std::unique_ptr<tensorflow::Tensor>* out_tensor) const {
  DataType type;
  TensorShape shape;
  TensorSlice slice;
  {
    mutex_lock l(mu_);
    const TensorSliceSet* tss = gtl::FindPtrOrNull(tensors_, name);
    if (tss == nullptr) {
      return errors::NotFound(name, " not found in checkpoint file");
    }

    if (tss->Slices().size() > 1) {
      // TODO(sherrym): Support multi-slice checkpoints.
      return errors::Unimplemented("Sliced checkpoints are not supported");
    }

    type = tss->type();
    shape = tss->shape();
    slice = tss->Slices().begin()->second.slice;
  }

  std::unique_ptr<tensorflow::Tensor> t(new tensorflow::Tensor(type, shape));
  bool success = false;

#define READER_COPY(dt)                                                  \
  case dt:                                                               \
    success = CopySliceData(name, slice,                                 \
                            t->flat<EnumToDataType<dt>::Type>().data()); \
    break;

  switch (type) {
    READER_COPY(DT_FLOAT);
    READER_COPY(DT_DOUBLE);
    READER_COPY(DT_INT32);
    READER_COPY(DT_UINT8);
    READER_COPY(DT_INT16);
    READER_COPY(DT_INT8);
    READER_COPY(DT_INT64);
    default:
      return errors::Unimplemented("Data type not supported");
  }

  if (!success) {
    return errors::NotFound(name, " not found in checkpoint file");
  }
  std::swap(*out_tensor, t);

  return Status::OK();
}

TensorSliceReader::VarToShapeMap TensorSliceReader::GetVariableToShapeMap()
    const {
  VarToShapeMap name_to_shape;
  if (status().ok()) {
    for (auto e : Tensors()) {
      name_to_shape[e.first] = e.second->shape();
    }
  }
  return name_to_shape;
}

const string TensorSliceReader::DebugString() const {
  string shape_str;
  if (status().ok()) {
    for (auto e : Tensors()) {
      strings::StrAppend(&shape_str, e.first, " (",
                         EnumName_DataType(e.second->type()), ") ",
                         e.second->shape().DebugString(), "\n");
    }
  }
  return shape_str;
}

}  // namespace checkpoint

}  // namespace tensorflow
