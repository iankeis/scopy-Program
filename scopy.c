/*
    Ian Kieswether
    CS426
    Project - scopy
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int fileCopy(const char *sourceFile, const char *targetFile, uid_t euid, uid_t ruid){
    int fd_source;      // file descriptor for source file to be copied
    int fd_target;      // file descriptor for target file that is being copied to
    struct stat stbuf;  // buffer for handling file copy

    seteuid(euid);      // set euid to file owners uid to read protected file

    if((fd_source = open(sourceFile, O_RDONLY)) < 0){
        perror("The source file could not be read");
        return 0;
    }
    seteuid(ruid);      // reset euid to ruid to write copied file as property of copier

    lstat(sourceFile, &stbuf);   // store file stats in stbuf

    // if the file is not a regular file, exit with a failure
    if((stbuf.st_mode & S_IFMT) != S_IFREG){
        printf("the file must be regular\n");
        exit(EXIT_FAILURE);
    }

    if((fd_target = open(targetFile, O_CREAT | O_RDWR, 0700)) < 0){
        perror("write error");
        close(fd_source);
        return 0;
    }

    sendfile(fd_target, fd_source, 0, stbuf.st_size);   // copy contents of stbuf to new file

    close(fd_source);
    close(fd_target);
    return 1;
}

int main(int argc, char const *argv[]){
    if(argc != 3){
        perror("input should be of the form ./scopy [inputfile] [outputfile]");
        exit(EXIT_FAILURE);
    }

    // store euid and ruid up front
    uid_t const euid = geteuid();
    uid_t const ruid = getuid();

    char *userName = getlogin();        // grab username of person running program
    int authorized = 0;

    //////////////////////////////////////////////////////////
    char filePath[255];
    //try changing to memcpy
    strcpy(filePath, argv[1]);
    strcat(filePath, ".acl");

    FILE *fp;            // file pointer for .acl file
    char temp[255];

    seteuid(euid);      // set euid to file owners uid to check config file for authorization

    if((fp = fopen(filePath, "r"))){

        seteuid(ruid);  // restore euid to ruid as soon as file is successfully opened

        while(fscanf(fp, "%s" , temp) > 0){
            if(strcmp(temp, userName) == 0){
                authorized = 1;
            }
        }
    } else {
        printf("The config file doesn't exist or you do not have permission to access it\n");
    }


    // if username found in config file, proceed to copy file
    int success = 0;
    if(authorized){ success = fileCopy(argv[1], argv[2], euid, ruid); }

    if(success){
        printf("The file was successfully copied as \"%s\"\n", argv[2]);
    } else {
        printf("The file was was not copied\n");
        return 1;
    }
    return 0;
}






/*
1. switch strcat and strcpy to safer versions strncat strncpy or memcpy
2. check for symlinks using lstat
3. check file permissions and make sure config file has same owner as the file being copied
*/
