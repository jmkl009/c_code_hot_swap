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

    /**
     *
     * Construct a StackUnwinder given the pid of the target process.
     * Needs ptrace_attach to be called for it to work.
     *
     * @param pid pid of the target process
     */
    StackUnwinder(pid_t pid);
    ~StackUnwinder();

    /**
     * Print the stack trace of the target process
     */
    void print_stack_trace();

    /**
     * Move up a stack frame.
     */
    void up_stack();

    /**
     * Move down a stack frame.
     */
    void down_stack();

    /**
     * Print the information of the current stack frame.
     */
    void print_curr_frame();

    /**
     * Resume the execution of the current stack frame.
     * Needs following ptrace calls to really continue the execution of the target process.
     */
    void resume_curr_frame();

    /**
     * Rewind to the instruction that created this stack frame and execute it once again.
     */
    void restart_curr_frame();

    /**
     *
     * Get the function name of the current stack frame.
     *
     * @return the name of the function
     */
    char *get_curr_funcname();

    /**
     * Get the address of the first instruction of the function of the current stack frame.
     */
    void *get_func_start_addr();

    /**
     *
     * Go to a stack frame by the index.
     *
     * @param idx the index of the traget stack frame
     */
    void go_to_frame(int idx);

    /**
     *
     * Get the calling instruction of the current stack frame.
     *
     * @return the address of the calling instruction
     */
    void *get_func_call_addr();
};


#endif //C_CODE_HOT_SWAP_STACKUNWINDER_H
