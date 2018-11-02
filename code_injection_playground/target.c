//
// Created by WangJingjin on 2018/11/2.
//

#include <stdio.h>
int main(void)
{
    int c;
    while (EOF != (c = getc(stdin)))
        putc(c, stdout);
    return 0;
}
