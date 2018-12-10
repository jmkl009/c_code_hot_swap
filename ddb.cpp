//
// Created by WangJingjin on 2018/12/10.
//

#include "hot_swap.hpp"
#include <unordered_map>
#include <execinfo.h>
#include <libs/bin_dlsym.hpp>

#define UNW_LOCAL_ONLY
#include <libunwind.h>


using std::unordered_map;

//void see_backtrace(int l, double j, void *k) {
//    void **buffer = (void **)malloc(10 * sizeof(void *));
//    size_t i;
//    size_t size = backtrace(buffer, 10);
//    char **strings = backtrace_symbols(buffer, size);
//
//    printf ("Obtained %zd stack frames.\n", size);
//
//    for (i = 0; i < size; i++)
//        printf ("%s:%p\n", strings[i], buffer[i]);
//
//    free (strings);
//    unw_getcontext();
//}

void show_backtrace (void) {
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        printf ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);
    }
}

int main(int argc, char *argv[]) {
//    see_backtrace(0, 0, NULL);
    exe bin("/home/william/introcs/NEAT/NEAT.cpython-36m-x86_64-linux-gnu.so");
    vector<string> linker_flags = bin.lookup_linker_flags();
    for (string &flag : linker_flags) {
        std::cout << flag << std::endl;
    }
//    pid_t target = findProcessByName("libgame.so");
//    unw_create_addr_space()
//    lookup_linker_flags();
//    show_backtrace();
}