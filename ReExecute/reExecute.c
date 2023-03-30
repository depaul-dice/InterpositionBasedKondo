#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include "reExecute.h"

/*
 *
 *
 * Function to open a new file and setup the subset and backup files 
 * related to that original file and read the redirection metadata along
 * with it
 * @param filename Name of file to open
 * @param path Path of file to open
 *
 */
void openNewFile(const char* filename, char* path, int fd){
    // Create new metadata structure
    fileInfo* newFile = malloc(sizeof(fileInfo));
    newFile->pointers=NULL;

    // Get CWD and navigate to the metadata file location
    // subset file location
    // and backup file location
    char pointer[PATH_MAX] = "";
    getcwd(pointer, sizeof(pointer));
    strcat(pointer, "/tracelog/pointers/");
    char* filebasename = basename(filename);
    strcat(pointer, filebasename);
    char backup[PATH_MAX] = "";
    char subset[PATH_MAX] = "";
    getcwd(subset, sizeof(backup));
    strcat(subset, "/tracelog/subsets/");
    strcat(subset, filebasename);    
    getcwd(backup, sizeof(backup));
    strcat(backup, "/backups/");
    strcat(backup, filebasename);
    strcpy(newFile->path, path);

    // Open the file with the redirection metadata and read it line by line
    // and popul;ate the appropriate linked list
    
    FILE* fptr = fopen(pointer, "r");
    int ret = fscanf(fptr, "%ld", &newFile->size);
    while(1){
        int timestamp, numPointers;
        off_t opSize;
        int ret = fscanf(fptr, "%d:%d:%ld", &timestamp, &numPointers, &opSize);
        if(ret==EOF)
            break;
        int index = 0;
        // Create a structure to hold metadata for the call
        Pointer* curCall = malloc(sizeof(Pointer));
        curCall->first=NULL;
        curCall->last= NULL;
        curCall->timestamp = timestamp;
        curCall->numPointers = numPointers;
        curCall->OpSize = opSize;

        // Loop the given number of times to read the multiple
        // locations from where we have to populate this call
        while(index<numPointers){
            off_t offset, size;
            int backup;
            fscanf(fptr, "%d:%ld:%ld",&backup, &offset, &size);
            SubsetPointer *curPointer = malloc(sizeof(SubsetPointer));
            curPointer->start=offset;
            curPointer->size = size;
            curPointer->next = NULL;
            curPointer->backup = backup;
            if(curCall->first==NULL){
                curCall->first=curPointer;
                curCall->last = curPointer;
            }else{
                curCall->last->next = curPointer;
                curCall->last = curPointer;
            }
            index++;
        }
        HASH_ADD_INT(newFile->pointers, timestamp, curCall);
    }
    // Open the subset and backup files and store their FD
    if(fd!=0)
    newFile->SubsetFD = fd;
    else
    newFile->SubsetFD = open(subset, O_RDONLY);
    newFile->BackupFD = open(backup, O_RDONLY);
    strcpy(newFile->filename, filename);
    HASH_ADD(hh, Fname, filename, sizeof(path), newFile);
}

/*
 *
 *
 * Function to open a new file and setup the subset and backup files 
 * related to that original file and read the redirection metadata along
 * with it
 * @param filename Name of file to open
 * @param path Path of file to open
 *
 */
void openFile(const char* filename, char* path, int fd){
    fileInfo* curFile;
    HASH_FIND(hh, Fname ,path, 1024, curFile);
    // If file is already open do nothing
    if(curFile==NULL){
    // If not open then call helper
        openNewFile(filename, path, fd);
    }else{
        curFile->SubsetFD=fd;
    }
}

/*
 *
 *
 * Function to perform a read call based on the current timestamp
 *  @param path Path to the file the read call has been made on
 *  @param buf Buffwer to return the data in
 *  @return Number of bytes read
 */
ssize_t performRead(char* path, void* buf){

    void* movingBuf = buf;
    int size = 0;
    Pointer* curRead;
    fileInfo *curFile, *tmp;
    // Find appropriate file metadata from map
    HASH_ITER(hh, Fname, curFile, tmp){
        if(strcmp(path, curFile->path)==0){
            // Find appropriate call metadata from map in file metadata structure
            HASH_FIND_INT(curFile->pointers, &logicalTime, curRead);
            logicalTime++;
            // Loop through the different subsets that makeup the data segment
            SubsetPointer* cur = curRead->first;
            while(cur!=NULL){
                // Call helper to get data from apt file
                getData(movingBuf, cur, curFile);
                movingBuf+=cur->size;
                size += cur->size;
                cur=cur->next;
            }
            return size;
        }
    }
    return -1;
}

/*
 *
 *
 * Function to read data from the appropriate file and appropriate location
 * to perform a read
 * @param buf Buffer to read data into
 * @param cur Structure to current subset to read
 * @param fileInfio Metadata structure for the file to read for
 *
 */
void getData(void* buf, SubsetPointer* cur, fileInfo* curFile){
    static ssize_t (*real_read)(int fd, void *buf, size_t count) = NULL;
    if(real_read == NULL)
        real_read = dlsym(RTLD_NEXT, "read");

	static off_t (*real_lseek)(int fildes, off_t offset, int whence) = NULL;
	if (!real_lseek)
		real_lseek = dlsym(RTLD_NEXT, "lseek");
    
    int fd;
    
    // Check if we are reading the data from the subset file or the backup file
    if(cur->backup==1){
        fd = curFile->BackupFD;
    }else{
        fd = curFile->SubsetFD;
    }
    // Lseek to apt position and then read it 
    real_lseek(fd, cur->start, 0);
    int ret = real_read(fd, buf, cur->size);

}

/*
 *
 *
 * Function to change the size member of the stat structure
 * @param path Path to file operation is being done on
 * @param buf Generic pointer pointing to the stat structure
 * @param loc = 1 for stat structure loc = 0 for stat64 structure
 *
 */
void engineerStat(char *path, void *buf, int loc){
    fileInfo* curFile;
    HASH_FIND(hh, Fname , path, PATH_MAX*sizeof(char), curFile);
    if(curFile==NULL){
        return;
    }
     if(loc == 1){
            struct stat* stats = (struct stat*) buf;
            // fprintf(stdout, "in capture stat we have %ld\n",stats->st_size);
            stats->st_size= curFile->size;
        }else{
            struct stat64* stats = (struct stat64*) buf;
            // fprintf(stdout, "in capture stat we have %ld\n",stats->st_size);
            stats->st_size = curFile->size;
        }
    return;
}

