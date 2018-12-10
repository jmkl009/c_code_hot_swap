//
// Created by WangJingjin on 2018-12-10.
//

#ifndef DDB_BIN_DLSYM_H
#define DDB_BIN_DLSYM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <elf.h>

#include <sys/types.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <vector>
#include <string>
#include <iostream>

using std::vector;
using std::string;

typedef struct handle {
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shdr;
    uint8_t *mem;
    char *symname;
    Elf64_Addr symaddr;
    struct user_regs_struct pt_reg;
    char *exec;
} handle_t;

typedef struct symaddr {
    void * addr;
    uint16_t type;
} symaddr_t;

class exe {
private:
    int fd;
    handle_t h;
    size_t mem_len;
    char *bin_name;
    Elf64_Addr lookup_symbol(const char *symname);

public:
    exe(char * name);
    ~exe();
    symaddr_t bin_dlsym(char *symbol);
    vector<string> lookup_linker_flags();

};

#endif //DDB_BIN_DLSYM_H
