/*
 * inject.c
 *
 *  Created on: Jun 4, 2011
 *      Author: d
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "utils.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <jni.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

const char *linker_path = "/system/bin/linker";

char *sos[] = {
        "libcameraservice.so",
        NULL
};

int main(int argc, char *argv[]) {

    int pid;
    struct soinfo si;
    struct elf_info einfo;

    extern dl_fl_t ldl;

    void *handle = NULL;
    long proc = 0;

    pid = atoi(argv[1]);
    LOGE("test:%d",pid);

    ptrace_attach(pid);
    ptrace_find_dlinfo(pid);

    handle = ptrace_dlopen(pid, "/system/lib/libhook.so",RTLD_NOW);
    //handle = ptrace_dlopen(pid, "/data/data/com.vito.research.camerahook/libhook.so",RTLD_NOW);
    LOGE("ptrace_dlopen:%lx",handle);

    proc = (long)ptrace_dlsym(pid, handle, "my_memcpy");

    LOGE("ptrace_dlsym:%lx",proc);

    replace_all_rels(pid, "memcpy", proc, sos);
    ptrace_detach(pid);
    exit(0);
    return 0;
}










