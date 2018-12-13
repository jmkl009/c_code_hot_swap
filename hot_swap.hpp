//
// Created by WangJingjin on 2018/12/9.
//

#ifndef PROJECT_HOT_SWAP_H
#define PROJECT_HOT_SWAP_H
#include "cparser.hpp"
#include "libs/bin_dlsym.hpp"
#include "libs/ptrace.h"
#include "libs/utils.h"
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>

using std::vector;
using std::string;

typedef struct {
    long targetMallocAddr;
    long targetFreeAddr;
    long targetDlopenAddr;
    long targetDlcloseAddr;
} TargetUsefulFuncAddrs;

/**
 *
 * Call dlopen in the target process.
 *
 * @param target the pid of the target process
 * @param shared_obj_real_path the absolute path to the shared library for dlopen
 * @param funcname the name of the function contained in the shared library (This argument
 * is here for the convenience of implementation)
 * @param targetMallocAddr the address of malloc() in the target process's virtual memory
 * @param targetDlopenAddr the address of dlopen() in the target process's virtual memory
 * @param targetFreeAddr the address of free() in the target process's virtual memory
 * @param targetFuncAddr the symaddr_t struct of the target function to replace in the target process's virtual memory
 * @param libPathLength the length of shared_obj_real_path (again, for the convenience of implementation)
 * @param inject_offset for a running process, this should be 2, for a stopped process, this should be 1.
 * @return the handle returned by dlopen
 */
void *pdlopen(pid_t target, char *shared_obj_real_path, char *funcname, long targetMallocAddr,
              long targetDlopenAddr, long targetFreeAddr, symaddr_t targetFuncAddr, int libPathLength, int inject_offset);

/**
 *
 * Call dlclose in the target process.
 *
 * @param target the pid of the target process
 * @param targetLibHandle the handle returned previously by pdlopen()
 * @param targetDlcloseAddr the address of dlclose() in the target process's virtual memory
 * @return 0 for sucess, error otherwise.
 */
int pdlclose(pid_t target, void *targetLibHandle, long targetDlcloseAddr);

/**
 *
 * Use gcc to compile a function in the source file
 *
 * @param srcFilePath the path of the source file
 * @param funcname the name of the function to be compiled
 * @param tmpDirPath the path of the temporary directory to store the compiled shared library
 * @param linker_flags the linker flags to pass to the compiler
 * @return
 */
char *compile_func_in_file(char *srcFilePath, char *funcname, char *tmpDirPath, vector<string> *linker_flags);

/**
 *
 * Find out the relevant function addresses for pdlopen() and pdlclose()
 * in the target process's virtual mempry
 *
 * @param target the pid of the target process
 * @param addrs the struct to store the addresses
 */
void init_target_useful_func_addrs(pid_t target, TargetUsefulFuncAddrs* addrs);

#endif //PROJECT_HOT_SWAP_H
