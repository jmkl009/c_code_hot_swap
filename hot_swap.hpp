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

void *pdlopen(pid_t target, char *shared_obj_real_path, char *funcname, long targetMallocAddr,
              long targetDlopenAddr, long targetFreeAddr, symaddr_t targetFuncAddr, int libPathLength);
int pdlclose(pid_t target, void *targetLibHandle, long targetDlcloseAddr);
char *compile_func_in_file(char *srcFilePath, char *funcname, char *tmpDirPath, vector<string> *linker_flags);
void init_target_useful_func_addrs(pid_t target, TargetUsefulFuncAddrs* addrs);

#endif //PROJECT_HOT_SWAP_H
