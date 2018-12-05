/* Code sample: manual setting of a breakpoint, using a ptrace*/
/**
 * Some Notes for MnCu:
 * 
 *  x86 is little-endian byte order
 *      - starting from least significant byte
 * 
 *  PTRACE-PEEKDATA / PTRACE-PEEKTEXT
 *      - reads the data/code section of the child
 *      - Linux does not have seperate text and data address space, same virtual address
 *        space
 * 
 *  PTRACE-PEEKUSER
 *      - reads the contents of the child's USER area which holds contents of registers
 *        and other info
 *      - the component "regs" can be read
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>


/* Run a target process in tracing mode by exec()-ing the given program name.*/
void run_target(const char* programname){

    fprintf(stdout, "start running target\n");

    /* Allow tracing of this process */
    if(ptrace(PTRACE_TRACEME, 0, 0, 0) < 0){
        perror("ptrace");
        return;
    }

    /* Replace this process's image with given program */
    execl(programname, programname, (char *)NULL);
}



void run_debugger(pid_t child_pid){
    int wait_status;
    struct user_regs_struct regs;

    fprintf(stdout, "start debugger\n");

    wait(&wait_status);

    /* Obtain and show child's instruction pointer */
    ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    // reg.rip and rax instead of ip and eax
    printf("Child started.rip = 0x%llx\n", regs.rip); 
    /* Look at the word at the address we're interested in */
    unsigned long long int addr = 0x4005a8;
    unsigned long long int data = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)addr, 0);
    printf("Original data at 0x%llx: 0x%llx\n", addr, data);

    /* Write the trap instruction 'int 3' into the address */
    unsigned long long int data_with_trap = (data & 0xFFFFFFFFFFFFFF00) | 0xCC;
    //printf("data_with_trap is 0x%llx\n", data_with_trap); // debug
    ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data_with_trap);

    /* See what's there again... */
    unsigned long long int readback_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, 0);
    printf("After trap, data at 0x%llx: 0x%llx\n", addr, readback_data);

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
    printf("Child stopped at rip = 0x%llx\n", regs.rip);

    /* Remove the breakpoint by restoring the previous data
    ** at the target address, and unwind the RIP back by 1 to 
    ** let the CPU execute the original instruction that was 
    ** there.
    */
    ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data);
    regs.rip -= 1; // rip contains the address of the next instruction
    unsigned long long int another_readback_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, 0);
    printf("After trap, data at 0x%llx: 0x%llx\n", addr, another_readback_data);
    //printf("where is regs.rip? It is located at 0x%llx\n", addr);
    ptrace(PTRACE_SETREGS, child_pid, 0, &regs);

     /* Obtain and show child's instruction pointer */
    ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
    // reg.rip and rax instead of ip and eax
    printf("Child started.rip = 0x%llx\n", regs.rip); 
    /* Look at the word at the address we're interested in */
    addr = 0x4005a8;
    data = ptrace(PTRACE_PEEKTEXT, child_pid, (void *)addr, 0);
    printf("Original data at 0x%llx: 0x%llx\n", addr, data);

    /* Write the trap instruction 'int 3' into the address */
    data_with_trap = (data & 0xFFFFFFFFFFFFFF00) | 0xCC;
    //printf("data_with_trap is 0x%llx\n", data_with_trap); // debug
    ptrace(PTRACE_POKETEXT, child_pid, (void*)addr, (void*)data_with_trap);

    /* See what's there again... */
    readback_data = ptrace(PTRACE_PEEKTEXT, child_pid, (void*)addr, 0);
    printf("After trap, data at 0x%llx: 0x%llx\n", addr, readback_data);   


    /* The child can continue running now */
    ptrace(PTRACE_CONT, child_pid, 0, 0);

    wait(&wait_status);


    if (WIFEXITED(wait_status)) {
        printf("Child exited\n");
    } 
    else {
        /**
         * Notes for MnCu:
         *      WSTOPSIG(wait_status) return the number of signal
         *      strsignal returns a string describing the signal number passed in the argument sig     
        */
        printf("Child got a signal: %s\n", strsignal(WSTOPSIG(wait_status)));        
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
        exit(EXIT_FAILURE);
    }


    return 0;
}
