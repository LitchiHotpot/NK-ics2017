#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) {
    return mmio_read(addr, len, mmio_id);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int mmio_id = is_mmio(addr);

  if (mmio_id != -1) {

    mmio_write(addr, len, data, mmio_id);

  }

  else {

    memcpy(guest_to_host(addr), &data, len);

  }
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  paddr_t paddr;
  if(cpu.cr0.paging) {
      if ((addr & 0xfff) + len > 0x1000) {
	union {
	  uint8_t bytes[4];
	  uint32_t dword;
	} data = {0};
	for (int i = 0; i < len; i++) {
	  paddr = page_translate(addr + i, false);
	  data.bytes[i] = (uint8_t)paddr_read(paddr, 1);
	}
	return data.dword;
      }
      else {
	paddr_t paddr = page_translate(addr, false);
	return paddr_read(paddr, len);
      }
  }
  else
    return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
    paddr_t paddr;
    if(cpu.cr0.paging) {
        if ((addr & 0xfff) + len > 0x1000)
		    for (int i = 0; i < len; i++) {
			  paddr = page_translate(addr + i, true);
			  paddr_write(paddr, 1, data);
			  data >>= 8;
			}
        else {
            paddr_t paddr = page_translate(addr, true);
            return paddr_write(paddr, len, data);
        }
    }
    else
        paddr_write(addr, len, data);
}

paddr_t page_translate(vaddr_t vaddr, bool flag) {
 if (!cpu.cr0.paging) return vaddr;
  paddr_t dir = (vaddr >> 22) & 0x3ff;
  paddr_t page = (vaddr >> 12) & 0x3ff;
  paddr_t offset = vaddr & 0xfff;
  paddr_t PDT_base = cpu.cr3.page_directory_base;
  //Log("page_translate: dir: 0x%x page: 0x%x offset: 0x%x PDT_base: 0x%x\n", dir, page, offset, PDT_base);
  PDE pde;
  pde.val = paddr_read((PDT_base << 12) + (dir << 2), 4);
  if (!pde.present) {
    Log("page_translate: addr: 0x%x\n", vaddr);
    Log("page_translate: dir: 0x%x page: 0x%x offset: 0x%x PDT_base: 0x%x\n", dir, page, offset, PDT_base);
    assert(pde.present);
  }
  PTE pte;
  // Log("page_translate: page_frame: 0x%x\n", pde.page_frame);
  pte.val = paddr_read((pde.page_frame << 12) + (page << 2), 4);
  if (!pte.present) {
    Log("page_translate: addr: 0x%x\n", vaddr);
   assert(pte.present);
  }
  paddr_t paddr = (pte.page_frame << 12) | offset;
  //Log("page_translate: paddr: 0x%x\n", paddr);
  return paddr;
/*
    PDE page_dir_item;
    PTE page_table_item;
    paddr_t page_dir_item_addr = (cpu.cr3.page_directory_base << 12) + ((vaddr >> 22) & 0x3ff) * 4;
    page_dir_item.val = paddr_read(page_dir_item_addr, 4);
    assert(page_dir_item.present);
    page_dir_item.accessed = 1;
    paddr_write(page_dir_item_addr, 4, page_dir_item.val);
    paddr_t page_table_item_addr = (page_dir_item.page_frame << 12) + ((vaddr >> 12) & 0x3ff) * 4;
    page_table_item.val = paddr_read(page_table_item_addr, 4);
    assert(page_table_item.present);
    page_table_item.accessed = 1;
    if (flag) page_table_item.dirty = 1;
    paddr_write(page_table_item_addr, 4, page_table_item.val);
    paddr_t paddr = (page_table_item.page_frame << 12) + (vaddr & 0xfff);
    //Log("vaddr: %#10x, paddr: %#10x", vaddr, paddr);
    return paddr;
*/
}
