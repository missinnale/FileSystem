/*
  Elan Moyal
  gcc -g -Wall filesystem.c `pkg-config fuse --cflags --libs` -o filesystem
  for permission issues:
  sudo chown root /usr/local/bin/fusermount
  sudo chmod u+s /usr/local/bin/fusermount

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
char* globalFilePath = "/fusedata/";

char* concat(char* first, char* second){
  char* result = malloc(strlen(first) + strlen(second) + 1);
  strcpy(result, first);
  strcat(result, second);
  return result;
}

void* nemInit(){
  printf("Inside Init\n");
  char* fileNameStart = concat(globalFilePath, globalBlockName); // /tmp/fuse
  char* superBlock = concat(fileNameStart, "0");
  FILE* fd = fopen(superBlock, "r");
  if(fd != NULL){
    fclose(fd);
    free(superBlock);
    return NULL;
  }
  fclose(fd);

  //Create the 10,000 files with 0's in them via byte write
  printf("Creating Files\n");
  int i;
  for(i = 0; i < 1; ++i){
    char num[6];
    sprintf(num, "%d", i);
    char* tmpName = concat(fileNameStart, num);
    fd = fopen(tmpName, "wb");
    char* buffer[globalMaxFileSize];
    memset(buffer, "0", globalMaxFileSize);
    fwrite(buffer, globalMaxFileSize, sizeof(char), fd);
    fclose(fd);
    free(num);
  }
  printf("Done Creating Files\n");

  //overwrite the superBlock to contain correct information
  printf("Creating Super Block\n");
  fd = fopen(superBlock, "w+");
  fprintf(fd, "{creationTime:%ld, mounted: %i, devId: %i, freeStart: %i,freeEnd: %i, root: %i, maxBlocks: %i}\n", time(NULL), 1, 20, 1, 25, 26, globalMaxBlocks);
  fclose(fd);
  free(superBlock);

  //overwrite the first free block to contain correct information
  printf("Creating Free Block List\n", );
  char* freeBlockStart = concat(fileNameStart, "1");
  for(i = 27; i < 400; ++i){
    if(i == 27){
      fd = fopen(freeBlockStart, "w+");
      fprintf(fd, "{ %d", i);
      fclose(fd);
    }
    else if(i == 399){
      fd = fopen(freeBlockStart, "a+");
      fprintf(fd, ", %d}", i);
      fclose(fd);
    }
    fd = fopen(freeBlockStart, "a+");
    fprintf(fd, ", %d", i);
    fclose(fd);

  }

  //overwrite the remaining free blocks to contain correct information
  int j;
  for(j = 2; j < 26; ++j){
    char* num[6];
    sprintf(num, "%d", j);
    char* freeBlock = concat(fileNameStart, num);
    for(i = 0; i < 400; ++i){
      if(i == 0){
        fd = fopen(freeBlock, "w+");
        fprintf(fd, "{ %d", ((j-1) * 400) + i);
        fclose(fd);
      }
      else if(i == 399){
        fd = fopen(freeBlock, "a+");
        fprintf(fd, ", %d}", ((j - 1) * 400) + i );
        fclose(fd);
      }
      fd = fopen(freeBlock, "a+");
      fprintf(fd, ", %d", ((j - 1) * 400) + i );
      fclose(fd);
    }
  }

  //overwrite the root block to contain correct info
  printf("Creating Root Block\n");
  char* rootBlock = concat(fileNameStart, "26");
  fd = fopen(rootBlock, "w+");
  fprintf(fd, "{size:%d, uid:%d, gid:%d, mode:%d, atime:%ld, ctime:%ld, mtime:%ld, linkcount:%d, filename_to_inode_dict: {d:.:%d, d:..:%d}}", 0, 1, 1, S_IFDIR | S_IRWXU , time(NULL), time(NULL), time(NULL), 2, 26, 26);
  fclose(fd);

  return NULL;
}

int nemGetattr(const char* path, struct stat* stbuf){
  /*
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));
  if(strcmp(path, "/") == 0){
    stbuf -> st_mode = S_IFDIR | 0755;
    stbuf -> st_nlink = 2;
  }
  else if(strcmp(path, globalFilePath) == 0){
    stbuf -> st_mode = S_IFREG | 0777;
    stbuf -> st_nlink = 1;
    stbuf -> st_size = globalMaxFileSize * globalMaxBlocks;
  }
  else{
    res = -ENOENT;
  }
  return res;
  */
  int res;
res = lstat(path, stbuf);
if (res == -1)
        return -errno;
return 0;
}

static int nemReaddir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
        (void) offset;
        (void) fi;
        if (strcmp(path, "/") != 0)
                return -ENOENT;
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, globalFilePath + 1, NULL, 0);
        return 0;
}
static int nemOpen(const char *path, struct fuse_file_info *fi)
{
        if (strcmp(path, globalFilePath) != 0)
                return -ENOENT;
        if ((fi->flags & 3) != O_RDONLY)
                return -EACCES;
        return 0;
}
static int nemRead(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
        int fd;
        int res;
        (void) fi;
        fd = open(path, O_RDONLY);
        if (fd == -1)
                return -errno;
        res = pread(fd, buf, size, offset);
        if (res == -1)
                res = -errno;
        close(fd);
        return res;
}

struct fuse_operations nemOperations = {
  //.init = nemInit,
  .getattr = nemGetattr,
  .readdir = nemReaddir,
  .open    = nemOpen,
  .read    = nemRead,
};

int main(int argc, char* argv[]){

  printf("Inside Main\n");
  umask(0);
  nemInit();
  return fuse_main(argc, argv, &nemOperations, NULL);
}
