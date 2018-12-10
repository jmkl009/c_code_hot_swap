//
// Created by WangJingjin on 2018/11/3.
//

#ifndef BIN_DLSYM
#define BIN_DLSYM

#include "bin_dlsym.hpp"

Elf64_Addr exe::lookup_symbol(const char *symname) {
    int i, j;
    char *strtab;
    Elf64_Sym *symtab;
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_SYMTAB) {
            strtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
            symtab = (Elf64_Sym *)&h.mem[h.shdr[i].sh_offset];
            for (j = 0; (unsigned)j < h.shdr[i].sh_size / sizeof(Elf64_Sym); j++, symtab++) {
                if(strcmp(&strtab[symtab->st_name], symname) == 0)
                    return (symtab->st_value);
            }
        }
    }
    return 0;
}

    exe::exe(char * name) {
        bin_name = strdup(name);
        h.exec = bin_name;
        if ((fd = open(h.exec, O_RDONLY)) < 0) {
            perror("open");
            exit(-1);
        }

        struct stat st;
        if (fstat(fd, &st) < 0) {
            perror("fstat");
            exit(-1);
        }

        mem_len = (size_t)st.st_size;
        h.mem = (uint8_t *)mmap(nullptr, mem_len, PROT_READ, MAP_PRIVATE, fd, 0);
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
    }

    exe::~exe() {
        munmap(h.mem, mem_len);
        close(fd);
        free(bin_name);
    }

    symaddr_t exe::bin_dlsym(char *symbol) {
        h.symname = symbol;
        if ((h.symaddr = lookup_symbol(h.symname)) == 0) {
            printf("Unable to find symbol: %s not found in executable\n", h.symname);
            exit(-1);
        }

        return (symaddr_t){.addr = (void *)h.symaddr, .type = h.ehdr->e_type};
    }

vector<string> exe::lookup_linker_flags() {
    vector<string> linker_flags;
    int i, j;
    char *strtab;
    Elf64_Dyn *dyntab;
    char buffer[256];
    strncpy(buffer, "-l", 2);
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_DYNAMIC) {
            dyntab = (Elf64_Dyn *)&h.mem[h.shdr[i].sh_offset];
            strtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
            for (j = 0; (unsigned)j < h.shdr[i].sh_size / sizeof(Elf64_Dyn); j++) {
                if (dyntab[j].d_tag == DT_NEEDED) {
//                        printf("%s\n", strtab + dyntab[j].d_un.d_val);
                    char *libname = strtab + dyntab[j].d_un.d_val;
                    char *realname_start = strstr(libname, "lib");
                    if (!realname_start) {
                        continue;
                    }
                    realname_start += 3;
                    char *realname_end = strstr(realname_start, ".so");
                    size_t libname_size = realname_end - realname_start;
                    strncpy(buffer + 2, realname_start, libname_size);
                    buffer[libname_size + 2] = '\0';
//                        printf("%s\n", buffer);
                    linker_flags.emplace_back(string(buffer));
                }
            }
            break;
        }
    }
    return linker_flags;
}

#endif
