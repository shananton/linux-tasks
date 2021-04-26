#include "transform.hpp"

#include <sys/mman.h>

namespace transform::internal {

void wait_for_all_children() {
  while (wait(nullptr) != -1)
    ;
}

void *map_shared_memory(size_t size) {
  auto shared_mem = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (shared_mem == MAP_FAILED) {
    throw std::runtime_error{strerror(errno)};
  }
  return shared_mem;
}

} // namespace transform::internal
