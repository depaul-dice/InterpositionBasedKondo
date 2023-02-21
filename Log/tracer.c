#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "tracer.h"
#include <sys/types.h>
#include <sys/stat.h>

/*
 *
 *
 * Function to setup the system before first call by populating
 * the blacklist and whitelist
 *
 */
void initFileList()
{

    static FILE *(*real_fopen)(const char *filename, const char *mode) = NULL;
    if (real_fopen == NULL)
        real_fopen = dlsym(RTLD_NEXT, "fopen");
    // mkdir("readOut");
    FILE *fptr = real_fopen("WhiteList.txt", "r");
    int n;
    fscanf(fptr, "%d\n", &n);
    fileList *newFile;
    for (int i = 0; i < n; i++)
    {
        newFile = malloc(sizeof(fileList));
        fscanf(fptr, "%s\n", newFile->path);
        HASH_ADD_STR(whiteList, path, newFile);
    }
    fclose(fptr);

    fptr = real_fopen("BlackList.txt", "r");
    fscanf(fptr, "%d\n", &n);

    for (int i = 0; i < n; i++)
    {
        newFile = malloc(sizeof(fileList));
        fscanf(fptr, "%s\n", newFile->path);
        HASH_ADD_STR(blackList, path, newFile);
    }
    fclose(fptr);
}
/*
 *
 *
 * Function to check whether file with given path needs to be specialized or not
 * @param path Path to file
 * @return 1 if needs to be specialized and 0 if not
 *
 */
int inList(char *path)
{
    fileList *cur, *tmp, *cur1, *tmp1;
    HASH_ITER(hh, whiteList, cur, tmp)
    {
        if (strcmp(cur->path, path) == 0 || (strstr(path, cur->path) != NULL))
        {
            HASH_ITER(hh, blackList, cur1, tmp1)
            {
                if ((strcmp(cur1->path, path) == 0) || (strstr(path, cur1->path) != NULL))
                {
                    return 0;
                }
            }
            fileAndDesc *cur;
            HASH_FIND(hh1, openListPath, path, strlen(path), cur);
            if (cur == NULL)
                return 1;
            else
                return 0;
        }
    }
    return 0;
}
/*
 *
 *
 * Function to add a file to the list of open files
 * @param buf Path to file
 * @param fd File Desc of the file to be added
 * 
 */
void addOpenFile(char *buf, int fd)
{

    fileAndDesc *newFile = malloc(sizeof(fileAndDesc));
    strcpy(newFile->path, buf);
    newFile->fd = fd;
    newFile->fptr = NULL;
    HASH_ADD(hh2, openListFD, fd, sizeof(int), newFile);
    HASH_ADD(hh1, openListPath, path, strlen(buf), newFile);
}

/*
 *
 *
 * Function to remove a file from the list of open files
 * @param fd Fd of file to be closed
 *
 */
void closeFile(int fd)
{
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    setToClose(cur->path);
    HASH_DELETE(hh2, openListFD, cur);
    HASH_DELETE(hh1, openListPath, cur);
    free(cur);
}

/*
 *
 *
 * Function to add a file to the list of open files
 * @param buf Path to file
 * @param fptr File pointer to the file
 *
 */
void addOpenFileFPTR(char *buf, FILE *fptr)
{

    fileAndDesc *newFile = malloc(sizeof(fileAndDesc));
    strcpy(newFile->path, buf);
    newFile->fptr = fptr;
    newFile->fd = -1;
    HASH_ADD(hh3, openListFPTR, fptr, sizeof(fptr), newFile);
    HASH_ADD(hh1, openListPath, path, strlen(buf), newFile);
}
/*
 *
 *
 * Function to remove from list of open files
 * @param fptr File pointer to file
 *
 */
void closeFileFPTR(FILE *fptr)
{
    fileAndDesc *cur;
    HASH_FIND(hh3, openListFPTR, &fptr, sizeof(fptr), cur);
    setToClose(cur->path);
    HASH_DELETE(hh3, openListFPTR, cur);
    HASH_DELETE(hh1, openListPath, cur);
    free(cur);
}

/*
 *
 * Wrapper for fopen 
 *
 */
FILE *fopen(const char *filename, const char *mode)
{

    // Check if system has been setup
    if (checkSetup() == 0)
    {
        initFileList();
        finishSetup();
        setup = 1;
    }

    static FILE *(*real_fopen)(const char *filename, const char *mode) = NULL;
    if (real_fopen == NULL)
        real_fopen = dlsym(RTLD_NEXT, "fopen");

    // Get absolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);

    FILE *ret = real_fopen(filename, mode);

    // Check if we need to specialize this file ot not
    if (inList(path))
    {
        // Add file to list of open files
        addOpenFileFPTR(path, ret);
        // Open files and create a metadata structure for the files if being opened for the first time
        openFile(filename, path, 0, -1, ret);
    }
    return ret;
}
/*
 *
 * Wrapper for open 
 *
 */
int openat(int dirfd, const char *filename, int flags)
{

    // Check if system has been setup
    if (checkSetup() == 0)
    {
        initFileList();
        finishSetup();
        setup = 1;
    }

    // Get absolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);


    static int (*real_open)(const char *, int) = NULL;
    if (!real_open)
        real_open = dlsym(RTLD_NEXT, "open");

    int fd = real_open(filename, flags);


    // Check if we need to specialize this file ot not
    if (inList(path))
    {
        // Add file to list of open files
        addOpenFile(path, fd);
        // Open files and create a metadata structure for the files if being opened for the first time
        openFile(filename, path, fd, flags, NULL);
    }
    return fd;
}



/*
 *
 * Wrapper for open 
 *
 */
int open(const char *filename, int flags)
{

    // Check if system has been setup
    if (checkSetup() == 0)
    {
        initFileList();
        finishSetup();
        setup = 1;
    }

    // Get absolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);


    static int (*real_open)(const char *, int) = NULL;
    if (!real_open)
        real_open = dlsym(RTLD_NEXT, "open");

    int fd = real_open(filename, flags);


    // Check if we need to specialize this file ot not
    if (inList(path))
    {
        // Add file to list of open files
        addOpenFile(path, fd);
        // Open files and create a metadata structure for the files if being opened for the first time
        openFile(filename, path, fd, flags, NULL);
    }
    return fd;
}

/*
 *
 * Wrapper for open64 
 *
 */
int open64(const char *filename, int flags)
{
    // Check if system has been setup
    if (checkSetup() == 0)
    {
        initFileList();
        finishSetup();
        setup = 1;
    }

    // Get absolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);

    static int (*real_open64)(const char *, int) = NULL;
    if (!real_open64)
        real_open64 = dlsym(RTLD_NEXT, "open");

    int fd = real_open64(filename, flags);

    // Check if we need to specialize this file or not 
    if (inList(path))
    {
        // Add file to list of open files
        addOpenFile(path, fd);
        // Open files and create a metadata structure for the files if being opened for the first time
        openFile(filename, path, fd, flags, NULL);
    }
    return fd;
}

/*
 *
 * Wrapper for close
 *
 */
int close(int fd)
{
    // Create pointer to real close and grab using dlsym
    static int (*real_close)(int fd) = NULL;
    if (!real_close)
        real_close = dlsym(RTLD_NEXT, "close");

    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
        // If not just clal real close and pass on its return
        return real_close(fd);
    closeFile(fd);
    // Pass on the return after actually calling close on the file descriptor
    return real_close(fd);
}

/*
 *
 * Wrapper for dclose
 *
 */
int fclose(FILE *stream)
{
    // Create pointer to real close and grab using dlsym
    static int (*real_fclose)(FILE * stream) = NULL;
    if (!real_fclose)
        real_fclose = dlsym(RTLD_NEXT, "fclose");

    fileAndDesc *cur;
    HASH_FIND(hh3, openListFPTR, &stream, sizeof(stream), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_fclose(stream);
    }

    closeFileFPTR(stream);
    return real_fclose(stream);
}

/*
 *
 * Wrapper for __pread_chk
 *
 */
ssize_t __pread_chk(int fd, void *buf, size_t nbytes, off_t offset, size_t buflen)
{
    static ssize_t (*real_pread64)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if (real_pread64 == NULL)
        real_pread64 = dlsym(RTLD_NEXT, "pread64");
    
    // Look for this file in the list of open files
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
        // If not just clal real close and pass on its return
        return real_pread64(fd, buf, nbytes, offset);
    ssize_t ret = real_pread64(fd, buf, nbytes, offset);
    // Log the read 
    HeapLocations* loc = setHeap(buf, cur->path, ret);
    logRead(cur->path, cur->fd, 1, ret, offset, (void *) loc);
    return ret;
}

/*
 *
 * Wrapper for pread
 *
 */
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    static ssize_t (*real_pread)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if (real_pread == NULL)
        real_pread = dlsym(RTLD_NEXT, "pread");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
        // If not just clal real close and pass on its return
        return real_pread(fd, buf, count, offset);
    ssize_t ret = real_pread(fd, buf, count, offset);
    // Log the read
    HeapLocations* loc = setHeap(buf, cur->path, ret);
    logRead(cur->path, cur->fd, 1, ret, offset, (void *) loc);
    return ret;
}
/*
 *
 * Wrapper for pread64
 *
 */
ssize_t pread64(int fd, void *buf, size_t count, off_t offset)
{

    static ssize_t (*real_pread64)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if (real_pread64 == NULL)
        real_pread64 = dlsym(RTLD_NEXT, "pread64");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
        // If not just clal real close and pass on its return
        return real_pread64(fd, buf, count, offset);
    ssize_t ret = real_pread64(fd, buf, count, offset);
    // Log the read
    HeapLocations* loc = setHeap(buf, cur->path, ret);
    logRead(cur->path, cur->fd, 1, ret, offset, (void *) loc);
    return ret;
}

/*
 *
 * Wrapper for read
 *
 */
ssize_t read(int fd, void *buf, size_t count)
{
    static ssize_t (*real_read)(int fd, void *buf, size_t count) = NULL;
    if (real_read == NULL)
        real_read = dlsym(RTLD_NEXT, "read");
    ssize_t ret = real_read(fd, buf, count);
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
        // If not just clal real close and pass on its return
        return ret;
    // Log the read
    HeapLocations* loc = setHeap(buf, cur->path, ret);
    logRead(cur->path, cur->fd, 0, ret, 0, (void *) loc);
    return ret;
}

/*
 *
 * Wrapper for fread
 *
 */
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    static ssize_t (*real_fread)(void *ptr, size_t size, size_t nmemb, FILE *stream) = NULL;
    if (real_fread == NULL)
        real_fread = dlsym(RTLD_NEXT, "fread");
    static FILE *(*real_fopen)(const char *filename, const char *mode) = NULL;
    if (real_fopen == NULL)
        real_fopen = dlsym(RTLD_NEXT, "fopen");
    fileAndDesc *cur;
    HASH_FIND(hh3, openListFPTR, &stream, sizeof(stream), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_fread(ptr, size, nmemb, stream);
    }
    size_t ret = real_fread(ptr, size, nmemb, stream);
    // Calculate total read Size
    off_t readSize = ret * size;
    // Log the read
    HeapLocations* loc = setHeap(ptr, cur->path, ret);
    logRead(cur->path, -1, 0, readSize, 0, (void *) loc);
    return ret;
}
/*
 *
 * Wrapper for lseek
 *
 */
off_t lseek(int fildes, off_t offset, int whence)
{
    // Create a pointer to real lseek and grab it using dlsym
    static off_t (*real_lseek)(int fildes, off_t offset, int whence) = NULL;
    if (!real_lseek)
        real_lseek = dlsym(RTLD_NEXT, "lseek");

    off_t ret = real_lseek(fildes, offset, whence);

    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fildes, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return ret;
    }

    // Perform the lseek based change on the metadata structure
    performLseek(cur->path, offset, whence);
    return ret;
}
/*
 *
 * Wrapper for lseek64
 *
 */
off_t lseek64(int fildes, off_t offset, int whence)
{
    // Create a pointer to real lseek and grab it using dlsym
    static off_t (*real_lseek64)(int fildes, off_t offset, int whence) = NULL;
    if (!real_lseek64)
        real_lseek64 = dlsym(RTLD_NEXT, "lseek64");

    off_t ret = real_lseek64(fildes, offset, whence);

    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fildes, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return ret;
    }

    // Perform the lseek based change on the metadata structure
    performLseek(cur->path, offset, whence);
    return ret;
}
/*
 *
 * Wrapper for fseek
 *
 */
int fseek(FILE *stream, long int offset, int whence)
{
    fprintf(stdout, "Fseek called\n");
    // Create a pointer to real lseek and grab it using dlsym
    static int (*real_fseek)(FILE * stream, long int offset, int whence) = NULL;
    if (!real_fseek)
        real_fseek = dlsym(RTLD_NEXT, "fseek");

    int ret = real_fseek(stream, offset, whence);

    fileAndDesc *cur;
    HASH_FIND(hh3, openListFPTR, &stream, sizeof(stream), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return ret;
    }
    // Perform the lseek based change on the metadata structure
    performLseek(cur->path, offset, whence);
    return ret;
}
/*
 *
 * Wrapper for write
 *
 */
ssize_t write(int fildes, const void *buf, size_t nbytes)
{
    // Create pointer and grab real write using dlsym
    static ssize_t (*real_write)(int fildes, const void *buf, size_t nbytes) = NULL;
    if (!real_write)
        real_write = dlsym(RTLD_NEXT, "write");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fildes, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_write(fildes, buf, nbytes);
    }
    // Call helper function to log and version for the write call
    performBackup(cur->path, 1, (int)nbytes, 0);
    // Call and pass on real write
    return real_write(fildes, buf, nbytes);
}

/*
 *
 * Wrapper for pwrite
 *
 */
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    static ssize_t (*real_pwrite)(int fd, const void *buf, size_t count, off_t offset) = NULL;
    if (real_pwrite == NULL)
        real_pwrite = dlsym(RTLD_NEXT, "pwrite");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_pwrite(fd, buf, count, offset);
    }
    // Call helper function to log and version for the write call
    performBackup(cur->path, 0, (int)count, offset);
    // Call and pass on real write
    return real_pwrite(fd, buf, count, offset);
}

int fstat(int fd, struct stat *buf)
{
    static int (*real_fstat)(int fd, struct stat *buf) = NULL;
    if (real_fstat == NULL)
        real_fstat = dlsym(RTLD_NEXT, "fstat");

    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_fstat(fd, buf);
    }
    else
    {
        // fprintf(stdout, "intercepted fstat for %s\n", cur->path);
        // fprintf(stdout, "Size:%ld BlkSize:%d\n",buf->st_size, buf->st_blksize);

        int ret = real_fstat(fd, buf);
        captureStat(cur->path, buf, 1);
        return real_fstat(fd, buf);
    }
}
int fstat64(int fd, struct stat64 *buf)
{
    static int (*real_fstat)(int fd, struct stat64 *buf) = NULL;
    if (real_fstat == NULL)
        real_fstat = dlsym(RTLD_NEXT, "fstat64");

    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);

    // Checkk if we are keeping a track of this file or not
    if (cur == NULL)
    {
        return real_fstat(fd, buf);
    }
    else
    {
        // fprintf(stdout, "intercepeted fstat64 for %s\n", cur->path);
        captureStat(cur->path, buf, 0);
        // fprintf(stdout, "Size:%ld BlkSize:%d\n",buf->st_size, buf->st_blksize);
        return real_fstat(fd, buf);
    }
}

// int lstat64(const char *__restrict __file,
//             struct stat64 *__restrict __buf)
// {
//     // fprintf(stdout, "intercepted lstat on %s\n", __file);
//     static int (*real_lstat)(const char *__restrict __file,
//                              struct stat64 *__restrict __buf) = NULL;
//     if (real_lstat == NULL)
//         real_lstat = dlsym(RTLD_NEXT, "lstat64");
//     char newpath[PATH_MAX];
//     realpath(__file, newpath);
//     if (inList(newpath))
//     {
//         fprintf(stdout, "intercepted lstat on %s\n", __file);
//     }
//     return real_lstat(__file, __buf);
// }

// int stat64(const char *__restrict path,
//            struct stat64 *__restrict buf)
// {
//     static int (*real_stat)(const char *__restrict path,
//                             struct stat64 *__restrict buf) = NULL;
//     if (real_stat == NULL)
//         real_stat = dlsym(RTLD_NEXT, "stat64");
//     char realPath[PATH_MAX];
//     realpath(path, realPath);
//     if (inList(realPath))
//     {
//         // fprintf(stdout, "intercepted for %s\n", realPath);
//     }
//     return real_stat(path, buf);
// }

// /*
//  *
//  * Wrapper for lstat
//  *
//  */
// int lstat(const char *path, struct stat *buf)
// {
//     static int (*real_lstat)(const char *path, struct stat *buf) = NULL;
//     if (real_lstat == NULL)
//         real_lstat = dlsym(RTLD_NEXT, "lstat");
//     // Get absolute apth to file

//     char newpath[PATH_MAX];
//     realpath(path, newpath);
//     if (inList(newpath))
//     {
        
//     }
//     return real_lstat(path, buf);
// }

// int stat(const char *path, struct stat *buf)
// {
//     static int (*real_stat)(const char *path, struct stat *buf) = NULL;
//     if (real_stat == NULL)
//         real_stat = dlsym(RTLD_NEXT, "stat");
//     char realPath[PATH_MAX];
//     realpath(path, realPath);
//     if (inList(realPath))
//     {
//         // fprintf(stdout, "intercepted for %s\n", realPath);
//     }
//     return real_stat(path, buf);
// }
//
//

HeapLocations *check_in_range(void *ptr)
{
	HeapLocations *cur = head;
	long int check = (long int)ptr;
	while(cur!=NULL){
		if (check >= (long int)cur->start && check < (long int)cur->end)
		{
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}

// void *malloc(size_t size)
// {   
//     heapSize+=1;
// 	static void *(*real_malloc)(size_t size) = NULL;
// 	if (real_malloc == NULL)
// 		real_malloc = dlsym(RTLD_NEXT, "malloc");

// 	void *ret = real_malloc(size);
// 	HeapLocations *cur = real_malloc(sizeof(HeapLocations));
// 	cur->start = (long int)ret;
// 	cur->end = (long int)ret + size;
// 	cur->tracking = 0;
// 	cur->head = NULL;
// 	cur->tail = NULL;
// 	cur->next = NULL;
//     cur->size = cur->end - cur->start;
//     cur->readSize = 0;
// 	if(head == NULL){
// 		head = cur;
// 		tail = cur;
// 	}else{
// 		tail->next = cur;
// 		tail = cur;
// 	}
// 	return ret;
// }
// void *memcpy(void *__restrict __dest, const void *__restrict __src,
//                      size_t __n)
// {
// 	static void *(*real_malloc)(size_t size) = NULL;
//         if (real_malloc == NULL)
//                 real_malloc = dlsym(RTLD_NEXT, "malloc");
// 	static void *(*real_memcpy)(void *__restrict __dest, const void *__restrict __src,
//                      size_t __n) = NULL;
// 	if (real_memcpy == NULL)
// 		real_memcpy = dlsym(RTLD_NEXT, "memcpy");

// 	HeapLocations *cur = check_in_range((void *)__src);
// 	if (cur && cur->tracking)
// 	{	
// 		UsedChunks *curChunk = real_malloc(sizeof(UsedChunks));
// 		curChunk->start = (long int)__dest;
// 		curChunk->end = (long int)__dest + __n;
// 		curChunk->next = NULL;
// 		if(cur->head==NULL){
// 			cur->head = curChunk;
// 			cur->tail = curChunk;
// 		}else{
// 			cur->tail->next = curChunk;
// 			cur->tail = curChunk;
// 		}
//         fprintf(stdout, "Copying from file %s \nwith actual read size %ld \nand copying %ld bytes\n",cur->path,cur->readSize, __n);
	
//     fprintf(stdout,"The size of the linlist is %d\n",heapSize);
//     }
// 	return real_memcpy(__dest, __src, __n);
// }
// void free(void *ptr){
// 	static void *(*real_free)(void *ptr) = NULL;
// 	if (real_free == NULL)
// 		real_free = dlsym(RTLD_NEXT, "free");	
	
// 	long int ptrKey = (long int) ptr;
// 	HeapLocations *cur = head;
// 	HeapLocations* prev = NULL;
// 	long int check = (long int)ptr;
// 	while(cur!=NULL)
// 	{
// 		if (check >= (long int)cur->start && check < (long int)cur->end)
// 		{
// 			if(prev == NULL){
// 				head = head->next;
// 			}else{
// 				prev->next = cur->next;
// 				if(cur == tail){
// 					tail = prev;
// 				}
// 			}
// 			break;
// 		}
// 		prev = cur;
// 		cur = cur->next;
// 	}

// 	if(cur!=NULL){
// 		// if(cur->tracking==1){
// 		// 	fprintf(stdout, "Deletting a malloc which had a tracking of 1\n");
// 		// }
// 		// prev->next = cuz/r->next;
// 		UsedChunks* chunks=cur->head;
// 		UsedChunks* tmp;
// 		while(chunks != NULL){
// 			tmp = chunks;
// 			chunks = chunks->next;
// 			// fprint(stdout, "Calling free on %ld\n", tmp);
// 			real_free(tmp);
// 		}
// 		real_free(cur);
// 	}
	
// 	real_free(ptr);
// }

HeapLocations* setHeap(void *buf, char *path, ssize_t n){
    return NULL;
    // // fprintf(stdout, "Reading from the file %s into location starting at %ld\n",path, (long int) buf);
    // HeapLocations* cur = check_in_range(buf);
    // if(cur!=NULL){
	// // fprintf(stdout, "Into location %ld\n",(long int) cur->start);
    //     cur->tracking = 1;
    //     cur->readSize = n;
    //     cur->timestamp = logicalTimestamp;
    //     strcpy(cur->path, path);
    // }
    // return cur;
}
