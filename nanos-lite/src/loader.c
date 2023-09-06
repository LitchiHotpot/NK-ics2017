#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

//Do not forget extern
void ramdisk_read(void *, uint32_t, uint32_t);
size_t get_ramdisk_size();
void* new_page(void);

uintptr_t loader(_Protect *as, const char *filename) {
  //TODO();
  int fd = fs_open(filename, 0, 0);
  size_t f_size = fs_filesz(fd);
    void *fz_end = DEFAULT_ENTRY + f_size;

    void *va, *pa;

    for(va = DEFAULT_ENTRY; va < fz_end; va += PGSIZE){

        pa = new_page();

        Log("Map va to pa: 0x%08x to 0x%08x", va, pa);

        _map(as, va, pa);

        fs_read(fd, pa, (fz_end - va) < PGSIZE ? (fz_end - va) : PGSIZE);

    }
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
