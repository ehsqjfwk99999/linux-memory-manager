#include "uapi_mm.h"

typedef struct emp_ {
  char name[32];
  size_t emp_id;
} emp_t;

typedef struct student_ {
  char name[32];
  size_t rollno;
  size_t marks_phys;
  size_t marks_chem;
  size_t marks_maths;
  struct student_ *next;
} student_t;

int main() {
  mm_init();
  MM_REG_STRUCT(emp_t);
  MM_REG_STRUCT(student_t);
  mm_print_registered_page_families();
  return 0;
}