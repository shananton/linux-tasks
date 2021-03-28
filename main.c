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

int verbosity = 0;

typedef struct {
  blkcnt_t nblocks;
  blkcnt_t nseq;
} file_info_t;

file_info_t process_file(const char* pathname) {
  INFO(1, "%s:\n", pathname);
  int fd;
  ASSIGN_WITH_ERRNO(fd, open(pathname, O_RDONLY), "open(O_RDONLY)");

  struct stat file_stat;
  CALL_WITH_ERRNO(fstat(fd, &file_stat), "fstat()");

  INFO(1, "  Block size: %ld\n", file_stat.st_blksize);

  blkcnt_t total_blocks = (file_stat.st_size + file_stat.st_blksize - 1) / file_stat.st_blksize;
  INFO(1, "  Logical blocks: %ld\n", total_blocks);

  file_info_t file_info;
  file_info.nblocks = file_stat.st_blocks * 512ULL / file_stat.st_blksize;
  INFO(1, "  Physical blocks: %ld\n", file_info.nblocks);

  INFO(2, "  Logical-to-physical block mapping:\n");
  file_info.nseq = 0;
  int last_block = 0;
  for (blkcnt_t i = 0; i < total_blocks; ++i) {
    int n = i;
    CALL_WITH_ERRNO(ioctl(fd, FIBMAP, &n), "ioctl(FIBMAP)");
    INFO(2, "    %5ld -> %10d\n", i, n);
    if ((!last_block && n) || (last_block && n && n != last_block + 1)) {
      ++file_info.nseq;
    }
    last_block = n;
  }
  INFO(1, "  Extents: %ld\n", file_info.nseq);

  close(fd);
  return file_info;
}

int main(int argc, const char* argv[]) {
  int first_file = 1;
  /* int stat_flags = AT_NO_AUTOMOUNT; */

  while (first_file < argc) {
    if (strcmp(argv[first_file], "-P") == 0 || 
        strcmp(argv[first_file], "--no-dereference") == 0) {
      /* stat_flags |= AT_SYMLINK_NOFOLLOW; */
      fprintf(stderr, "--no-dereference is not yet supported\n");
      exit(EXIT_FAILURE);
      /* ++first_file; */
      /* continue; */
    }
    if (strcmp(argv[first_file], "-v") == 0 ||
        strcmp(argv[first_file], "--verbose") == 0) {
      ++verbosity;
      ++first_file;
      continue;
    }
   break;
  }
  if (first_file == argc) {
    fprintf(stderr, "Usage: %s [-P | --no-dereference] [-v | --verbose] <file1> [file2 ...]", argv[0]);
    exit(EXIT_FAILURE);
  }

  double weighted_sum = 0;
  blkcnt_t total_blocks = 0;
  for (int i = first_file; i < argc; ++i) {
    file_info_t info = process_file(argv[i]);
    weighted_sum += (double)info.nblocks / info.nseq;
    total_blocks += info.nblocks;
  }

  printf("filefrag_total = %f (less is more fragmented)\n", weighted_sum / total_blocks);
  return 0;
}
