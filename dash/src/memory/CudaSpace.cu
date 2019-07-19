#include <dash/Exception.h>
#include <dash/internal/Logging.h>
#include <dash/memory/MemorySpaceBase.h>
#include <dash/memory/MemorySpace.h>
#include <dash/memory/CudaSpace.h>
#include <new>
#include <assert.h>

void* dash::CudaSpace::do_allocate(size_t bytes, size_t alignment)
{
  // Cuda guarantees alignment at 256 bytes but not more.
  assert(alignment <= 256);
  void_pointer ptr;
  auto ret = cudaMallocManaged(&ptr, bytes) ;
  if (ret != cudaSuccess) {
    DASH_LOG_ERROR(
        "CudaPace.do_allocate",
        "Cannot allocate managed memory",
        bytes,
        alignment);
    DASH_LOG_ERROR("CudaPace.do_allocate", cudaGetErrorString(ret));

    std::bad_alloc();
  }
  return ptr;
}

void dash::CudaSpace::do_deallocate(void* p, size_t bytes, size_t alignment)
{
  if (cudaFree(p) != cudaSuccess) {
    DASH_LOG_ERROR(
        "CudaPace.do_deallocate",
        "Cannot deallocate managed memory",
        p,
        bytes,
        alignment);
  }
}

bool dash::CudaSpace::do_is_equal(
    std::pmr::memory_resource const& other) const noexcept
{
  const CudaSpace* other_p = dynamic_cast<const CudaSpace*>(&other);

  return nullptr != other_p;
}

template <>
dash::MemorySpace<dash::memory_domain_local, dash::memory_space_cuda_tag>*
dash::get_default_memory_space<dash::memory_domain_local, dash::memory_space_cuda_tag>()
{
  static dash::CudaSpace cuda_space_singleton;
  return &cuda_space_singleton;
}

