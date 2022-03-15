#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t SYSTEM_PAGE_SIZE = 0;

void mm_init() { SYSTEM_PAGE_SIZE = getpagesize(); }

static void *mm_get_new_vm_page_from_kernel(int units) {
  void *vm_page =
      mmap(0, units * SYSTEM_PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,
           MAP_ANON | MAP_PRIVATE, 0, 0);
  if (vm_page == MAP_FAILED) {
    printf("Error: VM page allocation failed\n");
    return NULL;
  }
  memset(vm_page, 0, units * SYSTEM_PAGE_SIZE);
  return vm_page;
}

static void mm_return_vm_page_to_kernel(void *vm_page, int units) {
  if (munmap(vm_page, units * SYSTEM_PAGE_SIZE)) {
    printf("Error: Could not munmap VM page to kernel\n");
  }
}

int main() {
  mm_init();
  printf("VM page size: %lu\n", SYSTEM_PAGE_SIZE);
  void *addr1 = mm_get_new_vm_page_from_kernel(1);
  void *addr2 = mm_get_new_vm_page_from_kernel(1);
  printf("page1 = %p\n", addr1);
  printf("page2 = %p\n", addr2);

  return 0;
}