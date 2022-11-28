#define _GNU_SOURCE
#include "traceDataHandler.h"
#include <dlfcn.h>
/*
 *
 *
 * Function to check if system has been setup
 *
 */
int checkSetup()
{
    return setup;
}
/*
 *
 *
 * Function to setup the system before first call
 *
 */
void finishSetup()
{
    // make directory to store backups
    mkdir("backups", 0x0700);
}

/*
 *
 * Function to set a file to open. If a new file create a new structure to hold its details
 * Either FD is supplied or FPTR is supplied
 * @param filename  Name of file to open
 * @param path Path of file to open
 * @param fd File descriptor of file
 * @param flags Flags of the file that was opened
 * @param fptr File pointer of file to be opened
 *
 */
int openFile(const char *filename, char *path, int fd, int flags, FILE *fptr)
{
    fileTraceObject *newFile;
    HASH_FIND_STR(fileTrace, path, newFile);
    if (newFile == NULL)
    {
        newFile = malloc(sizeof(fileTraceObject));
        strcpy(newFile->path, path);
        strcpy(newFile->filename, filename);
        newFile->backupSize = 0;
        newFile->firstCall = NULL;
        newFile->lastCall = NULL;
        newFile->readList = NULL;
        newFile->firstVerstion = NULL;
        newFile->lastVersion = NULL;
        newFile->status = 1;
        newFile->fileDescriptor = fd;
        newFile->filePointer = 0;
        newFile->flags = flags;
        newFile->fptr = fptr;
        newFile->size = 0;
        HASH_ADD_STR(fileTrace, path, newFile);
        return 1;
    }
    else
    {
        newFile->status = 1;
        newFile->fileDescriptor = fd;
        newFile->filePointer = 0;
        newFile->flags = flags;
        newFile->fptr = fptr;
        return 1;
    }
}

/*
 *
 *
 * Function to set a file to closed
 * @param path  Path to the file
 *
 */
void setToClose(char *path)
{
    fileTraceObject *newFile;
    HASH_FIND_STR(fileTrace, path, newFile);
    if (newFile != NULL)
    {
        newFile->status = 0;
        newFile->fileDescriptor = -1;
        newFile->filePointer = -1;
        newFile->fptr = NULL;
        flushToFile(newFile);
    }
}
// if type = 0 change the file pointer
// if 1 then change the file pointer
/*
 *
 *
 * Function to log a read call
 * @param path Path to the file
 * @param fd FD of the file
 * @param type 0 if we are not supplied an offset 1 if we are supplied an offset
 * @param size Size of the read
 *
 */
void logRead(char *path, int fd, int type, off_t size, off_t offset)
{
    fileTraceObject *file;
    HASH_FIND_STR(fileTrace, path, file);
    if (file != NULL)
    {
        // if(file->head == NULL){
        //     file->head = heap;
        //     file->tail = heap;
        // }else{
        //     HeapLocations* cur = file->head;
        //     while(cur!=NULL){
        //         if(cur->next==NULL)
        //     }
        // }
        Call *newCall = malloc(sizeof(Call));
        if (type == 0)
        {
            newCall->offset = file->filePointer;
            file->filePointer += size;
        }
        else
        {
            newCall->offset = offset;
        }
        newCall->type = 1;     // 1 = write
        newCall->location = 3; // pread = 3
        newCall->nextCall = NULL;
        newCall->opSize = size;
        newCall->timestamp = logicalTimestamp++;
        if (file->firstCall == NULL)
        {
            file->firstCall = newCall;
            file->lastCall = newCall;
        }
        else
        {
            file->lastCall->nextCall = newCall;
            file->lastCall = newCall;
        }
        GlobalReadList *head = file->readList;
        if (head == NULL)
        {
            head = malloc(sizeof(GlobalReadList));
            head->start = newCall->offset;
            head->end = newCall->offset + newCall->opSize;
            head->next = NULL;
            file->readList = head;
        }
        else
        {
            GlobalReadList *newHead = addtoReadList(newCall, head);
            file->readList = newHead;
        }
    }
}
/*
 *
 *
 * Function to take a read call and add it to the combined read list
 * which keeps a track of all the total offsets that have been read
 * @param call A call structure of the new read Call
 * @param head Head of the linked list of the reaf offsets
 * @return new head of the linked list
 *
 */
GlobalReadList *addtoReadList(Call *call, GlobalReadList *head)
{
    GlobalReadList *newHead = head;
    GlobalReadList *cur = head;

    off_t readStart = call->offset;
    off_t readEnd = call->offset + call->opSize;
    off_t curStart = head->start;
    off_t curEnd = head->end;

    GlobalReadList *newRead = malloc(sizeof(GlobalReadList));
    newRead->start = readStart;
    newRead->end = readEnd;
    // Add to begingin of list
    if (readStart < curStart)
    {
        newHead = newRead;
        newRead->next = head;
    }
    else
    {
        while (cur != NULL)
        {
            if (cur->next == NULL)
            {
                newRead->next = NULL;
                cur->next = newRead;
                break;
            }
            if (cur->next->start > newRead->start)
            {
                newRead->next = cur->next;
                cur->next = newRead;
                break;
            }
            cur = cur->next;
        }
    }

    cur = newHead;
    while (cur != NULL)
    {

        if (cur->next == NULL)
        {
            return newHead;
        }
        if (cur->end >= cur->next->start)
        {
            cur->end = cur->end > cur->next->end ? cur->end : cur->next->end;
            GlobalReadList *temp = cur->next;
            cur->next = cur->next->next;
            free(temp);
        }
        else
        {
            cur = cur->next;
        }
    }
    return newHead;
}

/*
 *
 *
 * Function to change the file pointer in the backend structure
 * when lseek is performed
 * @param path PAth to file
 * @param offset New offset
 * @param whence Mode of lssek
 *
 */
void performLseek(char *path, off_t offset, int whence)
{
    fileTraceObject *file;
    HASH_FIND_STR(fileTrace, path, file);
    if (file != NULL)
    {
        if (whence == SEEK_SET)
        {
            file->filePointer = (int)offset;
        }
        else if (whence == SEEK_CUR)
        {
            file->filePointer += (int)offset;
        }
        // Take care of SEEK_END too later
    }
}

/*
 *
 *
 * Function to log a read and perform a backup if there is an overlap
 * between the write and a previous read
 * @param path Path to file the op is on
 * @param type 0 means we are given an offset and 1 means we are using the file pointer
 * @param nbytes Size of the write
 * @param offset Offset of the write if supplied
 *
 *
 */
void performBackup(char *path, int type, off_t nbytes, off_t offset)
{
    fileTraceObject *file;
    HASH_FIND_STR(fileTrace, path, file);
    int writeStart, writeEnd, readStart, readEnd;
    if (file != NULL)
    {
        // Type = 0 means we are given an offset
        if (type == 0)
        {
            writeStart = offset;
            // Else we take the current file pointer
        }
        else
        {
            writeStart = file->filePointer;
        }
        // Set the end of the write and other metadata
        writeEnd = writeStart + nbytes;
        Call *newCall = malloc(sizeof(Call));
        newCall->location = 0;
        newCall->offset = writeStart;
        newCall->opSize = writeEnd - writeStart;
        newCall->timestamp = logicalTimestamp++;
        newCall->nextCall = NULL;
        newCall->type = 0;
        // Change the tail of the call pointers if it already has a head
        if (file->firstCall != NULL)
        {
            file->lastCall->nextCall = newCall;
            file->lastCall = newCall;
        }
        // Add a new head
        else
        {
            file->firstCall = newCall;
            file->lastCall = newCall;
        }
        GlobalReadList *head = file->readList;
        GlobalReadList *cur = head;
        GlobalReadList *prev = head;
        // Loop through all read segments and check for overlap
        while (cur != NULL)
        {
            readStart = cur->start;
            readEnd = cur->end;
            if (checkOverlap(readStart, readEnd, writeStart, writeEnd))
            {
                // wS = write Start rS = read Start wE = write End rE = read End

                // Case 1 wS rS wE rE = change rS to wE
                if (writeStart <= readStart && writeEnd > readStart && writeEnd < readEnd)
                {
                    cur->start = writeEnd;
                    storeVersion(file, path, readStart, writeEnd);
                    break;
                    // Case 2 rS wS rE wE = change rE to wS
                }
                else if (readStart < writeStart && readEnd > writeStart && readEnd < writeEnd)
                {
                    cur->end = writeStart;
                    storeVersion(file, path, writeStart, readEnd);
                    prev = cur;
                    cur = cur->next;
                    // Case 3 wS rS rE wE = Remove the full read
                }
                else if (writeStart < readStart && readEnd < writeEnd)
                {
                    if (cur == head)
                    {
                        // If we are completely removing the head we have to create a new head
                        file->readList = head->next;
                        free(head);
                        head = file->readList;
                        cur = head;
                    }
                    else
                    {
                        // Remove the current element from the read List linked list
                        prev->next = cur->next;
                        free(cur);
                        cur = prev->next;
                    }
                    storeVersion(file, path, readStart, readEnd);
                    // Case 4 rS wS wE rE = split to 2 wSrS and rEwE
                }
                else if (readStart < writeStart && writeEnd < readEnd)
                {
                    GlobalReadList *newElement = malloc(sizeof(GlobalReadList));
                    newElement->start = writeEnd;
                    newElement->end = readEnd;
                    newElement->next = cur->next;
                    cur->next = newElement;
                    cur->end = writeStart;
                    storeVersion(file, path, writeStart, writeEnd);
                    break;
                }
            }
            if (cur == NULL)
                break;
            prev = cur;
            cur = cur->next;
        }
    }
}
/*
 *
 *
 * Function to check if the two intervals overlap
 * @param s1 Start of the first interval
 * @param e1 End of the first interval
 * @param s2 Start of the second interval
 * @param e2 End of the second interval
 * @return 1 if it overlaps 0 if it doesn't
 *
 *
 */
int checkOverlap(off_t s1, off_t e1, off_t s2, off_t e2)
{
    return ((s1 <= e2) && (s2 <= e1));
}

/*
 *
 *
 * Function to store a version of data before it is overwritten
 * @param file File structure that holds all metadata of the file
 * @param path Path to the file
 * @param start Start offset of the data to be stored
 * @param end End offset of the data to be stored
 *
 */
void storeVersion(fileTraceObject *file, char *path, off_t start, off_t end)
{
    static int (*real_open)(const char *, int) = NULL;
    if (!real_open)
        real_open = dlsym(RTLD_NEXT, "open");
    int fd = real_open(path, 0);

    static off_t (*real_lseek)(int fildes, off_t offset, int whence) = NULL;
    if (!real_lseek)
        real_lseek = dlsym(RTLD_NEXT, "lseek");
    real_lseek(fd, start, SEEK_SET);

    char *buf = malloc(end - start);
    static ssize_t (*real_read)(int fd, void *buf, size_t count) = NULL;
    if (real_read == NULL)
        real_read = dlsym(RTLD_NEXT, "read");

    int ret = real_read(fd, buf, end - start);
    static int (*real_close)(int fd) = NULL;
    if (!real_close)
        real_close = dlsym(RTLD_NEXT, "close");
    real_close(fd);

    static ssize_t (*real_write)(int fildes, const void *buf, size_t nbytes) = NULL;
    if (!real_write)
        real_write = dlsym(RTLD_NEXT, "write");
    
    chdir("backups");
    // Grab the base file name from the path
    char *bname = strrchr(file->path, '/');
    FILE *fptr = fopen(bname + 1, "a");
    fwrite(buf, end - start, 1, fptr);
    fclose(fptr);
    chdir("..");

    versionedData *newVersion = malloc(sizeof(versionedData));
    newVersion->origStart = start;
    newVersion->backupfileStart = file->backupSize;
    file->backupSize += end - start;
    newVersion->size = ret;
    newVersion->timestamp = logicalTimestamp;
    newVersion->next = NULL;
    if (file->firstVerstion == NULL)
    {
        file->firstVerstion = newVersion;
        file->lastVersion = newVersion;
    }
    else
    {
        file->lastVersion->next = newVersion;
        file->lastVersion = newVersion;
    }
}
/*
 *
 *
 * Function to flush the metadata to a file
 * @param file File metadata object to flush to file
 *
 */
void flushToFile(fileTraceObject *file)
{

    int find = '/';
    char *path = basename(file->filename);
    chdir("tracelog");
    FILE *fptr;
    if (path == NULL)
        fptr = fopen(file->filename, "w");
    else
        fptr = fopen(path, "w");
    if (fptr == NULL)
        fprintf(stdout, "Is null\n");
    fprintf(fptr, "%s:%ld\n", file->path, file->size);
    fprintf(fptr, "ReadList\n");
    GlobalReadList *head = file->readList;
    char buf[100];
    while (head != NULL)
    {
        fprintf(fptr, "%ld:%ld\n", head->start, head->end);
        head = head->next;
    }
    fprintf(fptr, "CallList\n");
    Call *call = file->firstCall;
    while (call != NULL)
    {
        fprintf(fptr, "%d:%ld:%ld:%d:%d\n", call->location, call->offset, call->opSize, call->timestamp, call->type);
        call = call->nextCall;
    }
    fprintf(fptr, "Backups\n");
    versionedData *version = file->firstVerstion;
    while (version != NULL)
    {
        fprintf(fptr, "%ld:%ld:%ld:%ld\n", version->backupfileStart, version->origStart, version->size, version->timestamp);
        version = version->next;
    }
    fclose(fptr);
    chdir("..");
}

/*
 *
 *
 * Function to capture the required info on the file when a stat call is made
 * @param path Path to the file
 * @param buf Generic pointer to stat structure
 * @param loc = 1 when stat structure loc = 0 when stat64 structure
 *
 */
int captureStat(char *path, void *buf, int loc)
{
    fileTraceObject *file;
    HASH_FIND_STR(fileTrace, path, file);
    if (file != NULL)
    {
        if (loc == 1)
        {
            struct stat *stats = (struct stat *)buf;
	    if(file->size<stats->st_size)
            file->size = stats->st_size;
        }
        else
        {
            struct stat64 *stats = (struct stat64 *)buf;
	    if(file->size<stats->st_size)
	    file->size = stats->st_size;
        }
    }
    return 1;
}
