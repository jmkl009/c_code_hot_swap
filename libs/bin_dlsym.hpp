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
    Elf64_Ehdr *ehdr; //Elf header
    Elf64_Phdr *phdr; //Program header
    Elf64_Shdr *shdr; //Section header
    uint8_t *mem;
    char *symname;
    Elf64_Addr symaddr;
    struct user_regs_struct pt_reg;
    char *exec;
} handle_t;

typedef struct symaddr {
    void * addr; //Virtual memory address
    uint16_t type; //Shared library or executable
} symaddr_t;

class exe {
private:
    int fd;
    handle_t h;
    size_t mem_len;
    char *bin_name;
    Elf64_Addr lookup_symbol(const char *symname);

public:

    /**
     * Consturct an exe object from an Elf file.
     * @param name The name of the Elf file
     */
    exe(char * name);
    ~exe();

    /**
     * Given a symbol (usually function name), output its address in memory. (Just like dlsym)
     * @param symbol The name of the symbol
     * @return a symaddr struct which indicates whether it is a shared library
     * or an excutable along with its place in memory.
     */
    symaddr_t bin_dlsym(char *symbol);

    /**
     * Get the linker flags the executable was compiled with.
     * @return a vector of linker flags the executable was compiled with.
     */
    vector<string> lookup_linker_flags();

};

#endif //DDB_BIN_DLSYM_H
