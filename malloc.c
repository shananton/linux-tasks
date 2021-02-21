#define _GNU_SOURCE
#include <unistd.h>

#include <stddef.h>
#include <string.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

#define ALIGNMENT alignof(max_align_t)
#define BUF_SIZE (64 * (1 << 20))
#define ALIGNED(addr) ((addr + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT)
#define ADVANCE(ptr, size) ((void*) ALIGNED((uintptr_t) ptr + size))
#define RETREAT(ptr, size) ((void*) (((uintptr_t) ptr - size) / ALIGNMENT * ALIGNMENT))
#define CHECK_BOUNDS(ptr, size, buf) if ((uintptr_t) ptr + size >= (uintptr_t) buf + sizeof(buf)) return NULL;

alignas(max_align_t) static unsigned char buf[BUF_SIZE];
static void* buf_ptr = buf;

static char print_buf[1024];
static char* print_ptr = print_buf;

static void print_reset() {
  print_ptr = print_buf;
}

static void print_str(const char* str) {
  size_t len = strlen(str);
  memcpy(print_ptr, str, len);
  print_ptr += len;
}

static void print_size(size_t n) {
  const size_t base = 10;
  char* begin_ptr = print_ptr;
  if (!n) {
    *print_ptr++ = '0';
  } else {
    for (; n; n /= base) {
      *print_ptr++ = '0' + n % base;
    }
    ptrdiff_t len = print_ptr - begin_ptr;
    for (ptrdiff_t i = 0; i < len / 2; ++i) {
      char t = *(begin_ptr + i);
      *(begin_ptr + i) = *(print_ptr - 1 - i);
      *(print_ptr - 1 - i) = t;
    }
  }
}

static void print_commit(int fd) {
  *print_ptr = 0;
  syscall(SYS_write, fd, print_buf, strlen(print_buf));
  print_reset();
}

void* malloc(size_t size) {
  print_str("Memory requested of size ");
  print_size(size);
  print_str("\n");
  print_commit(STDERR_FILENO);

  if (!size) {
    return NULL;
  }

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
  print_str("Calling realloc with new_size ");
  print_size(new_size);
  print_str("...\n");
  print_commit(STDERR_FILENO);

  if (!ptr) {
    return malloc(new_size);
  }
  size_t old_size = *(size_t*) RETREAT(ptr, sizeof(size_t));

  print_str("Old size was ");
  print_size(old_size);
  print_str("\n");
  print_commit(STDERR_FILENO);

  if (new_size <= old_size) {
    return ptr;
  }
  void* new_mem = malloc(new_size);
  if (!new_mem) {
    return NULL;
  }
  memcpy(new_mem, ptr, old_size);
  return new_mem;
}

