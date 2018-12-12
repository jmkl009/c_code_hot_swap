//
// Created by WangJingjin on 2018-12-11.
//

#include "StackUnwinder.h"

int half_resume (unw_addr_space_t as, unw_cursor_t *c, void *arg) {
    return 0;
}

StackUnwinder::StackUnwinder(pid_t pid) : target(pid), stack_trace_idx(0), user_stack_trace_idx(0), tmp_symbol_holder(NULL) {
    _UPT_accessors.resume = half_resume;
    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    context = _UPT_create(target);
    unw_cursor_t cursor;
    if (unw_init_remote(&cursor, as, context) != 0) {
        fprintf(stderr, "ERROR: cannot initialize cursor for remote unwinding\n");
        exit(1);
    }
    do {
        stack_trace.emplace_back(cursor);
    } while(unw_step(&cursor) > 0);
}

StackUnwinder::~StackUnwinder() {
    if (tmp_symbol_holder) {
        free(tmp_symbol_holder);
    }
    _UPT_destroy(context);
}

void StackUnwinder::print_stack_trace() {
    unsigned curr_stack_trace_idx = user_stack_trace_idx;
    for (unsigned i = curr_stack_trace_idx; i < stack_trace.size(); i++) {
        print_curr_frame();
        up_stack();
    }
    user_stack_trace_idx = curr_stack_trace_idx;
}

//void StackUnwinder::go_to_stack_idx(unsigned target_idx) {
//    size_t stack_trace_size = stack_trace.size();
//    size_t emplace_count = target_idx + 1 - stack_trace_size;
//    unw_cursor_t last_cursor = stack_trace[stack_trace_size - 1];
//    while (emplace_count > 0 && unw_step(&last_cursor) > 0) {
//        stack_trace.emplace_back(last_cursor);
//        emplace_count--;
//    }
//
//    if (target_idx >= stack_trace_size) {
//        target_idx = stack_trace_size - 1;
//    }
//    user_stack_trace_idx = target_idx;
//    cursor = stack_trace[target_idx];
//}

void StackUnwinder::up_stack() {
    if (user_stack_trace_idx < stack_trace.size()) {
        user_stack_trace_idx++;
    }
}

void StackUnwinder::down_stack() {
    if (user_stack_trace_idx > 0) {
        user_stack_trace_idx--;
    }
}

void StackUnwinder::print_curr_frame() {
    unw_cursor_t cursor = stack_trace[user_stack_trace_idx];
    unw_word_t offset, pc;
    char sym[1024];
    if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
        printf("ERROR: cannot read program counter\n");
        exit(1);
    }

    char command_buf[128];
    sprintf(command_buf, "/usr/bin/addr2line -e /proc/%d/exe %lx", target, pc);
    FILE *fp = popen(command_buf, "r");
    char *line = NULL;
    size_t cap = 0;
    ssize_t len = getline(&line, &cap, fp);
    if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
        if (len != -1) {
            line[len - 1] = '\0';
            printf("%d (%s+0x%lx): 0x%lx (%s)\n", user_stack_trace_idx, sym, offset, pc, line);
        } else {
            printf("%d (%s+0x%lx): 0x%lx\n", user_stack_trace_idx, sym, offset, pc);
        }
    } else {
        printf("-- no symbol name found\n");
    }

    free(line);
    pclose(fp);
}

void StackUnwinder::resume_curr_frame() {
    unw_cursor_t cursor = stack_trace[user_stack_trace_idx];
    unw_resume(&cursor);
}

void StackUnwinder::restart_curr_frame() {
    if (user_stack_trace_idx < stack_trace.size() - 1) {
//        user_stack_trace_idx++;
        unw_cursor_t cursor = stack_trace[user_stack_trace_idx + 1];
        unw_word_t pc;
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        unw_set_reg(&cursor, UNW_REG_IP, pc - 5);
//        unw_proc_info_t pip;
//        unw_get_proc_info(&cursor, &pip);
//        unw_set_reg(&cursor, UNW_REG_IP, pip.start_ip);
        unw_resume(&cursor);
    }
}

#define TMP_SYMBOL_HOLDER_SIZE 1024
char *StackUnwinder::get_curr_funcname() {
    if (!tmp_symbol_holder) {
        tmp_symbol_holder = (char *)malloc(TMP_SYMBOL_HOLDER_SIZE);
    }
    unw_word_t offset;
    unw_cursor_t cursor = stack_trace[user_stack_trace_idx];
    if (unw_get_proc_name(&cursor, tmp_symbol_holder, TMP_SYMBOL_HOLDER_SIZE, &offset) == 0) {
        return tmp_symbol_holder;
    }
    return NULL;
}

void *StackUnwinder::get_func_start_addr() {
    if (user_stack_trace_idx < stack_trace.size()) {
        unw_cursor_t cursor = stack_trace[user_stack_trace_idx];
//        unw_word_t pc;
        unw_proc_info_t pip;
        unw_get_proc_info(&cursor, &pip);
//        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        return (void *)(pip.start_ip);
    }
    return NULL;
}

void *StackUnwinder::get_func_call_addr() {
    if (user_stack_trace_idx < stack_trace.size() - 1) {
        unw_cursor_t cursor = stack_trace[user_stack_trace_idx + 1];
        unw_word_t pc;
//        unw_proc_info_t pip;
//        unw_get_proc_info(&cursor, &pip);
        unw_get_reg(&cursor, UNW_REG_IP, &pc);
        return (void *)(pc - 5);
    }
    return NULL;
}

void StackUnwinder::go_to_frame(int idx) {
    if (idx < 0) {
        user_stack_trace_idx = 0;
    } else if (idx >= stack_trace.size()) {
        user_stack_trace_idx = stack_trace.size() - 1;
    } else {
        user_stack_trace_idx = idx;
    }
}

