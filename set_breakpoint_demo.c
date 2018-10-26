/* Code sample: manual setting of a breakpoint, using a ptrace*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>



/* Run a target process in tracing mode by exec()-ing the given program name.*/
void run_target(const char* programname){

    printf(stdout, "start running target");

    /* Allow tracing of this process */
    if(ptrace(PT_TRACE_ME, 0, 0, 0) < 0){
        perror("ptrace");
        return;
    }

    /* Replace this process's image with given program */
    execl(programname, programname, 0);
}



void run_debugger(pid_t child_pid){
    int wait_status;
    struct user_regs_struct regs;

    printf(stdout, "start debugger\n");

    wait(&wait_status);

    /* Obtain and show child's instruction pointer */
    ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    // reg.rip and rax instead of eip and eax
    printf("Child started. EIP = 0x%08x\n", reg.eip); 
    /* Look at the word at the address we're interested in */
    unsigned addr = 0x8048096;
    unsigned data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, 0);
    printf("Original data at 0x%08x: 0x%08x\n", addr, data);

    /* Write the trap instruction 'int 3' into the address */
    unsigned data_with_trap = (data & 0xFFFFFF00) | 0xCC;
    ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data_with_trap);

    /* See what's there again... */
    unsigned readback_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, 0);
    printf("After trap, data at 0x%08x: 0x%08x\n", addr, readback_data);

    /* Let the child run to the breakpoint and wait for it to
    ** reach it 
    */
    ptrace(PTRACE_CONT, child_pid, 0, 0);

    wait(&wait_status);
    if (WIFSTOPPED(wait_status)) {
        printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));
    }
    else {
        perror("wait");
        return;
    }

    /* See where the child is now */
    ptrace(PTRACE_GETREGS, child_pid, 0, &regs);
    printf("Child stopped at EIP = 0x%08x\n", regs.eip);

    /* Remove the breakpoint by restoring the previous data
    ** at the target address, and unwind the EIP back by 1 to 
    ** let the CPU execute the original instruction that was 
    ** there.
    */
    ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data);
    regs.eip -= 1; // EIP contains the address of the next instruction
    ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

    /* The child can continue running now */
    ptrace(PTRACE_CONT, child_pid, 0, 0);

    wait(&wait_status);

    if (WIFEXITED(wait_status)) {
        printf("Child exited\n");
    } 
    else {
        printf("Unexpected signal\n");
    }

}

int main(int argc, char** argv){

    pid_t child_pid;

    if(argc < 2){
        fprintf(stderr, "Expected a program name as argument\n");
    }

    child_pid = fork();
    if(child_pid > 0){
        // Parent

        run_debugger(child_pid);
    }else if(child_pid == 0){
        // Child process
        run_target(argv[1]);
    }else{
        perror("fork failed\n");
        return EXIT_FAILURE;
    }


    return 0;
}