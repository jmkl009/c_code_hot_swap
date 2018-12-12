//
// Created by WangJingjin on 2018-12-10.
//

#ifndef DDB_UTILS_H
#define DDB_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <libunwind-ptrace.h>
#include "bin_dlsym.hpp"

#define INTEL_RET_INSTRUCTION 0xc3
#define INTEL_INT3_INSTRUCTION 0xcc

char *get_filename_from_path(char *filePath);
pid_t findProcessByName(char* processName);
long freespaceaddr(pid_t pid);
long getlibcaddr(pid_t pid);
void * checkloaded(pid_t pid, char* libname, char *symbol);
long getFunctionAddress(char* funcName);
unsigned char* findRet(void* endAddr);
void usage(char* name);
void *get_base_addr(pid_t pid);
void show_stack_trace(pid_t pid);

#endif //DDB_UTILS_H
