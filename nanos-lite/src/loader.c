#include "common.h"
#include "fs.h"
#include "memory.h"

// #define DEFAULT_ENTRY ((void *)0x4000000)
#define DEFAULT_ENTRY ((void *)0x8048000)

// extern void ramdisk_read(void *buf, off_t offset, size_t len);

// extern size_t get_ramdisk_size();

uintptr_t loader(_Protect *as, const char *filename) {
  // ramdisk_read(DEFAULT_ENTRY, 0, get_ramdisk_size());
  int fd = fs_open(filename, 0, 0);
  // fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));
  int size = fs_filesz(fd);
  void *pa;
  void *va = DEFAULT_ENTRY;
  for(int i=0; i<size; i+= PGSIZE) {
    pa = new_page();
    Log("Map va to pa: 0x%08x to 0x%08x", va, pa);
    _map(as, va, pa);
    fs_read(fd, pa, i < PGSIZE ? i : PGSIZE);
    va += PGSIZE;
  }
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
