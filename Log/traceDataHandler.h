#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../uthash.h"
#ifndef O_APPEND
# define O_APPEND     0x0008
#endif
#ifndef O_WRONLY
#define	O_WRONLY	0x0001
#endif
#ifndef O_CREAT
#define	O_CREAT		0x0200	
#endif
typedef struct Call{
    /* data */
    int type;
    int location;
    off_t offset;
    off_t opSize;
    int timestamp;
    struct Call* nextCall;
}Call;

typedef struct GlobalReadList{
    /* data */
    off_t start;
    off_t end;
    struct GlobalReadList* next;
}GlobalReadList;

typedef struct versionedData{
    off_t origStart;
    off_t backupfileStart;
    off_t size;
    off_t timestamp;
    struct versionedData* next;
} versionedData;

typedef struct fileTraceObject{
    FILE* fptr;
    char path[PATH_MAX];
    char filename [PATH_MAX];
    int filePointer;
    int fileDescriptor;
    int status;
    int flags;
    off_t size;
    off_t backupSize;
    Call* firstCall;
    Call* lastCall;
    GlobalReadList *readList;
    versionedData* firstVerstion;
    versionedData* lastVersion;
    UT_hash_handle hh;

}fileTraceObject;

fileTraceObject *fileTrace = NULL;

int setup = 0;

int logicalTimestamp = 0;
int checkSetup();

int openFile(const char *filename, char *path, int fd, int flags, FILE* fptr);

void printFileObjects();

void setToClose(char* path);

void logRead(char* path, int fd, int type, off_t size, off_t offset);

GlobalReadList *addtoReadList(Call *call, GlobalReadList *head);

void printReadList(GlobalReadList* head);

void performLseek(char* path, off_t offset, int whence);

void performBackup(char* path, int type, off_t nbytes, off_t offset);

int checkOverlap(off_t s1, off_t e1, off_t s2, off_t e2);

void storeVersion(fileTraceObject *file, char* path, off_t start, off_t end);

void finishSetup();

int getBackupFD();
int captureStat(char* path, void *buf, int loc);
// int captureStat(int fd, void *buf);

void flushToFile(fileTraceObject* file);
