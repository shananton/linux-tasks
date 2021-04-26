#include <ranges>
#include <concepts>
#include <functional>
#include <type_traits>
#include <atomic>

#include <cstring>
#include <errno.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace transform {

namespace internal {

void wait_for_all_children();

void* map_shared_memory(size_t size);

}

template <typename R>
concept contiguous_input_range =
  std::ranges::input_range<R> && 
  std::ranges::contiguous_range<R> &&
  std::ranges::sized_range<R>;

template <typename T>
concept trivially_placeable =
  std::is_trivially_constructible_v<std::decay_t<T>, T>;

template <typename T>
class mapped_memory_range {
public:
  mapped_memory_range(T* begin, T* end) noexcept : begin_{begin}, end_{end} {}
  ~mapped_memory_range() {
    if (!begin()) return;
    std::destroy(begin(), end());
    munmap(begin(), (end() - begin()) * sizeof(T));
  }
  mapped_memory_range(const mapped_memory_range&) = delete;
  mapped_memory_range& operator=(const mapped_memory_range&) = delete;
  mapped_memory_range(mapped_memory_range&& other) noexcept {
    swap(*this, other);
  }
  mapped_memory_range& operator=(mapped_memory_range&& other) noexcept {
    swap(*this, other);
    return *this;
  }
  friend void swap(mapped_memory_range& lhs, mapped_memory_range& rhs) noexcept {
    using std::swap;
    swap(lhs.begin_, rhs.begin_);
    swap(lhs.end_, rhs.end_);
  }

  T* begin() const noexcept { return begin_; }
  T* end() const noexcept { return end_; }
private:
  T* begin_{};
  T* end_{};
};

template <contiguous_input_range R, std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> F>
  requires trivially_placeable<std::indirect_result_t<F, std::ranges::iterator_t<R>>>
auto transform_static(const R& range, F&& func, size_t nprocesses) ->
  contiguous_input_range auto {
  using result_t = std::decay_t<std::indirect_result_t<F, std::ranges::iterator_t<R>>>;

  const auto range_size = std::ranges::size(range);
  const auto shared_mem = internal::map_shared_memory(sizeof(result_t) * range_size);
  
  auto input_it = std::ranges::begin(range);
  auto output_ptr = static_cast<result_t*>(shared_mem);
  for (size_t proc_idx = 0; proc_idx != nprocesses; ++proc_idx) {
    auto pid = fork();
    if (pid == -1) {
      auto error = errno;
      internal::wait_for_all_children();
      throw std::runtime_error{strerror(error)};
    }
    auto chunk_size = range_size / nprocesses + (proc_idx < range_size % nprocesses);
    if (pid == 0) {
      for (; chunk_size; --chunk_size, ++input_it, ++output_ptr) {
        auto&& element = *input_it;
        std::construct_at(output_ptr, std::invoke(func, std::forward<decltype(element)>(element)));
      }
      std::exit(EXIT_SUCCESS);
    } else {
      std::advance(input_it, chunk_size);
      std::advance(output_ptr, chunk_size);
    }
  }
  internal::wait_for_all_children();

  return mapped_memory_range{static_cast<result_t*>(shared_mem), output_ptr};
}

template <contiguous_input_range R, std::indirectly_regular_unary_invocable<std::ranges::iterator_t<R>> F>
  requires trivially_placeable<std::indirect_result_t<F, std::ranges::iterator_t<R>>>
auto transform_dynamic(const R& range, F&& func, size_t nprocesses, size_t chunk_size)
  -> contiguous_input_range auto {
  using result_t = std::decay_t<std::indirect_result_t<F, std::ranges::iterator_t<R>>>;

  const auto range_size = std::ranges::size(range);
  const auto range_begin = std::ranges::begin(range);
  const auto range_end = std::ranges::end(range);

  const auto total_result_size = sizeof(result_t) * range_size;
  using atomic_index_t = std::atomic<size_t>;
  constexpr auto alignment = alignof(atomic_index_t);
  const auto shared_mem = 
    internal::map_shared_memory((total_result_size + alignment - 1) / alignment * alignment + sizeof(atomic_index_t));
  
  auto output_begin = static_cast<result_t*>(shared_mem);
  auto atomic_idx = 
    new (static_cast<char*>(shared_mem) + (total_result_size + alignment - 1) / alignment * alignment) atomic_index_t{0};

  for (size_t proc_idx = 0; proc_idx != nprocesses; ++proc_idx) {
    auto pid = fork();
    if (pid == -1) {
      auto error = errno;
      internal::wait_for_all_children();
      throw std::runtime_error{strerror(error)};
    }
    if (pid == 0) {
      for (size_t current_idx; (current_idx = atomic_idx->fetch_add(chunk_size)) < range_size; ) {
        auto output_ptr = std::ranges::next(output_begin, current_idx);
        for (auto input_it = std::ranges::next(range_begin, current_idx),
             input_end_it = std::ranges::next(input_it, chunk_size, range_end);
             input_it != input_end_it; ++input_it, ++output_ptr) {
          auto&& element = *input_it;
          std::construct_at(output_ptr, std::invoke(func, std::forward<decltype(element)>(element)));
        }
      }
      std::exit(EXIT_SUCCESS);
    }
  }
  internal::wait_for_all_children();

  return mapped_memory_range{output_begin, std::ranges::next(output_begin, range_size)};
}

}
