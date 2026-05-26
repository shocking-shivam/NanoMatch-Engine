#pragma once
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace lob {

// Fixed-size slab pool; O(1) alloc/free after first slab.
template<class T, std::size_t SLAB_BYTES = (1u<<20)> // 1 MiB slabs by default
class SlabPool {
  std::vector<std::unique_ptr<char[]>> slabs_;
  void* free_ = nullptr;
  std::size_t cap_ = 0, in_use_ = 0;

  void add_slab() {
    const std::size_t n = SLAB_BYTES / sizeof(T);
    auto slab = std::make_unique<char[]>(n * sizeof(T));
    T* base = reinterpret_cast<T*>(slab.get());
    const std::size_t offset = sizeof(T) - sizeof(void*);
    for (std::size_t i = 0; i < n; ++i) {
      void* block = base + i;
      *reinterpret_cast<void**>(reinterpret_cast<char*>(block) + offset) = free_;
      free_ = block;
    }
    cap_ += n;
    slabs_.push_back(std::move(slab));
  }

public:
  explicit SlabPool(std::size_t initial_slabs = 1) {
    for (std::size_t i = 0; i < initial_slabs; ++i) add_slab();
  }

  T* alloc() {
    if (!free_) add_slab();          // cold path only
    void* block = free_;
    const std::size_t offset = sizeof(T) - sizeof(void*);
    free_ = *reinterpret_cast<void**>(reinterpret_cast<char*>(block) + offset);
    ++in_use_;
    return reinterpret_cast<T*>(block);
  }

  void destroy(T* ptr) {
    ptr->~T();
    std::memset(ptr, 0, sizeof(T));
    const std::size_t offset = sizeof(T) - sizeof(void*);
    *reinterpret_cast<void**>(reinterpret_cast<char*>(ptr) + offset) = free_;
    free_ = ptr;
    --in_use_;
  }

  void free(T* ptr) { destroy(ptr); }
  std::size_t in_use()  const { return in_use_; }
  std::size_t capacity() const { return cap_;   }
};

// STL-style adapter for containers that push one node at a time.
template<class T>
struct PoolAllocator {
  using value_type = T;
  SlabPool<T>* pool{};
  PoolAllocator() = default;
  explicit PoolAllocator(SlabPool<T>* p): pool(p) {}
  template<class U> PoolAllocator(const PoolAllocator<U>& o): pool((SlabPool<T>*)o.pool) {}

  T* allocate(std::size_t n) {
    if (n == 1) return pool->alloc();
    return static_cast<T*>(::operator new(n*sizeof(T))); // not hot path
  }
  void deallocate(T* p, std::size_t n) noexcept {
    if (n == 1) return pool->free(p);
    ::operator delete(p);
  }
  template<class U> struct rebind { using other = PoolAllocator<U>; };
  bool operator==(const PoolAllocator& rhs) const { return pool==rhs.pool; }
  bool operator!=(const PoolAllocator& rhs) const { return pool!=rhs.pool; }
};

} // namespace lob
