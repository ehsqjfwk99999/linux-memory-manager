#ifndef _MM_H_
#define _MM_H_

#include "glthread.h"
#include <unistd.h>

#define MM_MAX_STRUCT_NAME 32

typedef enum { MM_FALSE, MM_TRUE } vm_bool_t;

typedef struct block_meta_data_ {
  vm_bool_t is_free;
  size_t block_size;
  size_t offset;
  glthread_t priority_thread_glue;
  struct block_meta_data_ *prev_block;
  struct block_meta_data_ *next_block;
} block_meta_data_t;

typedef struct vm_page_ {
  struct vm_page_ *prev;
  struct vm_page_ *next;
  struct vm_page_family_ *pg_family;
  block_meta_data_t block_meta_data;
  char page_memory[0];
} vm_page_t;

typedef struct vm_page_family_ {
  char struct_name[MM_MAX_STRUCT_NAME];
  size_t struct_size;
  vm_page_t *first_page;
  glthread_t free_block_priority_list_head;
} vm_page_family_t;

typedef struct vm_page_for_families_ {
  struct vm_page_for_families_ *next;
  vm_page_family_t vm_page_family[0];
} vm_page_for_families_t;

vm_page_family_t *lookup_page_family_by_name(char *struct_name);
vm_page_t *allocate_vm_page(vm_page_family_t *vm_page_family);
void mm_vm_page_delete_and_free(vm_page_t *vm_page);

#define MAX_FAMILIES_PER_VM_PAGE                                               \
  (SYSTEM_PAGE_SIZE - sizeof(vm_page_for_families_t *)) /                      \
      sizeof(vm_page_family_t)

#define ITERATE_PAGE_FAMILIES_BEGIN(vm_page_for_families_ptr, curr)            \
  {                                                                            \
    size_t count = 0;                                                          \
    for (curr =                                                                \
             (vm_page_family_t *)&vm_page_for_families_ptr->vm_page_family[0]; \
         curr->struct_size && count < MAX_FAMILIES_PER_VM_PAGE;                \
         curr++, count++)

#define ITERATE_PAGE_FAMILIES_END(vm_page_for_families_ptr, curr) }

#define offset_of(container_structure, field_name)                             \
  ((size_t) & (((container_structure *)0)->field_name))

#define MM_GET_PAGE_FROM_META_BLOCK(block_meta_data_ptr)                       \
  ((size_t)((size_t)block_meta_data_ptr - block_meta_data_ptr->offset))

#define NEXT_META_BLOCK(block_meta_data_ptr) (block_meta_data_ptr->next_block)

#define NEXT_META_BLOCK_BY_SIZE(block_meta_data_ptr)                           \
  (block_meta_data_t *)((size_t)(block_meta_data_ptr + 1) +                    \
                        block_meta_data_ptr->block_size)

#define PREV_META_BLOCK(block_meta_data_ptr) (block_meta_data_ptr->prev_block)

#define mm_bind_blocks_for_allocation(allocated_meta_block, free_meta_block)   \
  free_meta_block->next_block = allocated_meta_block->next_block;              \
  free_meta_block->prev_block = allocated_meta_block;                          \
  allocated_meta_block->next_block = free_meta_block;                          \
  if (free_meta_block->next_block) {                                           \
    free_meta_block->next_block->prev_block = free_meta_block;                 \
  }

#define MARK_VM_PAGE_EMPTY(vm_page_t_ptr)                                      \
  vm_page_t_ptr->block_meta_data.next_block = NULL;                            \
  vm_page_t_ptr->block_meta_data.prev_block = NULL;                            \
  vm_page_t_ptr->block_meta_data.is_free = MM_TRUE;

#define ITERATE_VM_PAGE_BEGIN(vm_page_family_ptr, curr)                        \
  {                                                                            \
    curr = &vm_page_family_ptr->first_page;                                    \
    vm_page_t *next = NULL;                                                    \
    for (; curr; curr = next) {                                                \
      next = curr->next;

#define ITERATE_VM_PAGE_END(vm_page_family_ptr, curr)                          \
  }                                                                            \
  }

#define ITERATE_VM_PAGE_ALL_BLOCKS_BEGIN(vm_page_ptr, curr)                    \
  {                                                                            \
    curr = &vm_page_ptr->block_meta_data;                                      \
    block_meta_data_t *next = NULL;                                            \
    for (; curr; curr = next) {                                                \
      next = curr->next_block;

#define ITERATE_VM_PAGE_ALL_BLOCKS_END(vm_page_ptr, curr)                      \
  }                                                                            \
  }

#endif