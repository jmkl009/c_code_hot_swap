//
// Created by WangJingjin on 2018-12-11.
//

#ifndef C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
#define C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H

#include "hot_swap.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
using std::unordered_map;
using std::pair;

typedef struct func_info {
    char *shared_obj_path;
    void *handle;

    ~func_info() {
        if (shared_obj_path != NULL) {
            free(shared_obj_path);
        }
    }
} func_info;

typedef enum {
    STOPPED,
    RUNNING
} target_process_state;

class FunctionInjector {
private:
    pid_t target;
    char *tempdir;
    char *srcFilePath;
    exe *target_exe;
    TargetUsefulFuncAddrs func_addrs;
    vector<string> linker_flags;
    unordered_map<string, unordered_map<string, func_info*>> sources_to_compiled_functions;
    unordered_map<string, func_info*> *compiled_functions;

    void *inject_func_main(func_info *info, char *funcname, symaddr_t targetFuncAddr, target_process_state state);


public:
    FunctionInjector(pid_t pid, char *tempdir);
    ~FunctionInjector();

    void assign_source(char *srcFilePath);
    char *compile_func(char *funcname);
    int inject_func(char *funcname, target_process_state type);
    int inject_func_under_ptrace(char *funcname, target_process_state type);
};


#endif //C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
