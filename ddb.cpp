//
// Created by WangJingjin on 2018/12/10.
//

#include "hot_swap.hpp"
#include <unordered_map>
#include <execinfo.h>
#include "libs/bin_dlsym.hpp"

//#define UNW_LOCAL_ONLY
//#define UNW_REMOTE_ONLY
//#include <libunwind.h>

#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <stdlib.h>     /* For exit() */
#include <unistd.h>     /* For close() */
#include <stdio.h>
#include <errno.h>
//#include <libdwarf/dwarf.h>
//#include <libdwarf/libdwarf.h>

#include <sys/ptrace.h>
#include <stdio.h>
#include <stdlib.h>

#include <libunwind-ptrace.h>

//
//using std::unordered_map;
//
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
//}
//
//void show_backtrace (void) {
//    unw_cursor_t cursor; unw_context_t uc;
//    unw_word_t ip, sp;
//
//    unw_getcontext(&uc);
//    unw_init_local(&cursor, &uc);
//    while (unw_step(&cursor) > 0) {
//        unw_get_reg(&cursor, UNW_REG_IP, &ip);
//        unw_get_reg(&cursor, UNW_REG_SP, &sp);
//        printf ("ip = %lx, sp = %lx\n", (long) ip, (long) sp);
//    }
//}
//
//int main(int argc, char *argv[]) {
////    see_backtrace(0, 0, NULL);
////    exe bin("/home/william/introcs/NEAT/NEAT.cpython-36m-x86_64-linux-gnu.so");
////    vector<string> linker_flags = bin.lookup_linker_flags();
////    for (string &flag : linker_flags) {
////        std::cout << flag << std::endl;
////    }
////    pid_t target = findProcessByName("libgame.so");
////    unw_create_addr_space()
////    lookup_linker_flags();
//    show_backtrace();
//}
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
int resume (unw_addr_space_t as, unw_cursor_t *c, void *arg)
{
    struct UPT_info *ui = (struct UPT_info *)arg;
//    printf("hello\n");
    return 0;
}

void print_stack_trace(pid_t target) {
    int status;
    waitpid(target, &status, 0);
    if (WIFSIGNALED(status)) {
        printf("terminated signal: %d\n", WTERMSIG(status));
    } else if (WIFSTOPPED(status)) {
        printf("stopped signal: %d\n", WSTOPSIG(status));
    }
//    siginfo_t targetsig = ptrace_getsiginfo(target);
//    printf("sig: %s\n", strsignal(targetsig.si_signo));

    _UPT_accessors.resume = resume;

    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);

    void *context = _UPT_create(target);
    unw_cursor_t cursor;
    unw_cursor_t cursor_copy;
    if (unw_init_remote(&cursor, as, context) != 0) {
        printf("ERROR: cannot initialize cursor for remote unwinding\n");
        exit(1);
    }
    cursor_copy = cursor;


    char func[1024];
    unw_get_proc_name(&cursor, func, sizeof(func), NULL);
    do {
        unw_word_t offset, pc;
        char sym[1024];
        if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
            printf("ERROR: cannot read program counter\n");
            exit(1);
        }

        printf("0x%lx: ", pc);

        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
            printf("(%s+0x%lx)\n", sym, offset);
        else
            printf("-- no symbol name found\n");
    } while (unw_step(&cursor) > 0);

    if (WSTOPSIG(status) == SIGSEGV) {
//        printf("trying to resume...\n");
//
////        unw_word_t pc;
////        unw_get_reg(&cursor_copy, UNW_REG_IP, &pc);
////        printf("pc: 0x%lx\n", pc);
////        unw_proc_info_t proc_info;
////        unw_get_proc_info(&cursor_copy, &proc_info);
////        printf("start_ip: 0x%lx, end_ip: 0x%lx\n", proc_info.start_ip, proc_info.end_ip);
//
        unw_step(&cursor_copy);
        unw_step(&cursor_copy);
//
        unw_word_t pc;
        unw_get_reg(&cursor_copy, UNW_REG_IP, &pc);
//
//        pc -= 5;
        printf("pc: 0x%lx\n", pc);
        user_regs_struct regs;
        ptrace_getregs(target, &regs);
        printf("rip: 0x%lx\n", regs.rip);
////        unw_get_proc_info(&cursor_copy, &proc_info);
        unw_set_reg(&cursor_copy, UNW_REG_IP, pc - 5);
        unw_get_reg(&cursor_copy, UNW_REG_IP, &pc);
        printf("pc: 0x%lx\n", pc);
//
////        ptrace_write(target, pc, buf, 32);
//
////        pc += 5;
//        siginfo_t targetsig = ptrace_getsiginfo(target);
//        printf("sig: %s\n", strsignal(targetsig.si_signo));
//
//        ptrace(PTRACE_CONT, target, 0, SIGSTOP);
        unw_resume(&cursor_copy);
//        regs.rip = pc;
//        unw_get_reg(&cursor_copy, UNW_REG_SP, &pc);
//        regs.rsp = pc;
//        ptrace_setregs(target, &regs);
//        regs.rip += 4;
//        ptrace_setregs(target, &regs);
//        ptrace_singlestep(target);
//        ptrace_letgo(target);
////        ptrace_cont(target);
//        ptrace(PTRACE_LISTEN, target, 0, 0);
    }
    _UPT_destroy(context);
//    ptrace(PTRACE_CONT, target, 0, SIGSTOP);
}

#define TEMP_DIR "./tmp"
int main(int argc, char **argv)
{
//    if (argc != 2) {
//        printf("USAGE: unwind-pid <pid>\n");
//        exit(1);
//    }

    if(argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    char* command = argv[1];
    char* commandArg = argv[2];

    char* processName = NULL;
    pid_t target = 0;
    if (!strcmp(command, "-n")) {
        processName = commandArg;
        target = findProcessByName(processName);
        if(target == -1)
        {
            fprintf(stderr, "doesn't look like a process named \"%s\" is running right now\n", processName);
            return 1;
        }

        printf("targeting process \"%s\" with pid %d\n", processName, target);

    } else if(!strcmp(command, "-p")) {
        target = atoi(commandArg);
        printf("targeting process with pid %d\n", target);
    } else {
        usage(argv[0]);
        exit(1);
    }

    char srcFilePath[PATH_MAX];
    if (argc >= 5) {
        strcpy(srcFilePath, argv[4]);
        char * file_command = argv[3];
        if (strcmp(file_command, "-f")) {
            usage(argv[0]);
            exit(1);
        }
    } else {
        char buff[PATH_MAX];
        printf("Please enter the path of the source file: \n");
        if (scanf("%s", buff) == 0) {
            fprintf(stderr, "Please enter a valid source file name.\n");
            exit(1);
        }
        strcpy(srcFilePath, buff);
    }

    mkdir(TEMP_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    TargetUsefulFuncAddrs func_addrs;
    init_target_useful_func_addrs(target, &func_addrs);
    char exe_name_buf[256];
    sprintf(exe_name_buf, "/proc/%d/exe", target);
    exe bin(exe_name_buf);
    vector<string> linker_flags = bin.lookup_linker_flags();


//    ptrace_seize(target);
//    int status;
//    waitpid(target, &status, 0);
//    if (WIFSIGNALED(status)) {
//        printf("terminated signal: %d\n", WTERMSIG(status));
//    } else if (WIFSTOPPED(status)) {
//        printf("stopped signal: %d\n", WSTOPSIG(status));
//    }
    ptrace_seize(target);
    print_stack_trace(target);
//    ptrace_singlestep(target);
//    int status;
//    waitpid(target, &status, 0);
//    if (WIFSIGNALED(status)) {
//        printf("terminated signal: %d\n", WTERMSIG(status));
//    } else if (WIFSTOPPED(status)) {
//        printf("stopped signal: %d\n", WSTOPSIG(status));
//    }
//    ptrace_singlestep(target);
//        ptrace_attach(target);
//    if (ptrace(PTRACE_LISTEN, target, 0, 0) == -1) {
//        fprintf(stderr, "ptrace(PTRACE_LISTEN) failed\n");
//        exit(1);
//    }
//    ptrace_letgo(target);
//    ptrace_restart(target, NULL);

//    user_regs_struct regs;
//    ptrace_getregs(target, &regs);
//    printf("after incrementing, pc: 0x%lx\n", regs.rip);
//
    printf("Preparing to inject function...\n");
    printf("function: %s\n", "func3");
    symaddr_t targetFuncAddr = bin.bin_dlsym("func3");
    char *tmpSharedObjPath = compile_func_in_file(srcFilePath, "func3", TEMP_DIR, &linker_flags);
    char *shared_obj_real_path = realpath(tmpSharedObjPath, NULL);
    free(tmpSharedObjPath);
    int libPathLength = strlen(shared_obj_real_path) + 1;
    void *libAddr = pdlopen(target, shared_obj_real_path, "func3", func_addrs.targetMallocAddr, func_addrs.targetDlopenAddr,
                            func_addrs.targetFreeAddr, targetFuncAddr, libPathLength);
    free(shared_obj_real_path);

    if (!libAddr) {
        ptrace_detach(target);
        exit(1);
    }
//
    ptrace_detach(target);

//    _UPT_destroy(context);
    return 0;
}