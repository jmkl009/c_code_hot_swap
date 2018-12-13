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


/**
 *
 * Isolate a function in the inFile into a separate file
 *
 * @param inFile The name of the file in which the function is searched
 * @param funcName The name of the function to search
 * @param writeFile The output file
 * @return -1 for error, 0 for function not found, 1 for sucess.
 */
int isolateFunction(char* inFile, char * funcName, char* writeFile);



#endif //C_CODE_HOT_SWAP_CPARSER_H
