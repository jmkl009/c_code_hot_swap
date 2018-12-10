//
// Created by WangJingjin on 2018/12/9.
//

#ifndef PROJECT_HOT_SWAP_H
#define PROJECT_HOT_SWAP_H
#include "cparser.h"
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    long targetMallocAddr;
    long targetFreeAddr;
    long targetDlopenAddr;
    long targetDlcloseAddr;
} TargetUsefulFuncAddrs;

void *pdlopen(pid_t target, char *shared_obj_real_path, char *funcname, long targetMallocAddr,
              long targetDlopenAddr, long targetFreeAddr, int libPathLength);
int pdlclose(pid_t target, void *targetLibHandle, long targetDlcloseAddr);
char *compile_func_in_file(char *srcFilePath, char *funcname, char *tmpDirPath, char **libs, unsigned nlibs);

#endif //PROJECT_HOT_SWAP_H
