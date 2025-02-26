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

// TODO(opensource): Use a more generic sounding preprocessor name than
// GOOGLE_CUDA
#if GOOGLE_CUDA

#define EIGEN_USE_GPU

#include "tensorflow/core/common_runtime/gpu/gpu_device.h"

#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <vector>

#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#include "tensorflow/core/common_runtime/device_factory.h"
#include "tensorflow/core/common_runtime/gpu/gpu_event_mgr.h"
#include "tensorflow/core/common_runtime/gpu/gpu_init.h"
#include "tensorflow/core/common_runtime/gpu/gpu_stream_util.h"
#include "tensorflow/core/common_runtime/gpu/gpu_util.h"
#include "tensorflow/core/common_runtime/gpu/process_state.h"
#include "tensorflow/core/common_runtime/gpu_device_context.h"
#include "tensorflow/core/common_runtime/local_device.h"
#include "tensorflow/core/framework/allocator.h"
#include "tensorflow/core/framework/device_base.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/graph/types.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/gtl/stl_util.h"
#include "tensorflow/core/lib/strings/numbers.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/strcat.h"
#include "tensorflow/core/platform/cuda.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/macros.h"
#include "tensorflow/core/platform/stream_executor.h"
#include "tensorflow/core/platform/tracing.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session_options.h"
#include "tensorflow/core/util/device_name_utils.h"

namespace gpu = ::perftools::gputools;

namespace tensorflow {

// Eigen Ops directly allocate memory only for temporary buffers used
// during OpKernel::Compute().  The recommended way of allocating such
// memory is via OpKernelContext::allocate_temp().  However, Eigen Ops
// don't have access to OpKernelContext, instead they get access to
// memory directly through the device allocator.  As an Open Source
// project, Eigen assumes allocator semantics similar to those of the
// CUDA memory allocator, and may not work correctly due to race
// conditions if used with some other allocator.  For safety, we need
// to delay deallocation calls out of Eigen until all events on the
// corresponding stream have completed.  The following two classes
// serve this purpose in two different compilation environments.

#if defined(__GCUDACC__) || defined(__GCUDACC_HOST__)
class EigenAllocator : public ::Eigen::Allocator {
 public:
  EigenAllocator() {}

  void Reinitialize(OpKernelContext* context, gpu::Stream* stream,
                    ::tensorflow::Allocator* alloc, EventMgr* em) {
    if (LogMemory::IsEnabled()) {
      operation_ = context->op_kernel().name() + "/EigenAllocator";
      step_id_ = context->step_id();
    }
    stream_ = stream;
    allocator_ = alloc;
    em_ = em;
  }

  void* allocate(size_t num_bytes) const override {
    void* ret = allocator_->AllocateRaw(32 /* alignment */, num_bytes);
    // Eigen doesn't typically check the return pointer from allocate,
    // so we do it here and die with a more helpful error message.
    if (ret == nullptr) {
      LOG(FATAL) << "EigenAllocator for GPU ran out of memory when allocating "
                 << num_bytes << ". See error logs for more detailed info.";
    }
    if (LogMemory::IsEnabled()) {
      LogMemory::RecordRawAllocation(operation_, step_id_, num_bytes, ret,
                                     allocator_);
    }
    return ret;
  }

  void deallocate(void* buffer) const override {
    if (LogMemory::IsEnabled()) {
      LogMemory::RecordRawDeallocation(operation_, step_id_, buffer, allocator_,
                                       true);
    }
    em_->ThenDeleteBuffer(stream_, {allocator_, buffer, operation_, step_id_});
  }

 private:
  string operation_;
  int64 step_id_;
  gpu::Stream* stream_;                 // Not owned.
  ::tensorflow::Allocator* allocator_;  // Not owned.
  ::tensorflow::EventMgr* em_;          // Not owned.

  TF_DISALLOW_COPY_AND_ASSIGN(EigenAllocator);
};

#else
class EigenCudaStreamDevice : public ::Eigen::StreamInterface {
 public:
  EigenCudaStreamDevice() : scratch_(nullptr), semaphore_(nullptr) {
    Eigen::initializeDeviceProp();
  }
  ~EigenCudaStreamDevice() {
  }
  void Reinitialize(OpKernelContext* context, const cudaStream_t* cuda_stream,
                    int gpu_id, ::tensorflow::Allocator* alloc, char* scratch) {
    if (LogMemory::IsEnabled()) {
      operation_ = context->op_kernel().name() + "/EigenAllocator";
      step_id_ = context->step_id();
    }
    scratch_ = scratch;
    semaphore_ =
        reinterpret_cast<unsigned int*>(scratch + Eigen::kCudaScratchSize);
    stream_ = cuda_stream;
    allocator_ = alloc;
    device_prop_ = &Eigen::m_deviceProperties[gpu_id];
  }

  const cudaStream_t& stream() const override { return *stream_; }
  const cudaDeviceProp& deviceProperties() const override {
    return *device_prop_;
  }

  void* allocate(size_t num_bytes) const override {
    void* ret = allocator_->AllocateRaw(32 /* alignment */, num_bytes);
    if (ret == nullptr) {
      LOG(FATAL) << "EigenAllocator for GPU ran out of memory when allocating "
                 << num_bytes << ". See error logs for more detailed info.";
    }
    if (LogMemory::IsEnabled()) {
      LogMemory::RecordRawAllocation(operation_, step_id_, num_bytes, ret,
                                     allocator_);
    }
    return ret;
  }
  void deallocate(void* buffer) const override {
    if (LogMemory::IsEnabled()) {
      LogMemory::RecordRawDeallocation(operation_, step_id_, buffer, allocator_,
                                       true);
    }
    AsyncFreeData* afData =
        new AsyncFreeData(allocator_, buffer, operation_, step_id_);
    cudaError_t err = cudaStreamAddCallback(*stream_, asyncFree, afData, 0);
    CHECK_EQ(err, cudaSuccess);
  }

  // Return a pointer to a per stream scratchpad of 1024 bytes residing
  // in global memory.
  void* scratchpad() const {
    return scratch_;
  }

  // Return a semaphore. The semaphore is initially initialized to 0, and
  // each kernel using it is responsible for resetting to 0 upon completion
  // to maintain the invariant that the semaphore is always equal to 0 upon
  // each kernel start.
  unsigned int* semaphore() const { return semaphore_; }

 private:
  struct AsyncFreeData {
    AsyncFreeData(::tensorflow::Allocator* a, void* p, const string& o,
                  const int64 s)
        : allocator_(a), address_(p), operation_(o), step_id_(s) {}
    ::tensorflow::Allocator* allocator_;
    void* address_;
    const string operation_;
    const int64 step_id_;
  };

  static void CUDART_CB asyncFree(cudaStream_t stream, cudaError_t status,
                                  void* userData) {
    AsyncFreeData* data = static_cast<AsyncFreeData*>(userData);
    if (LogMemory::IsEnabled()) {
      LogMemory::RecordRawDeallocation(data->operation_, data->step_id_,
                                       data->address_, data->allocator_, false);
    }
    data->allocator_->DeallocateRaw(data->address_);
    delete data;
  }

  string operation_;
  int64 step_id_;
  const cudaStream_t* stream_;          // Not owned.
  const cudaDeviceProp* device_prop_;   // Not owned.
  ::tensorflow::Allocator* allocator_;  // Not owned.
  mutable char* scratch_;
  mutable unsigned int* semaphore_;

  TF_DISALLOW_COPY_AND_ASSIGN(EigenCudaStreamDevice);
};

#endif

BaseGPUDevice::BaseGPUDevice(const SessionOptions& options, const string& name,
                             Bytes memory_limit, BusAdjacency bus_adjacency,
                             int gpu_id, const string& physical_device_desc,
                             Allocator* gpu_allocator, Allocator* cpu_allocator,
                             bool sync_every_op, int32 max_streams)
    : LocalDevice(options, Device::BuildDeviceAttributes(
                               name, DEVICE_GPU, memory_limit, bus_adjacency,
                               physical_device_desc),
                  gpu_allocator),
      gpu_allocator_(gpu_allocator),
      cpu_allocator_(cpu_allocator),
      gpu_id_(gpu_id),
      sync_every_op_(sync_every_op) {
  ProcessState::singleton()->EnableGPUDevice();

  executor_ = GPUMachineManager()->ExecutorForDevice(gpu_id_).ValueOrDie();
  if (!executor_) {
    LOG(ERROR) << "Failed to get StreamExecutor for device " << gpu_id_;
    return;
  }
  em_.reset(new EventMgr(executor_, options.config.gpu_options()));

  if (max_streams < 1) {
    LOG(FATAL) << "Invalid value for max_streams.";
  }

  // Create the specified number of GPU streams
  for (int i = 0; i < max_streams; i++) {
    auto stream = new gpu::Stream(executor_);
    stream->Init();
    VLOG(2) << "Created stream[" << i << "] = " << stream;

    auto host_to_device_stream = new gpu::Stream(executor_);
    host_to_device_stream->Init();
    VLOG(2) << "Created host_to_device_stream[" << i
            << "] = " << host_to_device_stream;

    auto device_to_host_stream = new gpu::Stream(executor_);
    device_to_host_stream->Init();
    VLOG(2) << "Created device_to_host_stream[" << i
            << "] = " << device_to_host_stream;

    auto device_to_device_stream = new gpu::Stream(executor_);
    device_to_device_stream->Init();
    VLOG(2) << "Created device_to_device_stream[" << i
            << "] = " << device_to_device_stream;

    streams_.push_back({stream, host_to_device_stream, device_to_host_stream,
                        device_to_device_stream});

    perftools::gputools::DeviceMemory<char> mem =
        executor_->AllocateArray<char>(Eigen::kCudaScratchSize +
                                       sizeof(unsigned int));
    scratch_.push_back(static_cast<char*>(mem.opaque()));
    bool ok = executor_->SynchronousMemZero(
        &mem, Eigen::kCudaScratchSize + sizeof(unsigned int));
    if (!ok) {
      LOG(FATAL) << "Failed to initialize device " << gpu_id;
    }

    device_contexts_.push_back(
        new GPUDeviceContext(i, stream, host_to_device_stream,
                             device_to_host_stream, device_to_device_stream));
  }
  gpu_device_info_ = new GpuDeviceInfo;
  gpu_device_info_->stream = streams_[0].compute;
  gpu_device_info_->default_context = device_contexts_[0];
  gpu_device_info_->event_mgr = em_.get();
  set_tensorflow_gpu_device_info(gpu_device_info_);
}

BaseGPUDevice::~BaseGPUDevice() {
  delete gpu_device_info_;
  for (auto ctx : device_contexts_) ctx->Unref();
  for (auto& stream_group : streams_) {
    delete stream_group.compute;
    delete stream_group.host_to_device;
    delete stream_group.device_to_host;
    delete stream_group.device_to_device;
  }
}

bool BaseGPUDevice::RequiresRecordingAccessedTensors() const {
  // When there is no more than one stream, we release the tensor reference
  // at the end of the kernel launch, instead of at the end of the kernel
  // execution.
  return streams_.size() > 1;
}

Status BaseGPUDevice::FillContextMap(const Graph* graph,
                                     DeviceContextMap* device_context_map) {
  VLOG(2) << "FillContextMap";

  const auto num_streams = streams_.size();
  // Special case for single stream.
  if (num_streams == 1) {
    return Status::OK();
  }
  const int64 before = Env::Default()->NowMicros();
  gpu_stream_util::AssignStreamsOpts opts;
  opts.max_streams = num_streams;
  std::unordered_map<int, int> node_to_stream_id;
  TF_RETURN_IF_ERROR(
      gpu_stream_util::AssignStreams(graph, opts, &node_to_stream_id));
  int64 elapsed = Env::Default()->NowMicros() - before;
  VLOG(3) << "AssignStreams took " << elapsed << "us";

  // Fill in the context map.  It is OK for this map to contain
  // duplicate DeviceContexts so long as we increment the refcount.
  device_context_map->resize(graph->num_node_ids());
  for (Node* n : graph->nodes()) {
    auto mapped_stream = node_to_stream_id[n->id()];
    CHECK_LE(mapped_stream, num_streams);
    auto ctx = device_contexts_[mapped_stream];
    VLOG(3) << "Assigned stream " << node_to_stream_id[n->id()]
            << " ==> stream[" << ctx->stream_id() << "] for node id " << n->id()
            << " " << n->type_string() << " " << n->name();
    ctx->Ref();
    (*device_context_map)[n->id()] = ctx;
  }

  return Status::OK();
}

void BaseGPUDevice::Compute(OpKernel* op_kernel, OpKernelContext* context) {
  // ScopedActivity is cheap when tracing is not active, but we
  // can avoid computing the Hash64.
  // TODO(pbar) This would no longer be needed if Ops have a unique id.
  const uint64 id = port::Tracing::IsActive() ? Hash64(op_kernel->name()) : 0;
  port::Tracing::ScopedActivity region(port::Tracing::EventCategory::kCompute,
                                       id);

  GPUDeviceContext* gpu_device_context = device_contexts_[0];
  if (context->op_device_context() != nullptr) {
    gpu_device_context =
        static_cast<GPUDeviceContext*>(context->op_device_context());
  }
  gpu::Stream* stream = gpu_device_context->stream();
  const auto stream_id = gpu_device_context->stream_id();

  const bool vlog_1 = VLOG_IS_ON(1);
  const bool vlog_2 = vlog_1 && VLOG_IS_ON(2);

  if (vlog_1) {
    VLOG(1) << "GpuDevice::Compute " << op_kernel->name() << " op "
            << op_kernel->def().op() << " on GPU" << gpu_id_ << " stream["
            << stream_id << "]";
  }

  // NOTE(tucker): We need to discriminate between Eigen GPU
  // operations and all others.  If an operation is Eigen
  // implemented (or otherwise tries to launch a cuda kernel
  // directly), we need to establish a stacked-scoped environment
  // that directs it to execute on the proper device.  Otherwise we
  // expect the Op to use StreamExecutor directly and correctly.  The
  // way we make this discrimination is quite hacky: At the moment
  // the only non-Eigen GPU Op is the recv-op, which is known to be
  // asynchronous.
  if (op_kernel->is_internal() && op_kernel->type_string() == "_Recv") {
    context->SetStatus(errors::Internal(
        "Invalid synchronous 'Compute' on GPU for '_Recv' op"));
  } else {
    port::Tracing::ScopedAnnotation annotation(op_kernel->name(),
                                               op_kernel->type_string());

    const auto num_streams = streams_.size();
    if (num_streams > 1) {
      // If this op's device context is different from the other contexts,
      // we must wait on the stream.
      for (int i = 0; i < context->num_inputs(); ++i) {
        const GPUDeviceContext* idc =
            static_cast<GPUDeviceContext*>(context->input_device_context(i));
        OP_REQUIRES(context, idc != nullptr,
                    errors::Internal("Input device context ", i,
                                     " was not set properly."));
        if (vlog_2) {
          const void* base;
          size_t len;
          if (context->has_input(i)) {
            if (IsRefType(context->input_dtype(i))) {
              Tensor tensor = context->mutable_input(i, false);
              base = DMAHelper::base(&tensor);
              len = tensor.TotalBytes();
            } else {
              const Tensor& tensor = context->input(i);
              base = DMAHelper::base(&tensor);
              len = tensor.TotalBytes();
            }
            VLOG(2) << "Input " << i << " " << base << "  " << len;
            VLOG(2) << "  stream[" << stream_id << "].ThenWaitFor(stream["
                    << idc->stream_id() << "])"
                    << ((idc->stream() == stream) ? " not needed" : "");
          }
        }
        if (idc->stream() != stream) stream->ThenWaitFor(idc->stream());
      }
    }
    gpu::cuda::ScopedActivateExecutorContext scoped_activation{
        stream->parent()};
    op_kernel->Compute(context);
    if (context->status().ok()) {
      if (sync_every_op_) {
        // Note: GPUUtil::Sync() only syncs the default stream.
        // We need to either sync the stream used by this op, or
        // all streams.  Given that this flag is typically used for
        // debugging it makes more sense to sync all GPU activity.
        context->SetStatus(GPUUtil::SyncAll(this));
      }
    }
  }
}

void BaseGPUDevice::ConsumeListOfAccessedTensors(
    DeviceContext* device_context, const TensorReferenceVector& tensor_refs) {
  GPUDeviceContext* gpu_device_context = device_contexts_[0];
  if (device_context != nullptr) {
    gpu_device_context = static_cast<GPUDeviceContext*>(device_context);
  }
  gpu::Stream* stream = gpu_device_context->stream();
  em_->ThenDeleteTensors(stream, tensor_refs);
}

// Based on the semantics of Device::Sync this call should wait for
// all streams not just the current one.
Status BaseGPUDevice::Sync() { return GPUUtil::SyncAll(this); }

void BaseGPUDevice::ComputeAsync(AsyncOpKernel* op_kernel,
                                 OpKernelContext* context,
                                 AsyncOpKernel::DoneCallback done) {
  GPUDeviceContext* gpu_device_context = device_contexts_[0];
  if (context->op_device_context() != nullptr) {
    gpu_device_context =
        static_cast<GPUDeviceContext*>(context->op_device_context());
  }
  const auto stream_id = gpu_device_context->stream_id();

  VLOG(1) << "GpuDevice::ComputeAsync " << op_kernel->name() << " op "
          << op_kernel->def().op() << " on GPU" << gpu_id_ << " stream["
          << stream_id << "]";

  port::Tracing::TraceMe activity(
      strings::StrCat(op_kernel->name(), ":", op_kernel->type_string()));
  op_kernel->ComputeAsync(context, done);
}

Status BaseGPUDevice::MakeTensorFromProto(const TensorProto& tensor_proto,
                                          const AllocatorAttributes alloc_attrs,
                                          Tensor* tensor) {
  AllocatorAttributes attr;
  attr.set_on_host(true);
  attr.set_gpu_compatible(true);
  Allocator* host_alloc = GetAllocator(attr);
  Tensor parsed(tensor_proto.dtype());
  if (!parsed.FromProto(host_alloc, tensor_proto)) {
    return errors::InvalidArgument("Cannot parse tensor from proto: ",
                                   tensor_proto.DebugString());
  }
  Status status;
  if (alloc_attrs.on_host()) {
    *tensor = parsed;
  } else {
    if (!DMAHelper::CanUseDMA(&parsed)) {
      return errors::Internal("GPU copy from non-DMA ",
                              DataTypeString(parsed.dtype()), " tensor");
    }
    Tensor copy(GetAllocator(alloc_attrs), parsed.dtype(), parsed.shape());
    port::Tracing::ScopedAnnotation annotation("MakeTensorFromProto");
    Notification n;
    device_contexts_[0]->CopyCPUTensorToDevice(&parsed, this, &copy,
                                               [&n, &status](const Status& s) {
                                                 status = s;
                                                 n.Notify();
                                               });
    n.WaitForNotification();
    *tensor = copy;
  }
  return status;
}

namespace {
#if defined(__GCUDACC__) || defined(__GCUDACC_HOST__)
class ConcretePerOpGpuDevice : public PerOpGpuDevice {
 public:
  ConcretePerOpGpuDevice() : device_(nullptr) {}
  void Reinitialize(OpKernelContext* context, gpu::Stream* stream,
                    Allocator* base_allocator, ::tensorflow::EventMgr* em,
                    char* scratch) {
    allocator_.Reinitialize(context, stream, base_allocator, em);
    device_.Reinitialize(stream, &allocator_, scratch);
  }

  const Eigen::GpuDevice& device() const override { return device_; }

 private:
  EigenAllocator allocator_;
  Eigen::GpuDevice device_;
};
#else
class ConcretePerOpGpuDevice : public PerOpGpuDevice {
 public:
  ConcretePerOpGpuDevice() : device_(&stream_device_) {}

  void Reinitialize(OpKernelContext* context, const cudaStream_t* cuda_stream,
                    int gpu_id, Allocator* base_allocator, char* scratch) {
    stream_device_.Reinitialize(context, cuda_stream, gpu_id, base_allocator,
                                scratch);
  }

  const Eigen::GpuDevice& device() const override { return device_; }

 private:
  EigenCudaStreamDevice stream_device_;
  Eigen::GpuDevice device_;
};
#endif
}  // namespace

void BaseGPUDevice::ReinitializeDevice(OpKernelContext* context,
                                       PerOpGpuDevice* device, int stream_id,
                                       Allocator* allocator) {
  ConcretePerOpGpuDevice* concrete_device =
      static_cast<ConcretePerOpGpuDevice*>(device);
  DCHECK(concrete_device);
#if defined(__GCUDACC__) || defined(__GCUDACC_HOST__)
  concrete_device->Reinitialize(context, streams_[stream_id].compute, allocator,
                                em_.get(), scratch_[stream_id]);
#else
  const cudaStream_t* cuda_stream = reinterpret_cast<const cudaStream_t*>(
      streams_[stream_id].compute->implementation()->CudaStreamMemberHack());
  concrete_device->Reinitialize(context, cuda_stream, gpu_id_, allocator,
                                scratch_[stream_id]);
#endif
}

PerOpGpuDevice* BaseGPUDevice::MakeGpuDevice() {
  return new ConcretePerOpGpuDevice();
}

void BaseGPUDevice::ReinitializeGpuDevice(OpKernelContext* context,
                                          PerOpGpuDevice* device,
                                          DeviceContext* dc,
                                          Allocator* allocator) {
  if (dc) {
    const GPUDeviceContext* gpu_dc = static_cast<GPUDeviceContext*>(dc);
    const int stream_id = gpu_dc->stream_id();
    VLOG(1) << "  eigen_gpu_device(" << dc << ") => stream[" << stream_id
            << "]";
    CHECK_LT(stream_id, streams_.size());
    ReinitializeDevice(context, device, stream_id, allocator);
  } else {
    ReinitializeDevice(context, device, 0, allocator);
  }
}

void BaseGPUDeviceFactory::CreateDevices(const SessionOptions& options,
                                         const string& name_prefix,
                                         std::vector<Device*>* devices) {
  int n = INT_MAX;
  auto iter = options.config.device_count().find("GPU");
  if (iter != options.config.device_count().end()) {
    n = iter->second;
  }
  std::vector<int> valid_gpu_ids;
  GetValidDeviceIds(&valid_gpu_ids);
  if (static_cast<size_t>(n) > valid_gpu_ids.size()) {
    n = valid_gpu_ids.size();
  }
  for (int i = 0; i < n; i++) {
    devices->push_back(CreateGPUDevice(
        options, strings::StrCat(name_prefix, "/gpu:", i), valid_gpu_ids[i]));
  }
}

namespace {
int64 MinSystemMemory(int64 available_memory) {
  // We use the following heuristic for now:
  //
  // If the available_memory is < 2GiB, we allocate 200MiB to system memory.
  // Otherwise, allocate 300MiB to system memory.
  //
  // In the future we could be more sophisticated by using a table of
  // devices.
  if (available_memory < (1LL << 31)) {
    // 200MiB
    return 209715200LL;
  } else {
    // max(300 MiB, 0.95 * available_memory)
    return std::max(314572800LL, static_cast<int64>(available_memory * 0.05));
  }
}
}  // namespace

static string GetShortDeviceDescription(int device_id,
                                        const gpu::DeviceDescription& desc) {
  return strings::StrCat("device: ", device_id, ", name: ", desc.name(),
                         ", pci bus id: ", desc.pci_bus_id());
}

LocalDevice* BaseGPUDeviceFactory::CreateGPUDevice(
    const SessionOptions& options, const string& name, int gpu_id) {
  CHECK_GE(gpu_id, 0);

  // Look up the device, to see its attributes.
  gpu::Platform* gpu_platform = GPUMachineManager();
  CHECK_LT(gpu_id, gpu_platform->VisibleDeviceCount());
  gpu::StreamExecutor* se =
      gpu_platform->ExecutorForDevice(gpu_id).ValueOrDie();
  const gpu::DeviceDescription& desc = se->GetDeviceDescription();
  int numa_node = desc.numa_node();
  if (numa_node < 0) {
    // For some reason the StreamExecutor couldn't get the NUMA
    // affinity of the GPU.  If this is not a multi-socket mobo with
    // GPUs local to different buses, it doesn't matter.  If it is, we
    // may run into trouble later with data transfer operations.  The
    // trouble may manifest as slower than expected performance, or
    // outright failures.
    LOG(ERROR) << "Could not identify NUMA node of " << name
               << ", defaulting to 0.  Your kernel may not have been built "
                  "with NUMA support.";
    numa_node = 0;
  }

  int64 total_memory, available_memory;
  CHECK(se->DeviceMemoryUsage(&available_memory, &total_memory));

  int64 allocated_memory;
  double config_memory_fraction =
      options.config.gpu_options().per_process_gpu_memory_fraction();
  if (config_memory_fraction == 0) {
    allocated_memory = available_memory;
    const int64 min_system_memory = MinSystemMemory(available_memory);
    if (min_system_memory < allocated_memory) {
      allocated_memory -= min_system_memory;
    }
  } else {
    allocated_memory = total_memory * config_memory_fraction;
  }

  Bytes allocated_bytes = static_cast<Bytes>(allocated_memory);

  // Get GPU BusAdjacency from its reported NUMA affinity.
  // Because GPUs are virtualized in some environments, we can't just
  // use the GPU id.
  BusAdjacency bus_adjacency = BUS_ANY;
  switch (numa_node) {
    case 0:
      bus_adjacency = BUS_0;
      break;
    case 1:
      bus_adjacency = BUS_1;
      break;
    default:
      bus_adjacency = BUS_ANY;
  }
  VLOG(1) << "GPUDevice id " << gpu_id << " on bus " << bus_adjacency
          << " numa: " << numa_node << " pci: " << desc.pci_bus_id();

  ProcessState* process_state = ProcessState::singleton();
  return CreateGPUDevice(
      options, name, allocated_bytes, bus_adjacency, gpu_id,
      GetShortDeviceDescription(gpu_id, desc),
      process_state->GetGPUAllocator(options.config.gpu_options(), gpu_id,
                                     allocated_memory),
      process_state->GetCPUAllocator(numa_node));
}

static int GetDefaultMinGPUMultiprocessorCount(gpu::Platform* gpu_manager) {
  static const int kDefaultMinGPUMultiprocessorCount = 8;

  // Find the highest multi-processor count across all visible GPUs.
  int max_count = -1;
  for (int i = 0; i < gpu_manager->VisibleDeviceCount(); ++i) {
    auto exec_status = gpu_manager->ExecutorForDevice(i);
    if (!exec_status.ok()) {
      continue;
    }

    gpu::StreamExecutor* se = exec_status.ValueOrDie();
    const gpu::DeviceDescription& desc = se->GetDeviceDescription();
    max_count = std::max(max_count, desc.core_count());
  }

  if (max_count < 0 || kDefaultMinGPUMultiprocessorCount < max_count) {
    return kDefaultMinGPUMultiprocessorCount;
  } else {
    return max_count;
  }
}

static int GetMinGPUMultiprocessorCount(gpu::Platform* gpu_manager) {
  const char* tf_min_gpu_core_count = getenv("TF_MIN_GPU_MULTIPROCESSOR_COUNT");

  if (tf_min_gpu_core_count == nullptr ||
      strcmp(tf_min_gpu_core_count, "") == 0) {
    return GetDefaultMinGPUMultiprocessorCount(gpu_manager);
  }

  int min_gpu_core_count = -1;
  if (strings::safe_strto32(tf_min_gpu_core_count, &min_gpu_core_count)) {
    if (min_gpu_core_count >= 0) {
      return min_gpu_core_count;
    }
  }

  int count = GetDefaultMinGPUMultiprocessorCount(gpu_manager);
  LOG(ERROR) << "Invalid minimum GPU multiprocessor count: ["
             << tf_min_gpu_core_count << "]. "
             << "Using the default value: " << count;
  return count;
}

namespace {

struct CudaVersion {
  // Initialize from version_name in the form of "3.5"
  explicit CudaVersion(const std::string& version_name) {
    size_t dot_pos = version_name.find('.');
    CHECK(dot_pos != string::npos) << "Illegal version name: [" << version_name
                                   << "]";
    string major_str = version_name.substr(0, dot_pos);
    CHECK(strings::safe_strto32(major_str, &major_part))
        << "Illegal version name: [" << version_name << "]";
    string minor_str = version_name.substr(dot_pos + 1);
    CHECK(strings::safe_strto32(minor_str, &minor_part))
        << "Illegal version name: [" << version_name << "]";
  }
  CudaVersion() {}
  bool operator<(const CudaVersion& other) const {
    if (this->major_part != other.major_part) {
      return this->major_part < other.major_part;
    }
    return this->minor_part < other.minor_part;
  }
  friend std::ostream& operator<<(std::ostream& os,
                                  const CudaVersion& version) {
    os << version.major_part << "." << version.minor_part;
    return os;
  }
  int major_part = -1;
  int minor_part = -1;
};

std::vector<CudaVersion> supported_cuda_compute_capabilities = {
  TF_CUDA_CAPABILITIES,
};

std::vector<CudaVersion> GetSupportedCudaComputeCapabilities() {
  auto cuda_caps = supported_cuda_compute_capabilities;
#ifdef TF_EXTRA_CUDA_CAPABILITIES
// TF_EXTRA_CUDA_CAPABILITIES should be defined a sequence separated by commas,
// for example:
//   TF_EXTRA_CUDA_CAPABILITIES=3.0,4.0,5.0
// Use two-level macro expansion for stringification.
#define TF_XSTRING(...) #__VA_ARGS__
#define TF_STRING(s) TF_XSTRING(s)
  string extra_cuda_caps = TF_STRING(TF_EXTRA_CUDA_CAPABILITIES);
#undef TF_STRING
#undef TF_XSTRING
  auto extra_capabilities = str_util::Split(extra_cuda_caps, ',');
  for (const auto& capability : extra_capabilities) {
    cuda_caps.push_back(CudaVersion(capability));
  }
#endif
  return cuda_caps;
}

}  // namespace

void BaseGPUDeviceFactory::GetValidDeviceIds(std::vector<int>* ids) {
  auto gpu_manager = GPUMachineManager();
  if (gpu_manager) {
    auto cuda_supported_capabilities = GetSupportedCudaComputeCapabilities();
    CHECK(!cuda_supported_capabilities.empty());
    CudaVersion min_supported_capability = *std::min_element(
        cuda_supported_capabilities.begin(), cuda_supported_capabilities.end());

    int min_gpu_core_count = GetMinGPUMultiprocessorCount(gpu_manager);

    for (int i = 0; i < gpu_manager->VisibleDeviceCount(); ++i) {
      auto exec_status = gpu_manager->ExecutorForDevice(i);
      if (!exec_status.ok()) {
        continue;
      }
      gpu::StreamExecutor* se = exec_status.ValueOrDie();
      const gpu::DeviceDescription& desc = se->GetDeviceDescription();
      CudaVersion device_capability;
      if (!desc.cuda_compute_capability(&device_capability.major_part,
                                        &device_capability.minor_part)) {
        continue;
      }
      // Only GPUs with no less than the minimum supported compute capability is
      // accepted.
      if (device_capability < min_supported_capability) {
        LOG(INFO) << "Ignoring gpu device "
                  << "(" << GetShortDeviceDescription(i, desc) << ") "
                  << "with Cuda compute capability " << device_capability
                  << ". The minimum required Cuda capability is "
                  << min_supported_capability << ".";
        continue;
      }

      // Filter out slow GPUs. By default, GPUs with a lower multiprocessor
      // count than the fastest GPU are filtered out, unless they have 8 or more
      // multiprocessors. If the TF_MIN_GPU_MULTIPROCESSOR_COUNT environment
      // variable is set, its value will be used to filter out GPUs.
      if (desc.core_count() < min_gpu_core_count) {
        LOG(INFO) << "Ignoring gpu device "
                  << "(" << GetShortDeviceDescription(i, desc) << ") "
                  << "with Cuda multiprocessor count: " << desc.core_count()
                  << ". The minimum required count is " << min_gpu_core_count
                  << ". You can adjust this requirement with the env var "
                     "TF_MIN_GPU_MULTIPROCESSOR_COUNT.";
        continue;
      }

      int new_id = ids->size();
      ids->push_back(i);

      LOG(INFO) << "Creating TensorFlow device (/gpu:" << new_id << ") -> "
                << "(" << GetShortDeviceDescription(i, desc) << ")";
    }
  }
}

}  // namespace tensorflow

#endif  // GOOGLE_CUDA
