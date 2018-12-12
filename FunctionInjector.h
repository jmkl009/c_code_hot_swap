//
// Created by WangJingjin on 2018-12-11.
//

#ifndef C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
#define C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H

#include "hot_swap.hpp"
#include <unordered_set>
using std::unordered_set;

class FunctionInjector {
private:
    pid_t target;
    char *tempdir;
    char *srcFilePath;
    exe *target_exe;
    TargetUsefulFuncAddrs func_addrs;
    vector<string> linker_flags;
    std::unordered_set<string> injectedFunctions;

public:
    FunctionInjector(pid_t pid, char *tempdir);
    ~FunctionInjector();

    void assign_source(char *srcFilePath);
    int compile_func(char *funcname);
};


#endif //C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
