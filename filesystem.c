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

struct inode{
  char* size;
  char* uid;
  char* gid;
  char* mode;
  char* atime;
  char* ctime;
  char* mtime;
  char* linkcount;
  char* filename_to_inode_dict;
};

struct fileInode{
  char* size;
  char* uid;
  char* gid;
  char* mode;
  char* linkcount;
  char* atime;
  char* ctime;
  char* mtime;
  char* indirect;
  char* location;
  char* name;
};

char* concat(char* first, char* second){
  char* result = malloc(strlen(first) + strlen(second) + 1);
  strcpy(result, first);
  strcat(result, second);
  return result;
}



char* parentPath(char* childPath){
  char* root = "/";
  char* parentPath = malloc(strlen(childPath));
  char* tmp = malloc(strlen(childPath));
  tmp = strtok(childPath, "/");
  if(strlen(tmp + 1) != strlen(childPath)){
    while(tmp != NULL){
      parentPath = tmp;
      tmp = strtok(NULL, "/");
    }
    strcat(root, parentPath);
    parentPath = root;
    free(root);
    free(tmp);
  }
  else{
    parentPath = childPath;
  }

  return parentPath;
}

struct fileInode* fileInfo(char* filePath){
  struct fileInode* file = malloc(globalMaxFileSize);
  FILE* fd = fopen(filePath, "r+");
  fscanf(fd, "{size:%s, uid:%s, gid:%s, mode:%s, linkcount:%s, atime:%s, ctime:%s, mtime:%s, indirect:%s, location:%s, name:%s}", file->size, file->uid, file->gid, file->mode, file->linkcount, file->atime, file->ctime, file->mtime, file->indirect, file->location, file->name);
  fclose(fd);
  return file;
}



void* nemInit(){
  printf("Inside Init\n");
  char* fileNameStart = concat(globalFilePath, globalBlockName);
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
  for(i = 0; i < globalMaxBlocks; ++i){
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
  printf("Creating Free Block List\n");
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

int nemMkdir(const char* path, mode_t mode){
  /*TODO: Pull value from free block list,
          Add location to parent directory,
          Assign correct information
  */
  
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

int nemUnlink(const char* path){
  /*TODO: parse through path name to get containing folder,
          remove filename_to_inode_dict entry for file in containing folder,
          decrease linkcount by 1 of containing folder,
          add location of file to free block list,
          change path of file to the path of trashbin
  */
  char* parent = parentPath(path);
  FILE* fd = fopen(parent, "r+");
  struct inode* directory;
  fscanf(fd, "{size:%s, uid:%s, gid:%s, mode:%s, atime:%s, ctime:%s, mtime:%s, linkcount:%s, filename_to_inode_dict:{%s}", directory->size, directory->uid, directory->gid, directory->mode, directory->atime, directory->ctime, directory->mtime, directory->linkcount, directory->filename_to_inode_dict);
  fclose(fd);
  fd = fopen(parent, "w+");

  char* tmp = malloc(directory->filename_to_inode_dict);
  char* fileArray = malloc(directory->filename_to_inode_dict);
  struct fileInode* file = fileInfo(path);
  char* fileName = file->name;
  char* fileLocation = file->location;
  tmp = strtok(directory->filename_to_inode_dict, ",{}");
  char* fileEntry = concat(concat(fileName, ":"), fileLocation);
  while(tmp != NULL){
    if(strcmp(tmp, concat("d:", fileEntry)==0 || strcmp(tmp, concat("f:", fileEntry)==0))){
      continue;
    }
    strcat(fileArray, ",");
    strcat(fileArray, tmp);
  }
  directory->filename_to_inode_dict = fileArray;
  char* link[6];
  sprintf(link, "%i", atoi(directory->linkcount) - 1);
  fprintf(fd,"size:%s, uid:%s, gid:%s, mode:%s, atime:%s, ctime:%ld, mtime:%ld, linkcount:%s, filename_to_inode_dict:{%s}", directory->size, directory->uid, directory->gid, directory->mode, directory->atime, time(NULL), time(NULL), link , directory->filename_to_inode_dict);
  fclose(fd);

  int j;
  int i = 0;

  for(j = 1; j < 26; ++j){
    if(((atoi(fileLocation) + 399)/ 400) == j){
      char* freeBlock = concat(concat(globalFilePath, globalBlockName), j);
      fd = fopen(freeBlock,"a+");
      fputs(fileLocation, fd);
      fclose(fd);
      free(freeBlock);
    }
  }
  fd = fopen(concat(concate(globalFilePath,globalBlockName), fileLocation), "w+");
  char* buffer[globalMaxFileSize];
  memset(buffer, "0", globalMaxFileSize);
  fwrite(buffer, globalMaxFileSize, sizeof(char), fd);
  free(buffer);
  fclose(fd);

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
