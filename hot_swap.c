//
// Created by william on 04/11/18.
//
#include "libs/bin_dlsym.c"
#include "libs/utils.c"
#include "libs/ptrace.c"
#include "cparser.h"
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FUNC_NAME_MAX (1024)

/*
 * injectSharedLibrary()
 *
 * This is the code that will actually be injected into the target process.
 * This code is responsible for loading the shared library into the target
 * process' address space.  First, it calls malloc() to allocate a buffer to
 * hold the filename of the library to be loaded. Then, it calls
 * __libc_dlopen_mode(), libc's implementation of dlopen(), to load the desired
 * shared library. Finally, it calls free() to free the buffer containing the
 * library name. Each time it needs to give control back to the injector
 * process, it breaks back in by executing an "int $3" instruction. See the
 * comments below for more details on how this works.
 *
 */

void injectSharedLibrary(long mallocaddr, long freeaddr, long dlopenaddr)
{
    // here are the assumptions I'm making about what data will be located
    // where at the time the target executes this code:
    //
    //   rdi = address of malloc() in target process
    //   rsi = address of free() in target process
    //   rdx = address of __libc_dlopen_mode() in target process
    //   rcx = size of the path to the shared library we want to load

    // save addresses of free() and __libc_dlopen_mode() on the stack for later use
    asm(
    // rsi is going to contain the address of free(). it's going to get wiped
    // out by the call to malloc(), so save it on the stack for later
    "push %rsi \n"
    // same thing for rdx, which will contain the address of _dl_open()
    "push %rdx"
    );

    // call malloc() from within the target process
    asm(
    // save previous value of r9, because we're going to use it to call malloc()
    "push %r9 \n"
    // now move the address of malloc() into r9
    "mov %rdi,%r9 \n"
    // choose the amount of memory to allocate with malloc() based on the size
    // of the path to the shared library passed via rcx
    "mov %rcx,%rdi \n"
    // now call r9; malloc()
    "callq *%r9 \n"
    // after returning from malloc(), pop the previous value of r9 off the stack
    "pop %r9 \n"
    // break in so that we can see what malloc() returned
    "int $3"
    );

    // call __libc_dlopen_mode() to load the shared library
    asm(
    // get the address of __libc_dlopen_mode() off of the stack so we can call it
    "pop %rdx \n"
    // as before, save the previous value of r9 on the stack
    "push %r9 \n"
    // copy the address of __libc_dlopen_mode() into r9
    "mov %rdx,%r9 \n"
    // 1st argument to __libc_dlopen_mode(): filename = the address of the buffer returned by malloc()
    "mov %rax,%rdi \n"
    // 2nd argument to __libc_dlopen_mode(): flag = RTLD_LAZY
    "movabs $1,%rsi \n"
    // call __libc_dlopen_mode()
    "callq *%r9 \n"
    // restore old r9 value
    "pop %r9 \n"
    // break in so that we can see what __libc_dlopen_mode() returned
    "int $3"
    );

    // call free() to free the buffer we allocated earlier.
    //
    // Note: I found that if you put a nonzero value in r9, free() seems to
    // interpret that as an address to be freed, even though it's only
    // supposed to take one argument. As a result, I had to call it using a
    // register that's not used as part of the x64 calling convention. I
    // chose rbx.
    asm(
    // at this point, rax should still contain our malloc()d buffer from earlier.
    // we're going to free it, so move rax into rdi to make it the first argument to free().
    "mov %rax,%rdi \n"
    // pop rsi so that we can get the address to free(), which we pushed onto the stack a while ago.
    "pop %rsi \n"
    // save previous rbx value
    "push %rbx \n"
    // load the address of free() into rbx
    "mov %rsi,%rbx \n"
    // zero out rsi, because free() might think that it contains something that should be freed
    "xor %rsi,%rsi \n"
    // break in so that we can check out the arguments right before making the call
    "int $3 \n"
    // call free()
    "callq *%rbx \n"
    // restore previous rbx value
    "pop %rbx"
    );

    // we already overwrote the RET instruction at the end of this function
    // with an INT 3, so at this point the injector will regain control of
    // the target's execution.
}

/*
 * injectSharedLibrary_end()
 *
 * This function's only purpose is to be contiguous to injectSharedLibrary(),
 * so that we can use its address to more precisely figure out how long
 * injectSharedLibrary() is.
 *
 */

void injectSharedLibrary_end()
{
}

//TODO: add dlclose functionality so that dynamically linked functions do not accumulate.
int main(int argc, char* argv[]) {
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
    char * srcFilename;
    if (argc >= 5) {
        strcpy(srcFilePath, argv[4]);
        char * file_command = argv[3];
        if (!strcmp(file_command, "-f")) {
            char * curstr = strtok(argv[4], "/");
            char * prevstr = NULL;
            while (curstr != NULL) {
                prevstr = curstr;
                curstr = strtok(NULL, "/");
            }
            srcFilename = prevstr;
        } else {
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
        char * curstr = strtok(buff, "/");
        char * prevstr = NULL;
        while (curstr != NULL) {
            prevstr = curstr;
            curstr = strtok(NULL, "/");
        }
        srcFilename = prevstr;
    }

    printf("WARNING: Please make sure that the executable "
           "was compiled with flag -rdynamic, "
           "or else the code injection may not work "
           "if you call other functions in your modified ones.\n");

    printf("Please enter the function to replace: \n");
    char funcname[FUNC_NAME_MAX];
    if (scanf("%s", funcname) == 0) {
        fprintf(stderr, "Scan error.");
        exit(1);
    }

    printf("Isolating function...\n");
    int status = mkdir("./tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
//    if (status == -1) {
//        fprintf(stderr, "Making directory failed\n");
//        exit(1);
//    }

    char tmpFilePath[PATH_MAX];
    sprintf(tmpFilePath, "./tmp/%s-%s.c", srcFilename, funcname);
    int isolate_result = isolateFunction(srcFilePath, funcname, tmpFilePath);
    if (isolate_result == 0) {
        fprintf(stderr, "Function not found.\n");
        exit(1);
    } else if (isolate_result == -1) {
        fprintf(stderr, "File error.\n");
        exit(1);
    }

    //Shared object name
    size_t tmpFilePath_len = strlen(tmpFilePath);
    char tmpSharedObjPath[tmpFilePath_len + 2];

    sprintf(tmpSharedObjPath, "./tmp/%s-%s.so", srcFilename, funcname);

    fprintf(stderr, "%s\n%s\n", tmpSharedObjPath, tmpFilePath);
    //Compile shared object
    printf("Compiling function...\n");
    pid_t pid = fork();
    if (!pid) { //Child
        execlp("gcc", "gcc", "-shared", "-o", tmpSharedObjPath, "-fPIC", tmpFilePath, NULL);
        exit(-1);
    } else { //Parent
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) == 0) {
            printf("Successfully compiled.\n");
        } else {
            fprintf(stderr, "Compilation failed. Exit status:%d\n", WEXITSTATUS(status));
            exit(1);
        }
    }

    char *shared_obj_real_path = realpath(tmpSharedObjPath, NULL);
    int libPathLength = strlen(shared_obj_real_path) + 1;

    printf("Preparing to inject function...\n");


    int mypid = getpid();
    long mylibcaddr = getlibcaddr(mypid);

    // find the addresses of the syscalls that we'd like to use inside the
    // target, as loaded inside THIS process (i.e. NOT the target process)
    void* self = dlopen("libc.so.6", RTLD_LAZY);
    long mallocAddr = dlsym(self, "malloc");
    long freeAddr = dlsym(self, "free");
    long dlopenAddr = dlsym(self, "__libc_dlopen_mode");
    dlclose(self);

    // use the base address of libc to calculate offsets for the syscalls
    // we want to use
    long mallocOffset = mallocAddr - mylibcaddr;
    long freeOffset = freeAddr - mylibcaddr;
    long dlopenOffset = dlopenAddr - mylibcaddr;


    // get the target process' libc address and use it to find the
    // addresses of the syscalls we want to use inside the target process
    long targetLibcAddr = getlibcaddr(target);
    long targetMallocAddr = targetLibcAddr + mallocOffset;
    long targetFreeAddr = targetLibcAddr + freeOffset;
    long targetDlopenAddr = targetLibcAddr + dlopenOffset;

    struct user_regs_struct oldregs, regs;
    memset(&oldregs, 0, sizeof(struct user_regs_struct));
    memset(&regs, 0, sizeof(struct user_regs_struct));

    ptrace_attach(target);

    ptrace_getregs(target, &oldregs);
    memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

    // find a good address to copy code to
    long addr = freespaceaddr(target) + sizeof(long);

    // now that we have an address to copy code to, set the target's rip to
    // it. we have to advance by 2 bytes here because rip gets incremented
    // by the size of the current instruction, and the instruction at the
    // start of the function to inject always happens to be 2 bytes long.
    regs.rip = addr + 2;

    // pass arguments to my function injectSharedLibrary() by loading them
    // into the right registers. note that this will definitely only work
    // on x64, because it relies on the x64 calling convention, in which
    // arguments are passed via registers rdi, rsi, rdx, rcx, r8, and r9.
    // see comments in injectSharedLibrary() for more details.
    regs.rdi = targetMallocAddr;
    regs.rsi = targetFreeAddr;
    regs.rdx = targetDlopenAddr;
    regs.rcx = libPathLength;
    ptrace_setregs(target, &regs);

    // figure out the size of injectSharedLibrary() so we know how big of a buffer to allocate.
    size_t injectSharedLibrary_size = (intptr_t)injectSharedLibrary_end - (intptr_t)injectSharedLibrary;

    // also figure out where the RET instruction at the end of
    // injectSharedLibrary() lies so that we can overwrite it with an INT 3
    // in order to break back into the target process. note that on x64,
    // gcc and clang both force function addresses to be word-aligned,
    // which means that functions are padded with NOPs. as a result, even
    // though we've found the length of the function, it is very likely
    // padded with NOPs, so we need to actually search to find the RET.
    intptr_t injectSharedLibrary_ret = (intptr_t)findRet(injectSharedLibrary_end) - (intptr_t)injectSharedLibrary;

    // back up whatever data used to be at the address we want to modify.
    char* backup = malloc(injectSharedLibrary_size * sizeof(char));
    ptrace_read(target, addr, backup, injectSharedLibrary_size);

    // set up a buffer to hold the code we're going to inject into the
    // target process.
    char* newcode = malloc(injectSharedLibrary_size * sizeof(char));
    memset(newcode, 0, injectSharedLibrary_size * sizeof(char));

    // copy the code of injectSharedLibrary() to a buffer.
    memcpy(newcode, injectSharedLibrary, injectSharedLibrary_size - 1);
    // overwrite the RET instruction with an INT 3.
    newcode[injectSharedLibrary_ret] = INTEL_INT3_INSTRUCTION;

    // copy injectSharedLibrary()'s code to the target address inside the
    // target process' address space.
    ptrace_write(target, addr, newcode, injectSharedLibrary_size);

    // now that the new code is in place, let the target run our injected
    // code.
    ptrace_cont(target);

    // at this point, the target should have run malloc(). check its return
    // value to see if it succeeded, and bail out cleanly if it didn't.
    struct user_regs_struct malloc_regs;
    memset(&malloc_regs, 0, sizeof(struct user_regs_struct));
    ptrace_getregs(target, &malloc_regs);
    unsigned long long targetBuf = malloc_regs.rax;
    if(targetBuf == 0)
    {
        fprintf(stderr, "malloc() failed to allocate memory\n");
        restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
        free(backup);
        free(newcode);
        return 1;
    }

    // if we get here, then malloc likely succeeded, so now we need to copy
    // the path to the shared library we want to inject into the buffer
    // that the target process just malloc'd. this is needed so that it can
    // be passed as an argument to __libc_dlopen_mode later on.

    // read the current value of rax, which contains malloc's return value,
    // and copy the name of our shared library to that address inside the
    // target process.
    ptrace_write(target, targetBuf, shared_obj_real_path, libPathLength);

    // continue the target's execution again in order to call
    // __libc_dlopen_mode.
    ptrace_cont(target);

    // check out what the registers look like after calling dlopen.
    struct user_regs_struct dlopen_regs;
    memset(&dlopen_regs, 0, sizeof(struct user_regs_struct));
    ptrace_getregs(target, &dlopen_regs);
    unsigned long long libAddr = dlopen_regs.rax;

    // if rax is 0 here, then __libc_dlopen_mode failed, and we should bail
    // out cleanly.
    if(libAddr == 0)
    {
        fprintf(stderr, "__libc_dlopen_mode() failed to load %s\n", shared_obj_real_path);
        restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
        free(backup);
        free(newcode);
        return 1;
    }

    // now check /proc/pid/maps to see whether injection was successful.
    void * loaded_func_addr = checkloaded(target, shared_obj_real_path, funcname);
    if(loaded_func_addr) {
        printf("\"%s\" Successfully injected.\n", shared_obj_real_path);
        printf("Resolving symbols...\n", shared_obj_real_path);
//        memcpy(old_func, "\x48\xb8", 2 * sizeof(char));
//        memcpy(old_func + 0x02, &new_func, sizeof(void *));
//        memcpy(old_func + 0x0a, "\xff\xe0", 2 * sizeof(char));
        char buf[4 + sizeof(void *)];
        memcpy(buf, "\x48\xb8", 2 * sizeof(char));
        memcpy(buf + 0x02, &loaded_func_addr, sizeof(void *));
        memcpy(buf + 0x0a, "\xff\xe0", 2 * sizeof(char));

        char exename[32];
        sprintf(exename, "/proc/%d/exe", target);
//        fprintf(stderr, "exename: %s\nfuncname: %s\n", exename, funcname);
        void *orig_func_addr = bin_dlsym(exename, funcname) + addr - sizeof(long);

        ptrace_write(target, orig_func_addr, buf, 4 + sizeof(void *));
        printf("All complete.\n", shared_obj_real_path);
    }
    else {
        fprintf(stderr, "could not inject \"%s\"\n", shared_obj_real_path);
    }


    // as a courtesy, free the buffer that we allocated inside the target
    // process. we don't really care whether this succeeds, so don't
    // bother checking the return value.
    ptrace_cont(target);

    // at this point, if everything went according to plan, we've loaded
    // the shared library inside the target process, so we're done. restore
    // the old state and detach from the target.
    restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
    free(backup);
    free(newcode);
    free(shared_obj_real_path);
}