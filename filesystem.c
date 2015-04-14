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
      if(strlen(parentPath) + strlen(tmp) + 1 == strlen(childPath) ){break;}
      strcat(parentPath ,"/");
      strcat(parentPath, tmp);
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

char* childName(char* childPath){
  if(strcmp(childPath, "/") == 0){return childPath;}
  char* previous = malloc(strlen(childPath));
  char* current = malloc(strlen(childPath));
  current = strtok(childPath, "/");
  while(current != NULL){
    previous = current;
    current = strtok(NULL, "/");
  }
  return previous;
}

char* matchNameToLocation(char* name, char* nodeData){
  char* fusedataName = NULL;
  char* comparerName = malloc(100);
  char* fusedataNumber = malloc(5);
  char* tmp = malloc(strlen(nodeData));
  tmp = strtok(nodeData, ",");
  while(tmp != NULL){
    scanf("%*s:%s:%s", comparerName, fusedataNumber);
    if(name == comparerName){
      fusedataName = concat("/fusedata/fusedata.", fusedataNumber);
      break;
    }
  }
  free(comparerName);
  free(fusedataNumber);
  free(tmp);
  return fusedataName;
}

struct fileInode* fileInfo(char* filePath){
  struct fileInode* file = malloc(globalMaxFileSize);
  FILE* fd = fopen(filePath, "r+");
  fscanf(fd, "{size:%s, uid:%s, gid:%s, mode:%s, linkcount:%s, atime:%s, ctime:%s, mtime:%s, indirect:%s, location:%s, name:%s}", file->size, file->uid, file->gid, file->mode, file->linkcount, file->atime, file->ctime, file->mtime, file->indirect, file->location, file->name);
  fclose(fd);
  return file;
}

int nemCreate(char* path, mode_t mode, struct fuse_file_info* fi){
  /*TODO: Open path to file location,
          Obtain next free block,
          Add reference to file in directory,
          Add file inode with appropriate info
  */
  return 0;
}

void* nemDestroy(){
  /*TODO: Close open file,

  */
  return NULL;
}

//from fusexmp.c
int nemGetattr(const char* path, struct stat* stbuf){
  int res;
res = lstat(path, stbuf);
if (res == -1)
        return -errno;
return 0;
}


void* nemInit(struct fuse_conn_info* conn){
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
        fprintf(fd, "{%d", ((j-1) * 400) + i);
        fclose(fd);
      }
      else if(i == 399){
        fd = fopen(freeBlock, "a+");
        fprintf(fd, ",%d}", ((j - 1) * 400) + i );
        fclose(fd);
      }
      fd = fopen(freeBlock, "a+");
      fprintf(fd, ",%d", ((j - 1) * 400) + i );
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

int nemLink(char* from, char* to){
  /*TODO: Parse out 'to' path to get filename & fusedata number,
          Open 'from' path and add linkcount and info to dict
  */
  return 0;
}

int nemMkdir(const char* path, mode_t mode){
  /*TODO: Pull value from free block list,
          Add location to parent directory,
          Assign correct information
  */
  //Get value from free block list
  char* fileLocations;
  char* freeBlock;
  int j;
  for(j = 1; j < 26; ++j){
    char* freeBlockPath = concat(concat(globalFilePath, globalBlockName), j);
    FILE* fd = fopen(freeBlockPath, "r+");
    fscanf(fd, "{size:%*s, uid:%*s, gid:%*s, mode:%*s, atime:%*s, ctime:%*s, mtime:%*s, linkcount:%*s, filename_to_inode_dict:{%s}", fileLocations);
    if(strcmp(fileLocations, "{}") == 0 ){continue;}
    scanf("{%s, %*s}", freeBlock);
    break;
  }

  //Add location to parent directory
  int parentBlock = nemOpendir(parentPath(path));
  struct inode* directory;
  FILE* fd = fopen(concat("/fusedata/fusedata.", parentBlock), "r+");
  fscanf(fd, "{size:%s, uid:%s, gid:%s, mode:%s, atime:%s, ctime:%s, mtime:%s, linkcount:%s, filename_to_inode_dict:{%s}}", directory->size, directory->uid, directory->gid, directory->mode, directory->atime, directory->ctime, directory->mtime, directory->linkcount, directory->filename_to_inode_dict);
  char* additionalEntry = malloc(100);
  additionalEntry = ",d:";
  strcat(additionalEntry, childName(path));
  strcat(additionalEntry, ":");
  strcat(additionalEntry, freeBlock);
  strcat(directory->filename_to_inode_dict, additionalEntry);
  char* link[6];
  sprintf(link, "%d", atoi(directory->linkcount) + 1);
  fprintf(fd, "{size:%s, uid:%s, gid:%s, mode%s, atime:%s, ctime%ld, mtime%ld, linkcount:%s, filename_to_inode_dict:{%s}}",directory->size, directory->uid, directory->gid, directory->mode,directory->atime, time(NULL), time(NULL), link, directory->filename_to_inode_dict);
  fclose(fd);
  free(additionalEntry);

  //Assign info to new directory
  fd = fopen("/fusedata/fusedata.", "w+");
  fprintf(fd, "{size:%s, uid:%s, gid:%s, mode%s, atime:%s, ctime%ld, mtime%ld, linkcount:%s, filename_to_inode_dict:{%s}}","4096", "1", "1", mode,time(NULL), time(NULL), time(NULL), "0", "");
  fclose(fd);


}

//from hello.c
static int nemOpen(const char *path, struct fuse_file_info *fi)
{
        if (strcmp(path, globalFilePath) != 0)
                return -ENOENT;
        if ((fi->flags & 3) != O_RDONLY)
                return -EACCES;
        return 0;
}

int nemOpendir(char* path, struct fuse_file_info* fi){
  /*TODO: Start at root and locate first part of path,
          Find all subsequent portions of path,
          assign opened file off located path to file handle
  */
  if(strcmp(path, "/") == 0){fi->fh = 26; return fi->fh;}
  char* fileLocations = malloc(globalMaxFileSize);
  FILE* fd = fopen("/fusedata/fusedata.26", "r+");
  fscanf(fd, "{size:%*s, uid:%*s, gid:%*s, mode:%*s, atime:%*s, ctime:%*s, mtime:%*s, linkcount:%*s, filename_to_inode_dict:{%s}", fileLocations);
  fclose(fd);
  char* fuseName;
  char* tmp = malloc(strlen(path));
  tmp = strtok(path, "/");
  while(tmp != NULL){
    fuseName = matchNameToLocation(tmp, fileLocations);
    free(fileLocations);
    if(fuseName == NULL){ fi->fh = NULL; return fi->fh;/*return error*/}
    fd = fopen(fuseName, "r+");
    fscanf(fd, "{size:%*s, uid:%*s, gid:%*s, mode:%*s, atime:%*s, ctime:%*s, mtime:%*s, linkcount:%*s, filename_to_inode_dict:{%s}", fileLocations);
    tmp = strtok(NULL, "/");
  }
  int fuseNumber = malloc(globalMaxFileSize);
  scanf(fuseName, "/fusedata/fusedata.%d", fuseNumber);
  free(fuseName);
  fi->fh = fuseNumber;
  return fi->fh;
}

//from hello.c
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

//from hello.c
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

int nemRelease(char* path, struct fuse_file_info* fi){
  /*TODO: Remove reference to open file,
          Remove flags from fi
  */
  return 0;
}

int nemReleasedir(char* path, struct fuse_file_info* fi){
  /*TODO: Remove flags from fi,
          Remove other references to open directory
  */
  return 0;
}

int rename(char* from, char* to){
  /*TODO: Get path of 'from',
          Open file inode and edit reference to name,
          Overwrite file with new name from 'to'
  */
  return 0;
}

int statfs(char* path, struct statvfs* stbuf){
  /*TODO: Open inode from path,
          Apply values from inode to corresponding fields in stbuf
  */
  return 0;
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

  char* tmp = malloc(strlen(directory->filename_to_inode_dict));
  char* fileArray = malloc(directory->filename_to_inode_dict);
  struct fileInode* file = fileInfo(path);
  char* fileName = file->name;
  char* fileLocation = file->location;
  tmp = strtok(directory->filename_to_inode_dict, ",{}");
  char* fileEntry = concat(concat(fileName, ":"), fileLocation);
  while(tmp != NULL){
    if(strcmp(tmp, concat("d:", fileEntry))==0 || strcmp(tmp, concat("f:", fileEntry))==0))){
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
  for(j = 1; j < 26; ++j){
    if(((atoi(fileLocation) + 399)/ 400) == j){
      char* freeBlock = concat(concat(globalFilePath, globalBlockName), j);
      fd = fopen(freeBlock,"a+");
      fputs(fileLocation, fd);
      fclose(fd);
      free(freeBlock);
    }
  }
  fd = fopen(concat(concat(globalFilePath,globalBlockName), fileLocation), "w+");
  char* buffer[globalMaxFileSize];
  memset(buffer, "0", globalMaxFileSize);
  fwrite(buffer, globalMaxFileSize, sizeof(char), fd);
  free(buffer);
  fclose(fd);

}

int write(char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi){
  /*TODO: Open the open file based on fi,
          Seek through file based on offset,
          Write to file the contents of buf


  */
  return 0;
}

struct fuse_operations nemOperations = {
  .init    = nemInit,
  .getattr = nemGetattr,
  .readdir = nemReaddir,
  .open    = nemOpen,
  .read    = nemRead,
  .mkdir   = nemMkdir,
  .opendir = nemOpendir,
  .unlink  = nemUnlink,
};

int main(int argc, char* argv[]){

  printf("Inside Main\n");
  umask(0);
  return fuse_main(argc, argv, &nemOperations, NULL);
}
