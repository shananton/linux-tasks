#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#define INFO(level, format, args...) \
  if (verbosity >= level) { \
    printf(format, ## args); \
  }

#define ASSERT_ERRNO(error) \
  if (errno) { \
    perror(error); \
    exit(EXIT_FAILURE); \
  }

#define CALL_WITH_ERRNO(stmt, error) \
  errno = 0; \
  stmt; \
  ASSERT_ERRNO(error)

#define ASSIGN_WITH_ERRNO(var, stmt, error) \
  errno = 0; \
  var = stmt; \
  ASSERT_ERRNO(error)

#define INFO_LEVEL 1
#define VERBOSE_LEVEL 2

int verbosity = 0;

typedef struct {
  double fragmentation;
  blkcnt_t nblocks;
} file_info_t;

file_info_t process_file(const char* pathname) {
  INFO(1, "%s:\n", pathname);
  int fd;
  ASSIGN_WITH_ERRNO(fd, open(pathname, O_RDONLY), "open(O_RDONLY)");

  struct stat file_stat;
  CALL_WITH_ERRNO(fstat(fd, &file_stat), "fstat()");

  int block_size;
  CALL_WITH_ERRNO(ioctl(fd, FIGETBSZ, &block_size), "ioctl(FIGETBSZ)");
  INFO(INFO_LEVEL, "  Block size: %d\n", block_size);

  blkcnt_t logical_blocks = (file_stat.st_size + block_size - 1) / block_size;
  INFO(INFO_LEVEL, "  Logical blocks: %ld\n", logical_blocks);

  blkcnt_t physical_blocks = file_stat.st_blocks * 512ULL / block_size;
  INFO(INFO_LEVEL, "  Physical blocks: %ld\n", physical_blocks);

  INFO(VERBOSE_LEVEL, "  Logical-to-physical block mapping:\n");
  blkcnt_t nseq = 0;
  int last_block = 0;
  for (blkcnt_t i = 0; i < logical_blocks; ++i) {
    int n = i;
    CALL_WITH_ERRNO(ioctl(fd, FIBMAP, &n), "ioctl(FIBMAP)");
    INFO(VERBOSE_LEVEL, "    %5ld -> %10d\n", i, n);
    if (n && (!last_block || n != last_block + 1)) {
      ++nseq;
    }
    last_block = n;
  }
  INFO(INFO_LEVEL, "  Extents: %ld\n", nseq);

  file_info_t file_info = {
    .fragmentation = (nseq ? (double)physical_blocks / logical_blocks / nseq : 0),
    .nblocks = logical_blocks
  };
  INFO(INFO_LEVEL, "  File fragmentation: %f\n", file_info.fragmentation);

  close(fd);
  return file_info;
}

void opt_verbose() {
  ++verbosity;
}

void parse_short_opts(const char* opts) {
  for (const char* c = opts; *c; ++c) {
    switch (*c) {
      case 'v':
        opt_verbose();
        break;
      default:
        fprintf(stderr, "Option -%c is not recognised\n", *c);
        exit(EXIT_FAILURE);
    }
  }
}

int main(int argc, const char* argv[]) {
  int first_file = 1;

  for (; first_file < argc; ++first_file) {
    if (argv[first_file][0] == '-') {
      parse_short_opts(argv[first_file] + 1);
    } else if (strcmp(argv[first_file], "--verbose") == 0) {
      opt_verbose();
    } else {
      if (strcmp(argv[first_file], "--") == 0) ++first_file;
      break;
    }
  }
  if (first_file == argc) {
    fprintf(stderr, "Usage: %s [-v | --verbose] [--] <file1> [file2 ...]", argv[0]);
    exit(EXIT_FAILURE);
  }

  double weighted_sum = 0;
  blkcnt_t total_blocks = 0;
  for (int i = first_file; i < argc; ++i) {
    file_info_t info = process_file(argv[i]);
    weighted_sum += info.fragmentation * info.nblocks;
    total_blocks += info.nblocks;
  }

  printf("filefrag_total = %f (less is more fragmented)\n", weighted_sum / total_blocks);
  return 0;
}
