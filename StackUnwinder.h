//
// Created by WangJingjin on 2018-12-11.
//

#ifndef C_CODE_HOT_SWAP_STACKUNWINDER_H
#define C_CODE_HOT_SWAP_STACKUNWINDER_H
#include <libunwind-ptrace.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

using std::vector;

struct elf_image
{
    void *image;                /* pointer to mmap'd image */
    size_t size;                /* (file-) size of the image */
};

struct elf_dyn_info
{
    struct elf_image ei;
    unw_dyn_info_t di_cache;
    unw_dyn_info_t di_debug;    /* additional table info for .debug_frame */
};

struct UPT_info
{
    pid_t pid;          /* the process-id of the child we're unwinding */
    struct elf_dyn_info edi;
};

class StackUnwinder {
private:
    pid_t target;
    void *context;
    vector<unw_cursor_t> stack_trace;
    unsigned stack_trace_idx;
    unsigned user_stack_trace_idx;
    char *tmp_symbol_holder;

//    void go_to_stack_idx(unsigned target_idx);

public:
    StackUnwinder(pid_t pid);
    ~StackUnwinder();

    void print_stack_trace();
    void up_stack();
    void down_stack();
    void print_curr_frame();
    void resume_curr_frame();
    void restart_curr_frame();
    char *get_curr_funcname();
    void *get_func_start_addr();
    void go_to_frame(int idx);
    void *get_func_call_addr();
};


#endif //C_CODE_HOT_SWAP_STACKUNWINDER_H
