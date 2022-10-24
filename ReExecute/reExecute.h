#define _GNU_SOURCE
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include "../uthash.h"

typedef struct SubsetPointer{
    off_t start;
    off_t size;
    int backup;
    struct SubsetPointer* next;
} SubsetPointer;

typedef struct Pointer{
    int timestamp;
    int numPointers;
    off_t OpSize;
    SubsetPointer* first;
    SubsetPointer* last;
    UT_hash_handle hh;
} Pointer;

typedef struct fileInfo{
    struct Pointer* pointers;
    char filename[PATH_MAX];
    char path[PATH_MAX];
    int BackupFD;
    int SubsetFD;
    off_t size;
    UT_hash_handle hh;
}fileInfo;

fileInfo* Fd = NULL;
fileInfo* Fname = NULL;
int setup = 0;
int setup1 = 0;
int logicalTime = 0;
void init();
ssize_t performRead( char* path, void* buf);
void openFile(const char* filename,char* path, int fd);
void getData(void* buf, SubsetPointer* cur, fileInfo* curFile);
void engineerStat(char* path, void *buf, int loc);