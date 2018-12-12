//
// Created by WangJingjin on 2018-12-11.
//

#include "FunctionInjector.h"

FunctionInjector::FunctionInjector(pid_t pid, char *tempdir) : target(pid), srcFilePath(NULL), tempdir(tempdir), compiledFunctions() {
    init_target_useful_func_addrs(target, &this->func_addrs);
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
    for (pair<const string, func_info *> &elem : compiledFunctions) {
        delete elem.second;
    }
}

void FunctionInjector::assign_source(char *srcFilePath) {
    if (!srcFilePath) {
        return;
    }
    if (this->srcFilePath != NULL) {
        free(this->srcFilePath);
    }
    compiledFunctions.clear();
    this->srcFilePath = strdup(srcFilePath);
}

char *FunctionInjector::compile_func(char *funcname) {
    char *shared_obj_path = compile_func_in_file(srcFilePath, funcname, tempdir, &linker_flags);
    printf("shared_obj_path: %s\n", shared_obj_path);
    char *shared_obj_real_path = realpath(shared_obj_path, NULL);
    printf("shared_obj_real_path: %s\n", shared_obj_real_path);
    free(shared_obj_path);

    func_info *info = (func_info *)malloc(sizeof(func_info));
    info->shared_obj_path = shared_obj_real_path;
    info->handle = NULL;

    compiledFunctions.emplace(funcname, info);
    return shared_obj_real_path;
}

int FunctionInjector::inject_func(char *funcname, inject_type type) {
    string lib_name(funcname);
    auto lib_info = compiledFunctions.find(lib_name);
    if (lib_info == compiledFunctions.end()) {
        return 1; //Please compile it first.
    }

    func_info *info = lib_info->second;
    symaddr_t targetFuncAddr = target_exe->bin_dlsym(funcname);

    ptrace_attach(target);
    if (info->handle) {
        int ret = pdlclose(target, info->handle, func_addrs.targetDlcloseAddr);
        if (ret == 1) {
            return 2; //pdclose error.
        }
    }
    void *handle = NULL;
    if (type == DEBUG) {
        handle = pdlopen(target, info->shared_obj_path, funcname, func_addrs.targetMallocAddr,
                func_addrs.targetDlopenAddr, func_addrs.targetFreeAddr, targetFuncAddr,
                strlen(info->shared_obj_path) + 1, 1);
    } else if (type == RUNNING) {
        handle = pdlopen(target, info->shared_obj_path, funcname, func_addrs.targetMallocAddr,
                func_addrs.targetDlopenAddr, func_addrs.targetFreeAddr, targetFuncAddr,
                strlen(info->shared_obj_path) + 1, 2);
    }
    ptrace_detach(target);

    if (handle == NULL) {
        return 3; //pdlopen error.
    }

    lib_info->second->handle = handle;
    return 0;
}