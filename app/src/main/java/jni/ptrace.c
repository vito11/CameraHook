
#include <stdio.h>
#include <sys/ptrace.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#ifdef ANDROID
#include <sys/user.h>
#else
#include <sys/user.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <utils.h>
#include <stdarg.h>
#include "linker.h"
#include "utils.h"

#define CPSR_T_MASK     ( 1u << 5 )

static regs_t oldregs;

dl_fl_t ldl;

void ptrace_dump_regs(regs_t *regs, char *msg) {
    int i = 0;
    LOGI("------regs %s-----\n", msg);
    for (i = 0; i < 18; i++) {
        LOGI("r[%02d]=%lx\n", i, regs->uregs[i]);
    }
}


int  ptrace_attach(int pid) {
       regs_t regs;
       int status = 0;

       if (ptrace(PTRACE_ATTACH, pid, NULL, NULL ) < 0){
           LOGE("ptrace_attach failed:%d",errno);
           return -1;
       }
       status = ptrace_wait_for_signal(pid, SIGCONT);

       ptrace_readreg(pid, &regs);
       memcpy(&oldregs, &regs, sizeof(regs));

       ptrace_dump_regs(&oldregs, "old regs");

       if (regs.ARM_pc& 1) {
                   /* thumb */
                   regs.ARM_pc = 0x11;
                   regs.ARM_cpsr |=0x30;
               } else {
                   /* arm */
                   regs.ARM_pc= 0;

               }

       ptrace_writereg(pid, &regs);

       ptrace_cont(pid);


       status = ptrace_wait_for_signal(pid, SIGSEGV);
       ptrace_readreg(pid, &regs);
       ptrace_dump_regs(&regs, "stop regs");
       return 0;

}



void ptrace_cont(int pid) {
    //int stat;

    if (ptrace(PTRACE_CONT, pid, NULL, NULL ) < 0) {
        perror("ptrace_cont");
        exit(-1);
    }

    //while (!WIFSTOPPED(stat))
    //    waitpid(pid, &stat, WNOHANG);
}

void ptrace_detach(int pid) {
    ptrace_writereg(pid, &oldregs);

    if (ptrace(PTRACE_DETACH, pid, NULL, NULL ) < 0) {
        perror("ptrace_detach");
        exit(-1);
    }
}


void ptrace_write(int pid, unsigned long addr, void *vptr, int len) {
    int count;

    union u {
            long val;
            char chars[sizeof(long)];
    } d;

    char *src = (char *) vptr;
    count = 0;
    LOGI("ptrace_write addr: %lx,pid: %d", addr,pid);
    LOGI("ptrace_write addr: %s", vptr);

    while (count < len) {
        int size = 4;
        if(count + 4 > len)
        {
           size = len - count;
        }
        memcpy(d.chars, src+count, size);
        int status = ptrace(PTRACE_POKETEXT, pid, (void*) (addr + count), d.val);
        count += 4;

        if(status)
        {
          LOGI("ptrace_write failed");
        }
    }
}

void ptrace_read(int pid, unsigned long addr, void *vptr, int len) {
    int i, count;
    long word;
    unsigned long *ptr = (unsigned long *) vptr;

    i = count = 0;


    while (count < len) {
        word = ptrace(PTRACE_PEEKTEXT, pid, (void*) (addr + count), NULL );
        count += 4;
        ptr[i++] = word;
    }
}

char * ptrace_readstr(int pid, unsigned long addr) {
    char *str = (char *) malloc(64);
    int i, count;
    long word;
    char *pa;

    i = count = 0;
    pa = (char *) &word;

    while (i <= 60) {
        word = ptrace(PTRACE_PEEKTEXT, pid, (void*) (addr + count), NULL );
        count += 4;

        if (pa[0] == '\0') {
            str[i] = '\0';
            break;
        } else
            str[i++] = pa[0];

        if (pa[1] == '\0') {
            str[i] = '\0';
            break;
        } else
            str[i++] = pa[1];

        if (pa[2] == '\0') {
            str[i] = '\0';
            break;
        } else
            str[i++] = pa[2];

        if (pa[3] == '\0') {
            str[i] = '\0';
            break;
        } else
            str[i++] = pa[3];
    }
    return str;
}


void ptrace_readreg(int pid, regs_t *regs) {
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs))
        LOGI("*** ptrace_readreg error ***\n");

}

void ptrace_writereg(int pid, regs_t *regs) {
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs))
        LOGI("*** ptrace_writereg error ***\n");
}


unsigned long ptrace_push(int pid, regs_t *regs, void *paddr, int size) {
#ifdef ANDROID
    unsigned long arm_sp;
    LOGI("size: %d", size);

    LOGI("ARM_sp addr: %lx", regs->ARM_sp);

    arm_sp = regs->ARM_sp;
    arm_sp -= size;
    arm_sp = arm_sp - arm_sp % 4;
    regs->ARM_sp= arm_sp;

    LOGI("ARM_sp addr: %lx", regs->ARM_sp);

    ptrace_write(pid, arm_sp, paddr, size);
    return arm_sp;
#else
    unsigned long esp;
    regs_t regs;
    ptrace_readreg(pid, &regs);
    esp = regs.esp;
    esp -= size;
    esp = esp - esp % 4;
    regs.esp = esp;
    ptrace_writereg(pid, &regs);
    ptrace_write(pid, esp, paddr, size);
    return esp;
#endif
}

long ptrace_stack_alloc(pid_t pid, regs_t *regs, int size) {
    unsigned long arm_sp;
    arm_sp = regs->ARM_sp;
    arm_sp -= size;
    arm_sp = arm_sp - arm_sp % 4;
    regs->ARM_sp= arm_sp;
    return arm_sp;
}

void *ptrace_dlopen(pid_t pid, const char *filename, int flag) {

#ifdef ANDROID
    regs_t regs;
    //int stat;
    ptrace_readreg(pid, &regs);

    ptrace_dump_regs(&regs, "before call to ptrace_dlopen\n");

    regs.ARM_r0= (unsigned long)ptrace_push(pid,&regs, (void*)filename,strlen(filename)+1);
    regs.ARM_r1= flag;

    regs.ARM_pc= ldl.l_dlopen;

    if (regs.ARM_pc& 1) {
        /* thumb */
        regs.ARM_pc &= (~1u);
        regs.ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs.ARM_cpsr &= ~CPSR_T_MASK;
    }
    regs.ARM_lr= 0;

    ptrace_writereg(pid, &regs);
    ptrace_cont(pid);
    LOGI("done %d\n", ptrace_wait_for_signal(pid, SIGSEGV));
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before return ptrace_call\n");
    return (void*) regs.ARM_r0;
#endif

}


void *ptrace_dlsym(pid_t pid, void *handle, const char *symbol) {

#ifdef ANDROID
    regs_t regs;
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before call to ptrace_dlsym\n");


    regs.ARM_r0= (unsigned long)handle;
    regs.ARM_r1= (unsigned long)ptrace_push(pid,&regs, (void*)symbol,strlen(symbol)+1);

    regs.ARM_pc= ldl.l_dlsym;

    if (regs.ARM_pc& 1) {
        /* thumb */
        regs.ARM_pc &= (~1u);
        regs.ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs.ARM_cpsr &= ~CPSR_T_MASK;
    }

    regs.ARM_lr= 0;

    ptrace_writereg(pid, &regs);
    ptrace_dump_regs(&regs, "before continue ptrace_dlsym\n");
    ptrace_cont(pid);
    printf("done %d\n", ptrace_wait_for_signal(pid, SIGSEGV));
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before return ptrace_dlsym\n");

    return (void*) regs.ARM_r0;
#endif
}

int ptrace_mymath_add(pid_t pid, long mymath_add_addr, int a, int b) {
#ifdef ANDROID
    regs_t regs;
    //int stat;
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before call to ptrace_mymath_add\n");

#ifdef THUMB
    regs.ARM_lr = 1;
#else
    regs.ARM_lr= 0;
#endif

    regs.ARM_r0= a;
    regs.ARM_r1= b;

    regs.ARM_pc= mymath_add_addr;
    ptrace_writereg(pid, &regs);
    ptrace_cont(pid);
    LOGI("done %d\n", ptrace_wait_for_signal(pid, SIGSEGV));
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before return ptrace_mymath_add\n");

    return regs.ARM_r0;
#endif
}

int ptrace_call(int pid, long proc, int argc, ptrace_arg *argv) {
    int i = 0;
#define ARGS_MAX 64
    regs_t regs;
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before ptrace_call\n");

    /*prepare stacks*/
    for (i = 0; i < argc; i++) {
        ptrace_arg *arg = &argv[i];
        if (arg->type == PAT_STR) {
            arg->_stackid = ptrace_push(pid, &regs, arg->s, strlen(arg->s) + 1);
        } else if (arg->type == PAT_MEM) {
            //LOGI("push data %p to stack[%d] :%d \n", arg->mem.addr, stackcnt, *((int*)arg->mem.addr));
            arg->_stackid = ptrace_push(pid, &regs, arg->mem.addr, arg->mem.size);
        }
    }
    for (i = 0; (i < 4) && (i < argc); i++) {
        ptrace_arg *arg = &argv[i];
        if (arg->type == PAT_INT) {
            regs.uregs[i] = arg->i;
        } else if (arg->type == PAT_STR) {
            regs.uregs[i] = arg->_stackid;
        } else if (arg->type == PAT_MEM) {
            regs.uregs[i] = arg->_stackid;
        } else {
            LOGI("unkonwn arg type\n");
        }
    }

    for (i = argc - 1; i >= 4; i--) {
        ptrace_arg *arg = &argv[i];
        if (arg->type == PAT_INT) {
            ptrace_push(pid, &regs, &arg->i, sizeof(int));
        } else if (arg->type == PAT_STR) {
            ptrace_push(pid, &regs, &arg->_stackid, sizeof(unsigned long));
        } else if (arg->type == PAT_MEM) {
            ptrace_push(pid, &regs, &arg->_stackid, sizeof(unsigned long));
        } else {
            LOGI("unkonwn arg type\n");
        }
    }
#ifdef THUMB
    regs.ARM_lr = 1;
#else
    regs.ARM_lr= 0;
#endif
    regs.ARM_pc= proc;
    ptrace_writereg(pid, &regs);
    ptrace_cont(pid);
    LOGI("done %d\n", ptrace_wait_for_signal(pid, SIGSEGV));
    ptrace_readreg(pid, &regs);
    ptrace_dump_regs(&regs, "before return ptrace_call\n");

    //sync memory
    for (i = 0; i < argc; i++) {
        ptrace_arg *arg = &argv[i];
        if (arg->type == PAT_STR) {
        } else if (arg->type == PAT_MEM) {
            ptrace_read(pid, arg->_stackid, arg->mem.addr, arg->mem.size);
        }
    }

    return regs.ARM_r0;
}

int ptrace_wait_for_signal(int pid, int signal) {
    int status;
    pid_t res;

    res = waitpid(pid, &status, 0);
    LOGI("ptrace_wait_for_signal:%d,%s",res,strerror(errno));
    if (res != pid || !WIFSTOPPED (status))
        return 0;
    LOGI("WSTOPSIG:%d",WSTOPSIG (status));
    return WSTOPSIG (status) == signal;
}

static Elf32_Addr get_linker_base(int pid, Elf32_Addr *base_start, Elf32_Addr *base_end) {
        unsigned long base = 0;
        char mapname[FILENAME_MAX];
        memset(mapname, 0, FILENAME_MAX);
        snprintf(mapname, FILENAME_MAX, "/proc/%d/maps", pid);
        FILE *file = fopen(mapname, "r");
        *base_start = *base_end = 0;
        if (file) {
            //400a4000-400b9000 r-xp 00000000 103:00 139       /system/bin/linker
            while (1) {
                unsigned int atleast = 32;
                int xpos = 20;
                char startbuf[9];
                char endbuf[9];
                char line[FILENAME_MAX];
                memset(line, 0, FILENAME_MAX);
                char *linestr = fgets(line, FILENAME_MAX, file);
                if (!linestr) {
                    break;
                }
               // printf("........%s <--\n", line);
                if (strlen(line) > atleast && strstr(line, "/system/bin/linker")) {
                    memset(startbuf, 0, sizeof(startbuf));
                    memset(endbuf, 0, sizeof(endbuf));


                    memcpy(startbuf, line, 8);
                    memcpy(endbuf, &line[8 + 1], 8);
                    if (*base_start == 0) {
                        *base_start = strtoul(startbuf, NULL, 16);
                        *base_end = strtoul(endbuf, NULL, 16);
                        base = *base_start;
                    } else {
                        *base_end = strtoul(endbuf, NULL, 16);
                    }
                }
            }
            fclose(file);


        }
        return base;

}

dl_fl_t *ptrace_find_dlinfo(int pid) {
        Elf32_Sym sym;
        Elf32_Addr addr;
        struct soinfo lsi;
    #define LIBDLSO "libdl.so"
        Elf32_Addr base_start = 0;
        Elf32_Addr base_end = 0;
        Elf32_Addr base = get_linker_base(pid, &base_start, &base_end);


        if (base == 0) {
            printf("no linker found\n");
            return NULL ;
        } else {
            printf("search libdl.so from %08u to %08u\n", base_start, base_end);
        }

        for (addr = base_start; addr < base_end; addr += 4) {
            char soname[strlen(LIBDLSO)];
            Elf32_Addr off = 0;

            ptrace_read(pid, addr, soname, strlen(LIBDLSO));

            if (strncmp(LIBDLSO, soname, strlen(LIBDLSO))) {
                continue;
            }

            ptrace_read(pid, addr, &lsi, sizeof(lsi));

            off = (Elf32_Addr)lsi.symtab;

            ptrace_read(pid, off, &sym, sizeof(sym));
            //just skip
            off += sizeof(sym);

            ptrace_read(pid, off, &sym, sizeof(sym));
            ldl.l_dlopen = sym.st_value + lsi.base;
            off += sizeof(sym);

            ptrace_read(pid, off, &sym, sizeof(sym));
            ldl.l_dlclose = sym.st_value + lsi.base;
            off += sizeof(sym);

            ptrace_read(pid, off, &sym, sizeof(sym));
            ldl.l_dlsym = sym.st_value + lsi.base;
            off += sizeof(sym);

            LOGI("dlopen addr %p\n", (void*) ldl.l_dlopen);
            LOGI("dlclose addr %p\n", (void*) ldl.l_dlclose);
            LOGI("dlsym addr %p\n", (void*) ldl.l_dlsym);
            return &ldl;

        }
        printf("%s not found!\n", LIBDLSO);
        return NULL ;
}


int find_pid_of( const char *process_name )
{
	int id;
	pid_t pid = -1;
	DIR* dir;
	FILE *fp;
	char filename[32];
	char cmdline[256];

	struct dirent * entry;

	if ( process_name == NULL )
		return -1;

	dir = opendir( "/proc" );
	if ( dir == NULL )
		return -1;

	while( (entry = readdir( dir )) != NULL )
	{
		id = atoi( entry->d_name );
		if ( id != 0 )
		{
			sprintf( filename, "/proc/%d/cmdline", id );
			fp = fopen( filename, "r" );
			if ( fp )
			{
				fgets( cmdline, sizeof(cmdline), fp );
				fclose( fp );

				if ( strcmp( process_name, cmdline ) == 0 )
				{
					/* process found */
					pid = id;
					break;
				}
			}
		}
	}

	closedir( dir );

	return pid;
}


void* get_module_base( pid_t pid, const char* module_name )
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];

	if ( pid < 0 )
	{
		/* self process */
		snprintf( filename, sizeof(filename), "/proc/self/maps", pid );
	}
	else
	{
		snprintf( filename, sizeof(filename), "/proc/%d/maps", pid );
	}

	fp = fopen( filename, "r" );

	if ( fp != NULL )
	{
		while ( fgets( line, sizeof(line), fp ) )
		{
			if ( strstr( line, module_name ) )
			{
				pch = strtok( line, "-" );
				addr = strtoul( pch, NULL, 16 );

				if ( addr == 0x8000 )
					addr = 0;

				break;
			}
		}

				fclose( fp ) ;
	}

	return (void *)addr;
}

void* get_remote_addr( pid_t target_pid, const char* module_name, void* local_addr )
{
	void* local_handle, *remote_handle;

	local_handle = get_module_base( -1, module_name );
	remote_handle = get_module_base( target_pid, module_name );

	LOGI( "[+] get_remote_addr: local[%x], remote[%x]\n", local_handle, remote_handle );

	return (void *)( (uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle );
}

static char *nexttoksep(char **strp, char *sep)
{
    char *p = strsep(strp,sep);
    return (p == 0) ? "" : p;
}
static char *nexttok(char **strp)
{
    return nexttoksep(strp, " ");
}

int find_module_info_by_address(pid_t pid, void* addr, char *module, void** start, void** end) {
	char statline[1024];
	FILE *fp;
	char *address, *proms, *ptr, *p;

	if ( pid < 0 ) {
		/* self process */
		snprintf( statline, sizeof(statline), "/proc/self/maps");
	} else {
		snprintf( statline, sizeof(statline), "/proc/%d/maps", pid );
	}

	fp = fopen( statline, "r" );

	if ( fp != NULL ) {
		while ( fgets( statline, sizeof(statline), fp ) ) {
			ptr = statline;
			address = nexttok(&ptr); // skip address
			proms = nexttok(&ptr); // skip proms
			nexttok(&ptr); // skip offset
			nexttok(&ptr); // skip dev
			nexttok(&ptr); // skip inode

			while(*ptr != '\0') {
				if(*ptr == ' ')
					ptr++;
				else
					break;
			}

			p = ptr;
			while(*p != '\0') {
				if(*p == '\n')
					*p = '\0';
				p++;
			}

			// 4016a000-4016b000
			if(strlen(address) == 17) {
				address[8] = '\0';

				*start = (void*)strtoul(address, NULL, 16);
				*end   = (void*)strtoul(address+9, NULL, 16);

				// printf("[%p-%p] %s | %p\n", *start, *end, ptr, addr);

				if(addr > *start && addr < *end) {
					strcpy(module, ptr);

					fclose( fp ) ;
					return 0;
				}
			}
		}

		fclose( fp ) ;
	}

	return -1;
}

int find_module_info_by_name(pid_t pid, const char *module, void** start, void** end) {
	char statline[1024];
	FILE *fp;
	char *address, *proms, *ptr, *p;

	if ( pid < 0 ) {
		/* self process */
		snprintf( statline, sizeof(statline), "/proc/self/maps");
	} else {
		snprintf( statline, sizeof(statline), "/proc/%d/maps", pid );
	}

	fp = fopen( statline, "r" );

	if ( fp != NULL ) {
		while ( fgets( statline, sizeof(statline), fp ) ) {
			ptr = statline;
			address = nexttok(&ptr); // skip address
			proms = nexttok(&ptr); // skip proms
			nexttok(&ptr); // skip offset
			nexttok(&ptr); // skip dev
			nexttok(&ptr); // skip inode

			while(*ptr != '\0') {
				if(*ptr == ' ')
					ptr++;
				else
					break;
			}

			p = ptr;
			while(*p != '\0') {
				if(*p == '\n')
					*p = '\0';
				p++;
			}

			// 4016a000-4016b000
			if(strlen(address) == 17) {
				address[8] = '\0';

				*start = (void*)strtoul(address, NULL, 16);
				*end   = (void*)strtoul(address+9, NULL, 16);

				// printf("[%p-%p] %s | %p\n", *start, *end, ptr, addr);

				if(strncmp(module, ptr, strlen(module)) == 0) {
					fclose( fp ) ;
					return 0;
				}
			}
		}

		fclose( fp ) ;
	}

	return -1;
}

void* get_remote_address2(pid_t pid, void *local_addr) {
	char buf[256];
	void* local_start = 0;
	void* local_end = 0;
	void* remote_start = 0;
	void* remote_end = 0;

	if(find_module_info_by_address(-1, local_addr, buf, &local_start, &local_end) < 0) {
		LOGI("[-] find_module_info_by_address FAIL");
		return NULL;
	}

	LOGI("[+] the local module is %s", buf);

	if(find_module_info_by_name(pid, buf, &remote_start, &remote_end) < 0) {
		LOGI("[-] find_module_info_by_name FAIL");
		return NULL;
	}

	return (void *)( (uint32_t)local_addr + (uint32_t)remote_start - (uint32_t)local_start );
}