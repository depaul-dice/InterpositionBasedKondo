#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "wrappers.h"
#include <sys/stat.h>
#include <errno.h>
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
int inList(char *path, int check)
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
            if(cur==NULL || check ==0)
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
void addOpenFile(char *path, const char* filename, int fd)
{

    fileAndDesc *newFile = malloc(sizeof(fileAndDesc));
    strcpy(newFile->path, path);
    strcpy(newFile->filename, filename);
    newFile->fd = fd;
    HASH_ADD(hh2, openListFD, fd, sizeof(int), newFile);
    HASH_ADD(hh1, openListPath, path, strlen(path), newFile);
}
/*
 *
 *
 * Function to remove a file from the list of open files
 * @param fd Fd of file to be closed
 *
 */
void closeFile(int fd){
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
    // setToClose(cur->path);
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

    if (setup == 0)
    {
        initFileList();
        setup = 1;
    }

    static FILE *(*real_fopen)(const char *filename, const char *mode) = NULL;
    if (real_fopen == NULL)
        real_fopen = dlsym(RTLD_NEXT, "fopen");

    // Get abnsolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);


    FILE* ret;
    // Check if the file needs to be specialized
    if (inList(path, 1))
    {
        // Instead of opening the file open the subset of the file
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(filename);
        strcat(newPath, filebasename);
        ret = real_fopen(newPath, mode);
        // Add file to the list of the open files
        addOpenFileFPTR(path, ret);
        openFile(filename, path);
        // Return the fd to the subset file
        return ret;
    }

    ret = real_fopen(filename, mode);
    return ret;
}
/*
 *
 * Wrapper for open 
 *
 */
int open(const char *filename, int flags)
{

    // Sets up the backed DataStructures and files
    // if this is the first call in the function
    // fprintf(stdout, "Open called with %s\n", filename);
    if (setup == 0)
    {
        initFileList();
        setup = 1;
    }
    // Get the absolute path to the file
    char path[PATH_MAX];
    char *res = realpath(filename, path);

    // Create pointer to real open and grab it using dlsym
    static int (*real_open)(const char *, int) = NULL;
    if (!real_open)
        real_open = dlsym(RTLD_NEXT, "open");
    int fd;
    // Check if we need to specialize the file
    if (inList(path, 1))
    {
        // Isntead of opening the original file open the subset and
        // return the FD to the subset
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(filename);
        strcat(newPath, filebasename);
        fd = real_open(newPath, flags);
        // Add file to the list of the open files
        addOpenFile(path, filename, fd);
        openFile(filename, path);

        return fd;
    }else{
        return real_open(filename, flags);
    }
}
/*
 *
 * Wrapper for open64 
 *
 */
int open64(const char *filename, int flags)
{
    // Sets up the backed DataStructures and files
    // if this is the first call in the function
    // fprintf(stdout, "Open called with %s\n", filename);
    if (setup == 0)
    {
        initFileList();
        setup = 1;
    }
    // Get the absolute path to the file 
    char path[PATH_MAX];
    char *res = realpath(filename, path);

    // Create pointer to real open and grab it using dlsym
    static int (*real_open64)(const char *, int) = NULL;
    if (!real_open64)
        real_open64 = dlsym(RTLD_NEXT, "open");

    int fd;

    // Check if the file needs to be specialized
    if (inList(path, 1))
    {
        // Isntead of opening the original file open the subset and
        // return the FD to the subset        
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(filename);
        strcat(newPath, filebasename);
        fd = real_open64(newPath, flags);
        addOpenFile(path, filename, fd);
        openFile(filename, path);
        return fd;
    }
    return real_open64(filename, flags);
}
/*
 *
 * Wrapper for read 
 *
 */
ssize_t read(int fd, void *buf, size_t count){
    static ssize_t (*real_read)(int fd, void *buf, size_t count) = NULL;
    if(real_read == NULL)
        real_read = dlsym(RTLD_NEXT, "read");

    fileAndDesc *cur;
    // Check if this file needs to be specialized
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
	if (cur == NULL)
		return real_read(fd, buf, count);
    // call helper to perform the read
    performRead(cur->path, buf);
    return count;
}
/*
 *
 * Wrapper for pread 
 *
 */
ssize_t pread(int fd, void *buf, size_t count, off_t offset){
    static ssize_t (*real_pread)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if(real_pread == NULL)
        real_pread = dlsym(RTLD_NEXT, "pread");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
	// Checkk if we are keeping a track of this file or not
	if (cur == NULL)
		// If not just clal real close and pass on its return
		return real_pread(fd, buf, count, offset);
    // call helper to perform the read
    int ret = performRead(cur->path, buf);
    return ret;
}
/*
 *
 * Wrapper for __pread_chk 
 *
 */
ssize_t __pread_chk(int fd, void * buf, size_t nbytes, off_t offset, size_t buflen){
    static ssize_t (*real_pread64)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if (real_pread64 == NULL)
        real_pread64 = dlsym(RTLD_NEXT, "pread64");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
	// Checkk if we are keeping a track of this file or not
	if (cur == NULL)
		return real_pread64(fd, buf, nbytes, offset);
    // call helper to perform the read
    int ret = performRead(cur->path, buf);
    return ret;
}
/*
 *
 * Wrapper for pread64 
 *
 */
ssize_t pread64(int fd, void *buf, size_t count, off_t offset){
    static ssize_t (*real_pread64)(int fd, void *buf, size_t count, off_t offset) = NULL;
    if(real_pread64 == NULL)
        real_pread64 = dlsym(RTLD_NEXT, "pread");
    fileAndDesc *cur;
    HASH_FIND(hh2, openListFD, &fd, sizeof(int), cur);
	// Checkk if we are keeping a track of this file or not
	if (cur == NULL)
		// If not just clal real close and pass on its return
		return real_pread64(fd, buf, count, offset);
    // call helper to perform the read
    int ret = performRead(cur->path, buf);
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
    // call helper to perform the read
    int ret = performRead(cur->path, ptr);
    int returnVal;
    if(ret>0)
    returnVal = nmemb;
    else
    returnVal = -1; 

    return nmemb;
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
	if (cur == NULL)
		return real_close(fd);
    closeFile(fd);
	int ret = real_close(fd);
    return ret;
}
/*
 *
 * Wrapper for close 
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
    // fprintf(stdout, "Closed the file %s\n", cur->path);
    closeFileFPTR(stream);
    return real_fclose(stream);
}
/*
 *
 * Wrapper for lstat 
 *
 */
int lstat(const char *path, struct stat *buf)
{
    static int (*real_lstat)(const char *path, struct stat *buf) = NULL;
    if (real_lstat == NULL)
        real_lstat = dlsym(RTLD_NEXT, "lstat");
    char newpath[PATH_MAX];
    realpath(path, newpath);
    if (inList(newpath, 0))
    {
        // Instead of calling stat on OG file call it on the subset file
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(path);
        strcat(newPath, filebasename);
        int ret = real_lstat(newPath, buf);
        return ret;
    }
    return real_lstat(path, buf);
}
/*
 *
 * Wrapper for stat 
 *
 */
int stat(const char *path, struct stat *buf)
{
    static int (*real_stat)(const char *path, struct stat *buf) = NULL;
    if (real_stat == NULL)
        real_stat = dlsym(RTLD_NEXT, "stat");
    char realPath[PATH_MAX];
    realpath(path, realPath);
    if (inList(realPath, 0))
    {
        // Instead of calling stat on OG file call it on the subset file
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(path);
        strcat(newPath, filebasename);
        int ret = real_stat(newPath, buf);
        return ret;
    }
    return real_stat(path, buf);
}
/*
 *
 * Wrapper for fstat 
 *
 */
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

        int ret = real_fstat(fd, buf);
        engineerStat( cur->path, buf, 1);
        return ret;
    }
}
/*
 *
 * Wrapper for lstat64 
 *
 */
int lstat64(const char *__restrict __file,
            struct stat64 *__restrict __buf)
{
    static int (*real_lstat)(const char *__restrict __file,
            struct stat64 *__restrict __buf) = NULL;
    if (real_lstat == NULL)
        real_lstat = dlsym(RTLD_NEXT, "lstat64");
    char newpath[PATH_MAX];
    realpath(__file, newpath);
    if (inList(newpath, 0))
    {

        // Instead of calling stat on OG file call it on the subset file
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(__file);
        strcat(newPath, filebasename);
        int ret = real_lstat(newPath, __buf);
        return ret;
    }
    return real_lstat(__file, __buf);
}
/*
 *
 * Wrapper for stat64 
 *
 */
int stat64(const char *__restrict path,
           struct stat64 *__restrict buf)
{
    static int (*real_stat64)(const char *__restrict path,
           struct stat64 *__restrict buf) = NULL;
    if (real_stat64 == NULL)
        real_stat64 = dlsym(RTLD_NEXT, "stat64");
    char realPath[PATH_MAX];
    realpath(path, realPath);
    if (inList(realPath, 0))
    {

        // Instead of calling stat on OG file call it on the subset file
        char newPath[PATH_MAX];
        getcwd(newPath, sizeof(newPath));
        strcat(newPath, "/tracelog/subsets/");
        char* filebasename = basename(path);
        strcat(newPath, filebasename);
        int ret = real_stat64(newPath, buf);
        return ret;
    }
    return real_stat64(path, buf);
}
/*
 *
 * Wrapper for stat64 
 *
 */
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
        int ret = real_fstat(fd, buf);
        engineerStat( cur->path, buf, 0);
        return ret;
    }
}