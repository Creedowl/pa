#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

paddr_t page_translate(vaddr_t vaddr, bool is_write) {
  // Log("vaddr: 0x%08x is_write: %d", vaddr, is_write);
  uint32_t index = vaddr >> 22;
  uint32_t page = (vaddr >> 12) & 0x3ff;
  uint32_t offset = vaddr & 0xfff;
  // Log("index: %x page: %x offset: %x", index, page, offset);
  PDE pde;
  uint32_t pde_addr = (cpu.cr3.page_directory_base << 12) + index * 4;
  // Log("pde_addr: 0x%08x", pde_addr);
  pde.val = paddr_read(pde_addr, 4);
  assert(pde.present);
  PTE pte;
  uint32_t pte_addr = (pde.page_frame << 12) + page * 4;
  // Log("pte_addr: 0x%08x", pte_addr);
  pte.val = paddr_read(pte_addr, 4);
  assert(pte.present);
  if(pde.accessed == 0) {
    pde.accessed = 1;
    paddr_write(pde_addr, 4, pde.val);
  }
  if(pte.accessed == 0 || (pte.dirty == 0 && is_write)) {
    pte.accessed = 1;
    pte.dirty = 1;
    paddr_write(pte_addr, 4, pte.val);
  }
  // Log("addr 0x%08x", (pte.page_frame << 12) + offset);
  return (pte.page_frame << 12) + offset;
}

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) return mmio_read(addr, len, mmio_id);
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int mmio_id = is_mmio(addr);
  if (mmio_id != -1) mmio_write(addr, len, data, mmio_id);
  memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if(cpu.cr0.paging) {
    // cross page
    if((addr >> 12) != ((addr + len -1) >> 12)) {
      // one page can contain at most 0x1000 entities
      uint32_t pre_size = 0x1000 - (addr & 0xfff);
      uint32_t pre_addr, next_addr, pre_data, next_data;

      // data in the previous page
      pre_addr = page_translate(addr, false);
      pre_data = paddr_read(pre_addr, pre_size);

      // data in the next page
      next_addr = page_translate(addr + pre_size, false);
      next_data = paddr_read(next_addr, len - pre_size);

      // small endian
      return pre_data | (next_data << (pre_size * 8));
    } else {
      paddr_t paddr = page_translate(addr, false);
      return paddr_read(paddr, len);
    }
  }
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if(cpu.cr0.paging) {
    // cross page
    if((addr >> 12) != ((addr + len -1) >> 12)) {
      // one page can contain at most 0x1000 entities
      uint32_t pre_size = 0x1000 - (addr & 0xfff);
      uint32_t pre_addr, next_addr;

      // data in the previous page
      pre_addr = page_translate(addr, false);
      paddr_write(pre_addr, pre_size, data);

      // data in the next page
      next_addr = page_translate(addr + pre_size, false);
      paddr_write(next_addr, len - pre_size, data >> (32 - pre_size * 8));

      return;
    } else {
      paddr_t paddr = page_translate(addr, true);
      paddr_write(paddr, len, data);
      return;
    }
  }
  paddr_write(addr, len, data);
}
