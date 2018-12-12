#ifdef ARM
	#define REG_TYPE user_regs
#else
	#define REG_TYPE user_regs_struct
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
#include <time.h>
#include "utils.h"

void ptrace_attach(pid_t target);
void ptrace_detach(pid_t target);
void ptrace_seize(pid_t target);
//void ptrace_stop(pid_t target);
void ptrace_restart(pid_t target, int signal);
void ptrace_singlestep(pid_t target);
void ptrace_getregs(pid_t target, struct REG_TYPE* regs);
void ptrace_cont(pid_t target);
void ptrace_setregs(pid_t target, struct REG_TYPE* regs);
siginfo_t ptrace_getsiginfo(pid_t target);
void ptrace_read(int pid, unsigned long addr, void *vptr, int len);
void ptrace_write(int pid, unsigned long addr, void *vptr, int len);
void checktargetsig(int pid);
//void restoreStateAndDetach(pid_t target, unsigned long addr, void* backup, int datasize, struct REG_TYPE oldregs);
void restoreStates(pid_t target, unsigned long addr, void* backup, int datasize, struct REG_TYPE oldregs);
void ptrace_letgo(pid_t target);
