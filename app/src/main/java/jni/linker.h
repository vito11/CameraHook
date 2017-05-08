/*
 * linker.h
 *
 *  Created on: 2013-1-17
 *      Author: d
 */

#ifndef LINKER_H_
#define LINKER_H_

struct link_map {
  uintptr_t l_addr;
  char * l_name;
  uintptr_t l_ld;
  struct link_map * l_next;
  struct link_map * l_prev;
};


#define SOINFO_NAME_LEN 128
#define ANDROID_ARM_LINKER

struct soinfo {
  char name[SOINFO_NAME_LEN];
  const Elf32_Phdr* phdr;
  int phnum;
  unsigned entry;
  unsigned base;
  unsigned size;

  int unused;  // DO NOT USE, maintained for compatibility.

  unsigned* dynamic;

  unsigned unused2; // DO NOT USE, maintained for compatibility
  unsigned unused3; // DO NOT USE, maintained for compatibility

  struct soinfo* next;
  unsigned flags;

  const char* strtab;
  Elf32_Sym* symtab;

  unsigned nbucket;
  unsigned nchain;
  unsigned* bucket;
  unsigned* chain;

  unsigned* plt_got;

  Elf32_Rel* plt_rel;
  unsigned plt_rel_count;

  Elf32_Rel* rel;
  unsigned rel_count;

  unsigned* preinit_array;
  unsigned preinit_array_count;

  unsigned* init_array;
  unsigned init_array_count;
  unsigned* fini_array;
  unsigned fini_array_count;

  void (*init_func)();
  void (*fini_func)();

#if defined(ANDROID_ARM_LINKER)
  // ARM EABI section used for stack unwinding.
  unsigned* ARM_exidx;
  unsigned ARM_exidx_count;
#elif defined(ANDROID_MIPS_LINKER)
#if 0
  // Not yet.
  unsigned* mips_pltgot
#endif
  unsigned mips_symtabno;
  unsigned mips_local_gotno;
  unsigned mips_gotsym;
#endif

  unsigned refcount;
  struct link_map linkmap;



};



#endif /* LINKER_H_ */
