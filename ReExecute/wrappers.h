#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <unistd.h>
#include "../uthash.h"
#include "reExecute.h"

typedef struct fileAndDesc{
    char path[PATH_MAX];
    int fd;
    char filename[PATH_MAX];
    FILE* fptr;
    UT_hash_handle hh1;
    UT_hash_handle hh2;
    UT_hash_handle hh3;
} fileAndDesc;

typedef struct fileList{
    char path[PATH_MAX];
    int fd;
    UT_hash_handle hh;
} fileList;
int filename = 0;
fileAndDesc *openListPath = NULL;
fileAndDesc *openListFD = NULL;
fileAndDesc *openListFPTR = NULL;
fileList *blackList = NULL;
fileList *whiteList = NULL;
