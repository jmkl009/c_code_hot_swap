//
// Created by WangJingjin on 2018/10/17.
//

#include <dlfcn.h>
#include <stdio.h>

void something(long str, int i,int k, int j, int l) {
    printf("%p, %p, %p, %p, %p\n", &str, &i, &k, &j, &l);
}

typedef void* (*arbitrary)();
// do not mix this with   typedef void* (*arbitrary)(void); !!!

int main() {
    arbitrary my_function;
    // Introduce already loaded functions to runtime linker's space
    void* handle = dlopen(0, RTLD_NOW|RTLD_GLOBAL);
    // Load the function to our pointer, which doesn't know how many arguments there sould be
    *(void**)(&my_function) = dlsym(handle,"something");
    // Call something via my_function
    (void)  my_function("I accept a string and an integer!\n",(int)(2*2));
    return 0;
}