/*
  Elan Moyal
  gcc -Wall fileSystem.c `pkg-config fuse --cflags --libs` -o fileSystem
*/

#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS  64

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

const int globalMaxBlocks = 10000;
const int globalMaxFileSize = 4096;
char* globalBlockName = "fusedata.";

char* concat(char* first, char* second){
  char* result = malloc(strlen(first) + strlen(second) + 1);
  strcpy(result, first);
  strcat(result, second);
  return result;
}

void* nemInit(){
  printf("Inside Init\n");
  char* fileNameStart = concat("c:\\", globalBlockName);
  char* fileNameComplete = concat(fileNameStart, "0");
  FILE* fileSystem = fopen(fileNameComplete, "r");
  if(fileSystem != NULL){
    free(fileNameComplete);
    fclose(fileSystem);
    return;
  }
  fclose(fileSystem);
  int i;
  for(i = 0; i < globalMaxBlocks; ++i){
    char* num;
    sprintf(num, "%d", i);
    char* tmpName = concat(fileNameStart, num);
    free(num);
    fileSystem = fopen(tmpName, "wb");
    fwrite("", sizeof(globalMaxFileSize), sizeof(1), fileSystem);
    fclose(fileSystem);
  }
}

struct fuse_operations nemOperations = {
  .init = nemInit
};

int main(int argc, char* argv[]){

  printf("Inside Main\n");

  return fuse_main(argc, argv, &nemOperations, NULL);
}
