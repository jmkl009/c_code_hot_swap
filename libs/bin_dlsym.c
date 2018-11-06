//
// Created by WangJingjin on 2018/11/3.
//

#ifndef BIN_DLSYM
#define BIN_DLSYM

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

Elf64_Addr lookup_symbol(handle_t *h, const char *symname)
{
    int i, j;
    char *strtab;
    Elf64_Sym *symtab;
    for (i = 0; i < h->ehdr->e_shnum; i++) {
        if (h->shdr[i].sh_type == SHT_SYMTAB) {
            strtab = (char *)&h->mem[h->shdr[h->shdr[i].sh_link].sh_offset];
            symtab = (Elf64_Sym *)&h->mem[h->shdr[i].sh_offset];
            for (j = 0; j < h->shdr[i].sh_size / sizeof(Elf64_Sym); j++, symtab++) {
                if(strcmp(&strtab[symtab->st_name], symname) == 0)
                    return (symtab->st_value);
            }
        }
    }
    return 0;
}

void * bin_dlsym(const char * bin_name, const char *symbol) {
    int fd;
    handle_t h;
    struct stat st;

    h.exec = bin_name;
    h.symname = symbol;
    if ((fd = open(h.exec, O_RDONLY)) < 0) {
        perror("open");
        exit(-1);
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        exit(-1);
    }

    h.mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (h.mem == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }

    h.ehdr = (Elf64_Ehdr *)h.mem;
    h.phdr = (Elf64_Phdr *)(h.mem + h.ehdr->e_phoff);
    h.shdr = (Elf64_Shdr *)(h.mem + h.ehdr->e_shoff);

    if (h.mem[0] != 0x7f && !strcmp((char *)&h.mem[1], "ELF")) {
        printf("%s is not an ELF file\n",h.exec);
        exit(-1);
    }

    if (h.ehdr->e_shstrndx == 0 || h.ehdr->e_shoff == 0 || h.ehdr->e_shnum == 0) {
        printf("Section header table not found\n");
        exit(-1);
    }

    if ((h.symaddr = lookup_symbol(&h, h.symname)) == 0) {
        printf("Unable to find symbol: %s not found in executable\n", h.symname);
        exit(-1);
    }

    close(fd);

    return (void *)h.symaddr;
}

#endif