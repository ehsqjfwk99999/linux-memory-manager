#include "mm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static size_t SYSTEM_PAGE_SIZE = 0;
static vm_page_for_families_t *first_vm_page_for_families = NULL;

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

void mm_instantiate_new_page_family(char *struct_name, size_t struct_size) {
  vm_page_family_t *vm_page_family_curr = NULL;
  vm_page_for_families_t *new_vm_page_for_families = NULL;

  if (struct_size > SYSTEM_PAGE_SIZE) {
    printf("Error: %s() structures %s size exceeds system page size\n",
           __FUNCTION__, struct_name);
    return;
  }

  if (!first_vm_page_for_families) {
    first_vm_page_for_families =
        (vm_page_for_families_t *)mm_get_new_vm_page_from_kernel(1);
    first_vm_page_for_families->next = NULL;
    strncpy(first_vm_page_for_families->vm_page_family[0].struct_name,
            struct_name, MM_MAX_STRUCT_NAME);
    first_vm_page_for_families->vm_page_family[0].struct_size = struct_size;
    first_vm_page_for_families->vm_page_family[0].first_page = NULL;
    init_glthread(&first_vm_page_for_families->vm_page_family[0]
                       .free_block_priority_list_head);
    return;
  }

  size_t count = 0;

  ITERATE_PAGE_FAMILIES_BEGIN(first_vm_page_for_families, vm_page_family_curr) {
    if (strncmp(vm_page_family_curr->struct_name, struct_name,
                MM_MAX_STRUCT_NAME) != 0) {
      count++;
      continue;
    }
    assert(0);
  }
  ITERATE_PAGE_FAMILIES_END(first_vm_page_for_families, vm_page_family_curr)

  if (count == MAX_FAMILIES_PER_VM_PAGE) {
    new_vm_page_for_families =
        (vm_page_for_families_t *)mm_get_new_vm_page_from_kernel(1);
    new_vm_page_for_families->next = first_vm_page_for_families;
    first_vm_page_for_families = new_vm_page_for_families;
    vm_page_family_curr = &first_vm_page_for_families->vm_page_family[0];
  }

  strncpy(vm_page_family_curr->struct_name, struct_name, MM_MAX_STRUCT_NAME);
  vm_page_family_curr->struct_size = struct_size;
  vm_page_family_curr->first_page = NULL;
  init_glthread(&vm_page_family_curr->free_block_priority_list_head);
}

void mm_print_registered_page_families() {
  vm_page_family_t *vm_page_family_curr = NULL;
  vm_page_for_families_t *vm_page_for_families_curr = NULL;

  for (vm_page_for_families_curr = first_vm_page_for_families;
       vm_page_for_families_curr;
       vm_page_for_families_curr = vm_page_for_families_curr->next) {

    ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr,
                                vm_page_family_curr) {
      printf("Page Family: %s, Size = %lu\n", vm_page_family_curr->struct_name,
             vm_page_family_curr->struct_size);
    }
    ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr)
  }
}

vm_page_family_t *lookup_page_family_by_name(char *struct_name) {
  vm_page_family_t *vm_page_family_curr = NULL;
  vm_page_for_families_t *vm_page_for_families_curr = NULL;

  for (vm_page_for_families_curr = first_vm_page_for_families;
       vm_page_for_families_curr;
       vm_page_for_families_curr = vm_page_for_families_curr->next) {
    ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_curr,
                                vm_page_family_curr) {
      if (strncmp(vm_page_family_curr->struct_name, struct_name,
                  MM_MAX_STRUCT_NAME) == 0) {
        return vm_page_family_curr;
      }
    }
    ITERATE_PAGE_FAMILIES_END(vm_page_for_families_curr, vm_page_family_curr)
  }
  return NULL;
}

static void mm_union_free_blocks(block_meta_data_t *first,
                                 block_meta_data_t *second) {
  assert(first->is_free == MM_TRUE && second->is_free == MM_TRUE);

  first->block_size += (sizeof(block_meta_data_t) + second->block_size);

  first->next_block = second->next_block;

  if (second->next_block) {
    second->next_block->prev_block = first;
  }
}

vm_bool_t mm_is_vm_page_emtpy(vm_page_t *vm_page) {
  if (vm_page->block_meta_data.next_block == NULL &&
      vm_page->block_meta_data.prev_block == NULL &&
      vm_page->block_meta_data.is_free == MM_TRUE) {
    return MM_TRUE;
  }
  return MM_FALSE;
}

static inline size_t mm_max_page_allocatable_memory(int units) {
  return (size_t)(SYSTEM_PAGE_SIZE * units - offset_of(vm_page_t, page_memory));
}

vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family) {
  vm_page_t *vm_page = mm_get_new_vm_page_from_kernel(1);

  MARK_VM_PAGE_EMPTY(vm_page)

  vm_page->block_meta_data.block_size = mm_max_page_allocatable_memory(1);
  vm_page->block_meta_data.offset = offset_of(vm_page_t, block_meta_data);
  vm_page->next = NULL;
  vm_page->prev = NULL;
  init_glthread(&vm_page->block_meta_data.priority_thread_glue);

  vm_page->pg_family = vm_page_family;

  if (!vm_page_family->first_page) {
    vm_page_family->first_page = vm_page;
    return vm_page;
  }

  vm_page->next = vm_page_family->first_page;
  vm_page_family->first_page->prev = vm_page;
  vm_page_family->first_page = vm_page;
  return vm_page;
}

void mm_vm_page_delete_and_free(vm_page_t *vm_page) {
  vm_page_family_t *vm_page_family = vm_page->pg_family;

  if (vm_page_family->first_page == vm_page) {
    vm_page_family->first_page = vm_page->next;
    if (vm_page->next) {
      vm_page->next->prev = NULL;
    }
    vm_page->next = NULL;
    vm_page->prev = NULL;
    mm_return_vm_page_to_kernel((void *)vm_page, 1);
    return;
  }

  if (vm_page->next) {
    vm_page->next->prev = vm_page->prev;
  }
  vm_page->prev->next = vm_page->next;
  mm_return_vm_page_to_kernel((void *)vm_page, 1);
}

static int free_blocks_comparison_function(void *_block_meta_data1,
                                           void *_block_meta_data2) {
  block_meta_data_t *block_meta_data1 = (block_meta_data_t *)_block_meta_data1;
  block_meta_data_t *block_meta_data2 = (block_meta_data_t *)_block_meta_data2;

  if (block_meta_data1->block_size > block_meta_data2->block_size) {
    return -1;
  } else if (block_meta_data1->block_size > block_meta_data2->block_size) {
    return 1;
  }
  return 0;
}

static void
mm_add_free_block_meta_data_to_free_block_list(vm_page_family_t *vm_page_family,
                                               block_meta_data_t *free_block) {
  assert(free_block->is_free == MM_TRUE);

  glthread_priority_insert(&vm_page_family->free_block_priority_list_head,
                           &free_block->priority_thread_glue,
                           free_blocks_comparison_function,
                           offset_of(block_meta_data_t, priority_thread_glue));
}