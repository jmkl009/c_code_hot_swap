//
// Created by WangJingjin on 2018/10/25.
//

#ifndef C_CODE_HOT_SWAP_CPARSER_H
#define C_CODE_HOT_SWAP_CPARSER_H
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>


int isolateFunction(const char* inFile, char * funcName, const char* writeFile);



#endif //C_CODE_HOT_SWAP_CPARSER_H
