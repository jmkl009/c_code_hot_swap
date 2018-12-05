//
// Created by WangJingjin on 2018/10/17.
//
//#include <stdio.h>
//void * test2(char* val, long i);

//void test2(char* val, long i);
//int main();

//extern int counter;
//#include "../cparser.h"

//extern void incrementCounter();
//void incrementCounter() __attribute__((weak));
//void isolateFunction(const char *, const char *, const char*) __attribute__((weak));


void test2(char* val, long i) {
    printf("test2 in hot swap test-1: %s, %ld\n", val, i);
}

void test(char* val, long i) {
    test2(val, i);
    incrementCounter();
    isolateFunction("hello_world.c", "main", "test_parser.c");
//    printf("counter in test: %d\n", counter);s
    printf("test in hot swap test-1: %s, %ld\n", val, i);
//    main();
}