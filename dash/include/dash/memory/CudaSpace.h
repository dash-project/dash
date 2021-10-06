#ifndef DASH__MEMORY__CUDA_SPACE_H__INCLUDED
#define DASH__MEMORY__CUDA_SPACE_H__INCLUDED


#include <dash/memory/MemorySpaceBase.h>

// This memory space can only be used with nvcc
#ifdef __CUDACC__
namespace dash {

class CudaSpace
  : public dash::MemorySpace<memory_domain_local, memory_space_cuda_tag> {

public:
  using void_pointer       = void*;
  using const_void_pointer = const void*;

public:
  CudaSpace()                      = default;
  CudaSpace(CudaSpace const& other) = default;
  CudaSpace(CudaSpace&& other)      = default;
  CudaSpace& operator=(CudaSpace const& other) = default;
  CudaSpace& operator=(CudaSpace&& other) = default;
  ~CudaSpace()
  {
  }

protected:
  void* do_allocate(size_t bytes, size_t alignment) override;
  void  do_deallocate(void* p, size_t bytes, size_t alignment) override;
  bool  do_is_equal(std::pmr::memory_resource const& other) const
      noexcept override;
};

}  // namespace dash

#endif

#endif
