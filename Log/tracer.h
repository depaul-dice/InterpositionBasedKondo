#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include "../uthash.h"
#include "traceDataHandler.h"

typedef struct fileAndDesc{
    char path[PATH_MAX];
    int fd;
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

typedef struct{
    long int start; // start offswt of chunk used
    long int end; // end offset of chuk sed
    struct UsedChunks* next;
} UsedChunks;

typedef struct {
    /* data */
    long int start; // Start offset in heap
    long int end; // End location in heap
    long int size; // Size of heap location
    long int readSize; // Amount of data read into it
    int tracking; // Boolean to se if sensitive
    int timestamp; // Timestamp corresponding to read call
    UsedChunks *head; // Chubnks of used data
    UsedChunks *tail;
    char path[PATH_MAX]; // Path of file read from
    struct HeapLocations* next;
} HeapLocations;

HeapLocations* setHeap(void *buf, char *path, ssize_t n);
HeapLocations* head = NULL;
HeapLocations* tail = NULL;

int heapSize = 0;