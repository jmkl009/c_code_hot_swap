//
// Created by WangJingjin on 2018/11/2.
//

#include <stdio.h>
int func1() {
    printf("I am in func1 after mod!\n");
    func2();
    return 1;
}

int func2() {
    printf("Doesn't matter\n");
    return 2;
}

int main(void)
{
    
    int c;
    
    while ((c = getc(stdin)) != EOF) {
        putc(c, stdout);
        func1();
        func2();
    }
    return 0;
}
