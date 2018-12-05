//
// Created by WangJingjin on 2018/11/1.
//

#include <dlfcn.h>

typedef void*(*arbitrary)(void*);

int main() {
    void *handle = dlopen("../hot_swap_tests/test-1.so", RTLD_NOW|RTLD_GLOBAL);
    arbitrary func = dlsym(handle, "test2");
    dlclose(handle);
}