#ifndef _UAPI_MM_H_
#define _UAPI_MM_H_

#include <unistd.h>

void mm_init();

void mm_instantiate_new_page_family(char *struct_name, size_t struct_size);

#define MM_REG_STRUCT(struct_name)                                             \
  (mm_instantiate_new_page_family(#struct_name, sizeof(struct_name)))

void mm_print_registered_page_families();

#endif