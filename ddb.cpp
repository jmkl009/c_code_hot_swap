//
// Created by WangJingjin on 2018/12/10.
//

#include "hot_swap.hpp"
#include <execinfo.h>
#include "libs/bin_dlsym.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <sys/ptrace.h>
#include <stdio.h>

#include <libunwind-ptrace.h>
#include "StackUnwinder.h"
#include "FunctionInjector.h"
#include "libs/simplereader.h"
#include "libs/ptrace.h"

void print_stack_trace(pid_t target) {
    int status;
    waitpid(target, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("terminated signal: %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("stopped signal: %d\n", WSTOPSIG(status));
    }
    StackUnwinder unwinder(target);
    unwinder.print_stack_trace();
    unwinder.up_stack();
    unwinder.restart_curr_frame();
}

#define TEMP_DIR "./tmp"

typedef enum {
    BACKTRACE,
    PRINT_CURR_FRAME,
    UP_STACK,
    DOWN_STACK,
    RESUME_EXE,
    RESTART_FUNC,
    SHOW_REGS,
    INJECT_FUNC,
    UNKNOWN
} command_t;

typedef struct {
    long orig_code;
    struct user_regs_struct regs;
} checkpoint_t;

int global_pid;
int at_trap;
FunctionInjector *injector;
unordered_map<void *, checkpoint_t> break_point_map;
#define EXE_MODE 0
#define PID_MODE 1

char * get_exe_name(int pid) {
//    char cmdline[255], path[512], *p;
//    int fd;
//    snprintf(cmdline, 255, "/proc/%d/cmdline", pid);
//    if ((fd = open(cmdline, O_RDONLY)) < 0) {
//        perror("open");
//        exit(-1);
//    }
//    if (read(fd, path, 512) < 0) {
//        perror("read");
//        exit(-1);
//    }
//    if ((p = strdup(path)) == NULL) {
//        perror("strdup");
//        exit(-1);
//    }
//    return p;
    char exename[32];
    sprintf(exename, "/proc/%d/exe", pid);
    return strdup(exename);
}

void restore_breakpoints() {
    if (at_trap) { //rewind the instruction back by 1 if it trapped
        struct user_regs_struct regs;
        ptrace_getregs(global_pid, &regs);
        regs.rip --;
        ptrace_setregs(global_pid, &regs);
    }
    for (pair<void * const, checkpoint_t> &breakpoint: break_point_map) {
//        printf("restoring breakpoint at address:%p\n", breakpoint.first);
        ptrace_write(global_pid, (unsigned long)breakpoint.first, &breakpoint.second.orig_code, sizeof(long));
    }
}

void sighandler(int sig) {
    printf("Caught SIGINT: Detaching from %d\n", global_pid);
    restore_breakpoints();
    ptrace_detach(global_pid);
    exit(0);
}

void print_regs(handle_t *h) {
    printf("\nExecutable %s (pid: %d) has hit breakpoint 0x%lx\n", h->exec, global_pid, (unsigned long)h->pt_reg.rip);
    printf("%%rcx: %llx\n%%rdx: %llx\n%%rbx: %llx\n"
           "%%rax: %llx\n%%rdi: %llx\n%%rsi: %llx\n"
           "%%r8: %llx\n%%r9: %llx\n%%r10: %llx\n"
           "%%r11: %llx\n%%r12 %llx\n%%r13 %llx\n"
           "%%r14: %llx\n%%r15: %llx\n%%rsp: %llx",
           h->pt_reg.rcx, h->pt_reg.rdx, h->pt_reg.rbx,
           h->pt_reg.rax, h->pt_reg.rdi, h->pt_reg.rsi,
           h->pt_reg.r8, h->pt_reg.r9, h->pt_reg.r10,
           h->pt_reg.r11, h->pt_reg.r12, h->pt_reg.r13,
           h->pt_reg.r14, h->pt_reg.r15, h->pt_reg.rsp);
}

command_t interpret_command(char *command, ssize_t len) {
    if (len > 1) {
        switch(tolower(command[0])) {
            case 'b':
                return BACKTRACE;
            case 'p':
                return PRINT_CURR_FRAME;
            case 'u':
                return UP_STACK;
            case 'd':
                return DOWN_STACK;
            case 'r':
                if (len < 4) {
                    break;
                }
                return command[3] == 'u' ? RESUME_EXE : RESTART_FUNC;
            case 's':
                return SHOW_REGS;
            case 'i':
                return INJECT_FUNC;
            default:
                return UNKNOWN;
        }
    }
    return UNKNOWN;
}

void handle_injection(StackUnwinder *unwinder) {
    char *funcname = unwinder->get_curr_funcname();
    if (funcname == NULL) {
        printf("No symbols found\n");
        return;
    }
    fprintf(stderr, "Are you sure to replace current function: %s (y/n) ", funcname);
    char *response = NULL;
    size_t len = 0;
    ssize_t ret = getline(&response, &len, stdin);
    if (ret == -1) {
        printf("\n");
        return;
    }
//    printf("freeing response\n");
//    free(response);

    if (response[0] == 'y' || response[0] == 'Y') {
        fprintf(stderr, "Please provide the source file path to the function: ");
        char *srcFilePath = NULL;
        size_t len = 0;
        ssize_t ret = getline(&srcFilePath, &len, stdin);
        if (ret == -1) {
            printf("\n");
            return;
        }
        srcFilePath[ret - 1] = '\0';
        injector->assign_source(srcFilePath);
        injector->compile_func(funcname);
        injector->inject_func_under_ptrace(funcname, STOPPED);

        free(srcFilePath);
    }
//    fprintf(stderr, "All done.");
    free(response);
    return;
}

void restart_curr_frame(StackUnwinder *unwinder) {
    printf("restart called\n");
    void *call_addr = unwinder->get_func_call_addr();
    auto checkpoint = break_point_map.find(call_addr);
    if (checkpoint == break_point_map.end()) { //No checkpoint
        unwinder->restart_curr_frame();
        return;
    }
    long inst;
    ptrace_read(global_pid, checkpoint->second.regs.rip, &inst, sizeof(long));
    printf("restart at instruction: %p : %lx", (void *)checkpoint->second.regs.rip, inst);
//    checkpoint->second.regs.rip = (unsigned long)unwinder->get_func_call_addr();
    ptrace_setregs(global_pid, &checkpoint->second.regs);

}

#define CLEANUP_AND_RETURN \
free(line);\
return;

void start_ui(handle_t *h, StackUnwinder *unwinder) {
//    printf("starting ui....\n");
    char *line = NULL;
    size_t cap = 0;
    while (1) {
//        printf("looping....\n");
        printf("\n(ddb) ");
        ssize_t nbytes_read = getline(&line, &cap, stdin);
        if (nbytes_read == -1) {
            printf("-1! breaking...\n");
            break;
        }
//        printf("Interpreting command\n");
        command_t command = interpret_command(line, nbytes_read);
//        bool keep_ui = true;
        switch (command) {
            case BACKTRACE:
                unwinder->print_stack_trace();
                break;
            case PRINT_CURR_FRAME:
                unwinder->print_curr_frame();
                break;
            case UP_STACK:
                unwinder->up_stack();
                unwinder->print_curr_frame();
                break;
            case DOWN_STACK:
                unwinder->down_stack();
                unwinder->print_curr_frame();
                break;
            case RESUME_EXE:
                unwinder->resume_curr_frame();
                CLEANUP_AND_RETURN
                break;
            case RESTART_FUNC:
                restart_curr_frame(unwinder);
                CLEANUP_AND_RETURN
                break;
            case SHOW_REGS:
                print_regs(h);
                break;
            case INJECT_FUNC:
                handle_injection(unwinder);
                break;
            default:
//                printf("unknown! breaking...\n");
                CLEANUP_AND_RETURN
                break;
        }
//        if (!keep_ui) {
//            break;
//        }
    }

    free(line);
}

void set_breakpoint_at_addr(void *addr, bool record_regs) {
// Read the 8 bytes at addr
    long orig;
    ptrace_read(global_pid, (unsigned long)addr, &orig, sizeof(long));

// set a break point
    long trap = (orig & ~0xff) | 0xcc;
    ptrace_write(global_pid, (unsigned long)addr, &trap, sizeof(long));

    checkpoint_t checkpoint;
    checkpoint.orig_code = orig;
    if (record_regs) {
        ptrace_getregs(global_pid, &checkpoint.regs);
        printf("registers recorded\n");
    }
    printf("breakpoint inserted at address: %p\n", addr);
    break_point_map.emplace(addr, checkpoint);
}

//Only intended to be called after a breakpoint hit
void reset_breakpoint(void *addr, handle_t *h, bool refresh_regs) {
    printf("resetting breakpoint at: %p\n", addr);
    auto backup = break_point_map.find(addr);
    if (backup == break_point_map.end()) {
        printf("no backup code found\n");
        return;
    }
    if (refresh_regs) {
        ptrace_getregs(global_pid, &backup->second.regs);
        backup->second.regs.rip--;
        printf("registers recorded\n");
    }

    long orig = backup->second.orig_code;
    ptrace_write(global_pid, (unsigned long)addr, &orig, sizeof(long));
//    h->pt_reg.rip = h->pt_reg.rip - 1;
    ptrace_setregs(global_pid, &h->pt_reg);
    ptrace_singlestep(global_pid);
    wait(NULL);
    long trap = (orig & ~0xff) | 0xcc;
    ptrace_write(global_pid, (unsigned long)addr, &trap, sizeof(long));
}

#define JUMP_TO_UI_AND_RETURN \
free(response);\
start_ui(h, unwinder);\
return;

void handle_segfault(handle_t *h, StackUnwinder *unwinder) {

    fprintf(stderr, "Would you like to insert a checkpoint (y/n) ");
    char *response = NULL;
    size_t cap = 0;
    ssize_t ret = getline(&response, &cap, stdin);
    if (ret == -1) {
//        printf("starting ret == -1....");
        JUMP_TO_UI_AND_RETURN
    }

    if (response[0] == 'y' || response[0] == 'Y') {
        unwinder->print_stack_trace();
        printf("Which function would you like to insert a checkpoint? ");
        ssize_t ret = getline(&response, &cap, stdin);
        if (ret == -1) {
//            printf("starting ret == -1 in res....");
            JUMP_TO_UI_AND_RETURN
        }
        int i = atoi(response);
        unwinder->go_to_frame(i);
        void *func_call_addr = unwinder->get_func_call_addr();
        void *func_start_addr = unwinder->get_func_start_addr();
        printf("function call addr: %p\n", func_call_addr);
        printf("function start addr: %p\n", func_start_addr);
        if (!func_call_addr) {
            printf("Address error.\n");
//            printf("starting ui call_addr = NULL....");
            JUMP_TO_UI_AND_RETURN
        }
        set_breakpoint_at_addr(func_call_addr, false);
        printf("Successfully set checkpoint.\n");
    }

    free(response);
//    printf("starting ui....");
    start_ui(h, unwinder);
}


int main(int argc, char **argv, char **envp) {

    int fd, c, mode = 0;
    handle_t h;
    memset(&h, 0, sizeof(h));
    struct stat st;
    long trap, orig;
    int status, pid;
    char *args[2];

    printf("Usage: %s [-nep <proc name>/<exe>/<pid>]\n", argv[0]);

    memset(&h, 0, sizeof(handle_t));
    while ((c = getopt(argc, argv, "n:p:e:f:")) != -1) {
        switch (c) {
            case 'n':
                pid = findProcessByName(optarg);
                h.exec = get_exe_name(pid);
                if (h.exec == NULL) {
                    printf("Unable to retrieve executable path for pid: %d\n",
                           pid);
                    exit(-1);
                }
                mode = PID_MODE;
                break;
            case 'p':
                pid = atoi(optarg);
                h.exec = get_exe_name(pid);
                if (h.exec == NULL) {
                    printf("Unable to retrieve executable path for pid: %d\n",
                           pid);
                    exit(-1);
                }
                mode = PID_MODE;
                break;
            case 'e':
                if ((h.exec = strdup(optarg)) == NULL) {
                    perror("strdup");
                    exit(-1);
                }
                mode = EXE_MODE;
                break;
            default:
                printf("Unknown option\n");
                exit(1);
        }
    }

    if (h.exec == NULL) {
        exit(1);
    }

    if (mode == EXE_MODE) {
        args[0] = h.exec;
        args[1] = NULL;
    }
    signal(SIGINT, sighandler);
    exe bin(h.exec);

    if (mode == EXE_MODE) {
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(-1);
        }
        if (pid == 0) {
            if (ptrace(PTRACE_TRACEME, pid, NULL, NULL) < 0) {
                perror("PTRACE_TRACEME");
                exit(-1);
            }
            execve(h.exec, args, envp);
            exit(0);
        }
    } else { // attach to the process 'pid'
        ptrace_attach(pid);
    }
    global_pid = pid;
    injector = new FunctionInjector(global_pid, TEMP_DIR);
    printf("Beginning analysis of pid: %d at %lx\n", pid, h.symaddr);

// Begin tracing execution
    while (1) {
        at_trap = 0;
        ptrace_letgo(pid);
        wait(&status);

/*
    * If we receive a SIGTRAP then we presumably hit a break
    * Point instruction. In which case we will print out the
    *current register state.
*/
        StackUnwinder unwinder(pid);
//        char *line = NULL;
//        size_t cap = 0;
//        char dummy[1024];
        char c;
        if (WIFSTOPPED(status)) {
            switch WSTOPSIG(status) {
                case SIGTRAP:
                    at_trap = 1;
                    ptrace_getregs(pid, &h.pt_reg);
                    h.pt_reg.rip--;
                    print_regs(&h);
                    printf("\nPlease hit any key to continue (r to record registers): ");
                    c = getchar();
                    getchar(); //Bug here, if entered more than 1 character.
//                    scanf("%s", dummy); //Read until a new line;
                    printf("c: %c\n", c);
                    if (c != 'r') {
                        reset_breakpoint((void *)h.pt_reg.rip, &h, false);
                    } else {
                        reset_breakpoint((void *)h.pt_reg.rip, &h, true);
                    }
//                    free(line);
                    break;
                case SIGSEGV:
                    ptrace_getregs(pid, &h.pt_reg);
                    printf("\nExecutable %s (pid: %d) received a SIGSEGV at 0x%lx\n", h.exec, pid, (unsigned long)h.pt_reg.rip);
                    unwinder.print_curr_frame();
                    handle_segfault(&h, &unwinder);
//                    start_ui(&h, &unwinder);
                    break;
                case SIGINT:
                    printf("\nExecutable %s (pid: %d) received a SIGINT at 0x%lx\n", h.exec, pid, (unsigned long)h.pt_reg.rip);
                    unwinder.print_curr_frame();
                    start_ui(&h, &unwinder);
                default:
                    printf("\nsignal: %s\n", strsignal(WSTOPSIG(status)));
                    start_ui(&h, &unwinder);
                    break;
            }
        }

        if (WIFEXITED(status)) {
            printf("Completed tracing pid: %d\n", pid);
            break;
        }
    }
}