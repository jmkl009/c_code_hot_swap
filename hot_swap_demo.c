//// Created by WangJingjin on 2018/10/17.//#include <dlfcn.h>#include <stdio.h>#include <unistd.h>#include <sys/mman.h>#include <stdlib.h>#include "cparser.h"static int counter = 0;void something(char* val, long i,int k, int j, int l) {    printf("%p:%s, %p:%ld, %p, %p, %p\n", &val, val, &i, i, &k, &j, &l);}void something_2(char* val, long i) {    printf("%s, %ld\n", val, i);}void test(char* val, long i) {    printf("test in main: %s, %ld\n", val, i);//    return;}typedef void* (*arbitrary)();void replaceFunction(arbitrary old_func, arbitrary new_func) {    int32_t offset = (int64_t)new_func - ((int64_t)old_func + 5 * sizeof(char));    size_t pageSize = sysconf(_SC_PAGESIZE);    uintptr_t start = (uintptr_t)old_func;    uintptr_t end = start + 1;    uintptr_t pageStart = start & -pageSize;    mprotect((void *)pageStart, end - pageStart, PROT_READ | PROT_WRITE | PROT_EXEC);    int64_t instruction = 0xe9 | offset << 8;    *(int64_t *)old_func = instruction;}//__attribute__((constructor))//static void ctor(void) {//    printf("Dylib constructor called!\n");//}void test2(char* val, long i) {    printf("test2 in hot swap main: %s, %ld\n", val, i);}void incrementCounter() {    counter++;}int main() {//    arbitrary my_function;    // Introduce already loaded functions to runtime linker's space//    test("ghuh",(int)(2*2));//    printf("counter: %d\n", counter);//    char buffer[1024];//    scanf("%s", buffer);//    if (buffer[0] == '1') {//        handle = dlopen("../hot_swap_tests/test-1.so", RTLD_NOW|RTLD_GLOBAL);//    } else {//        handle = dlopen("../hot_swap_tests/test-2.so", RTLD_NOW|RTLD_GLOBAL);//    }//    void* handle = dlopen(buffer, RTLD_NOW|RTLD_GLOBAL);////    if (handle == 0) {//        perror(dlerror());//        exit(1);//    }//    arbitrary newFunc = dlsym(handle, "test");//    printf("newFunc addr:%p\n", newFunc);//    newFunc = dlsym(handle, "test2");//    printf("newFunc addr:%p\n", newFunc);//    newFunc = dlsym(handle, "test");//    printf("newFunc addr:%p\n", newFunc);////    replaceFunction(test, newFunc);////    test("ghuh",(int)(2*2));//    dlclose(handle);//    main();    isolateFunction("../hot_swap_demo.c", "test", "../parser_test_out.c");//    execlp("gcc", "gcc", "-shared", "-o", "../parser_test.so", "-fPIC", "../parser_test_out.c", "-undefined", "dynamic_lookup", NULL);    return 0;}