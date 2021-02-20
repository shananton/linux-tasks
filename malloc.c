#include <stddef.h>
#include <string.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ALIGNMENT alignof(max_align_t)
#define BUF_SIZE (64 * (1 << 20))
#define ALIGNED(addr) ((addr + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT)
#define ADVANCE(ptr, size) ((void*) ALIGNED((uintptr_t) ptr + size))
#define RETREAT(ptr, size) ((void*) (((uintptr_t) ptr - size) / ALIGNMENT * ALIGNMENT))
#define CHECK_BOUNDS(ptr, size, buf) if ((uintptr_t) ptr + size >= (uintptr_t) buf + sizeof(buf)) return NULL;

alignas(max_align_t) static unsigned char buf[BUF_SIZE];
static void* buf_ptr = buf;

void* malloc(size_t size) {
  fprintf(stderr, "Memory requested of size %zu\n", size);

  CHECK_BOUNDS(buf_ptr, sizeof(size_t), buf);
  *(size_t*) buf_ptr = size;
  buf_ptr = ADVANCE(buf_ptr, sizeof(size_t));

  CHECK_BOUNDS(buf_ptr, size, buf);
  void* allocated = buf_ptr;
  buf_ptr = ADVANCE(buf_ptr, size);
  return allocated;
}

void* calloc(size_t num, size_t size) {
  return malloc(num * size);
}

void free(void* ptr) {
}

void* realloc(void* ptr, size_t new_size) {
  fprintf(stderr, "Calling realloc with new_size %zu...\n", new_size);

  if (!ptr) {
    return malloc(new_size);
  }
  size_t* old_size_ptr = RETREAT(ptr, sizeof(size_t));
  fprintf(stderr, "Old size was %zu\n", *old_size_ptr);
  if (new_size <= *old_size_ptr) {
    *old_size_ptr = new_size;
    return ptr;
  }
  void* new_mem = malloc(new_size);
  if (!new_mem) {
    return NULL;
  }
  memcpy(new_mem, ptr, *old_size_ptr);
  return new_mem;
}

