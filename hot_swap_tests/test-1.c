//
// Created by WangJingjin on 2018/10/17.
//
//#include <stdio.h>
//void * test2(char* val, long i);

//void test2(char* val, long i);
//int main();

//extern int counter;

void test(char* val, long i) {
    test2(val, i);
    incrementCounter();
//    printf("counter in test: %d\n", counter);
    printf("test in hot swap test-1: %s, %ld\n", val, i);
//    main();
}

//void test2(char* val, long i) {
//
//    printf("test2 in hot swap test-1: %s, %ld\n", val, i);
//}