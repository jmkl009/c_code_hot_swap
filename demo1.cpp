//
// Created by WangJingjin on 2018/12/9.
//
//#include "hot_swap.hpp"
#include "FunctionInjector.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "libs/utils.h"

#define FUNC_NAME_MAX (1024)
#define TEMP_DIR "./tmp"

int main(int argc, char* argv[]) {
    if(argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    char* command = argv[1];
    char* commandArg = argv[2];

    char* processName = NULL;
    pid_t target = 0;
    if (!strcmp(command, "-n")) {
        processName = commandArg;
        target = findProcessByName(processName);
        if(target == -1)
        {
            fprintf(stderr, "doesn't look like a process named \"%s\" is running right now\n", processName);
            return 1;
        }

        printf("targeting process \"%s\" with pid %d\n", processName, target);

    } else if(!strcmp(command, "-p")) {
        target = atoi(commandArg);
        printf("targeting process with pid %d\n", target);
    } else {
        usage(argv[0]);
        exit(1);
    }

    char srcFilePath[PATH_MAX];
    if (argc >= 5) {
        strcpy(srcFilePath, argv[4]);
        char * file_command = argv[3];
        if (strcmp(file_command, "-f")) {
            usage(argv[0]);
            exit(1);
        }
    } else {
        char buff[PATH_MAX];
        printf("Please enter the path of the source file: \n");
        if (scanf("%s", buff) == 0) {
            fprintf(stderr, "Please enter a valid source file name.\n");
            exit(1);
        }
        strcpy(srcFilePath, buff);
    }

    struct stat srcFileStat;
    int ret = stat(srcFilePath, &srcFileStat);
    if (ret == -1) {
        perror("source file");
    }
    time_t last_modify = srcFileStat.st_mtim.tv_sec;
    printf("source file: %s, last_modify: %ld\n", srcFilePath, last_modify);

    printf("WARNING: Please make sure that the executable "
           "was compiled with flag -rdynamic, "
           "or else the code injection may not work "
           "if you call statically compiled functions.\n");

    int status = mkdir(TEMP_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    TargetUsefulFuncAddrs func_addrs;
    init_target_useful_func_addrs(target, &func_addrs);

    printf("Please enter the name of the function to watch: \n");
    char funcname[FUNC_NAME_MAX];
    if (scanf("%s", funcname) == 0) {
        fprintf(stderr, "Scan error.");
        exit(1);
    }
    FunctionInjector injector(target, TEMP_DIR);
    injector.assign_source(srcFilePath);

    void *libAddr = NULL;
    while (1) {
        sleep(1);
        stat(srcFilePath, &srcFileStat);
        time_t modified_time = srcFileStat.st_mtim.tv_sec;
        if (modified_time == last_modify) {
            continue;
        }

        last_modify = modified_time;
        injector.compile_func(funcname);
        injector.inject_func(funcname, RUNNING);
    }
}