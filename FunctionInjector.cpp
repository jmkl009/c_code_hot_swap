//
// Created by WangJingjin on 2018-12-11.
//

#include "FunctionInjector.h"

FunctionInjector::FunctionInjector(pid_t pid, char *tempdir) : target(pid), srcFilePath(NULL), tempdir(tempdir) {
    TargetUsefulFuncAddrs func_addrs;
    init_target_useful_func_addrs(target, &func_addrs);
    char exe_name_buf[256];
    sprintf(exe_name_buf, "/proc/%d/exe", target);
    target_exe = new exe(exe_name_buf);
    linker_flags = target_exe->lookup_linker_flags();
}

FunctionInjector::~FunctionInjector() {
    delete target_exe;
    if (srcFilePath != NULL) {
        free(srcFilePath);
    }
}

void FunctionInjector::assign_source(char *srcFilePath) {
    if (!srcFilePath) {
        return;
    }
    if (srcFilePath != NULL) {
        free(srcFilePath);
    }
    this->srcFilePath = strdup(srcFilePath);
}

int FunctionInjector::compile_func(char *funcname) {
    if (!compile_func_in_file(srcFilePath, funcname, tempdir, &linker_flags)) {
        return 1;
    }
    return 0;
}