
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <iostream>
#include <string>

#include "fslib.hpp"
#include "vfs.hpp"
#include "tokenizer.hpp"
#include "mysh.hpp"

using namespace std;


Tokenizer g_filename_tokenizer(" /", "");
int g_max_num_child_in_block = BLOCKSIZE / sizeof(Vnode);
Vnode *curVnode;


Vnode* findVnode(vector<string>& filenames, int type) {
  int res;

  Vnode* curFile;
  int start = 0;

  if (filenames[0] == "~") {
    start = 1;
    curVnode = g_root_directory;
  }

  bool fileExist = false;
  for (int i = start; i < filenames.size(); i++) {
    if (curVnode->type == 1 && curVnode->size > 0) {
      int numChild = curVnode->size;
      int curPtr = curVnode->fatPtr;

      while (curPtr != EOBLOCK && numChild > 0) {
	bool foundDir = false;

	for (int j = 0; j < g_max_num_child_in_block; j++) {
	  if (! numChild) {
	    break;
	  }
	  numChild--;

	  char vnodeName[255];

	  res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + curPtr) + j * sizeof(Vnode), SEEK_SET);
	  if (res == -1) {
	    perror("lseek child vnode failed");
	    return NULL;
	  }

	  res = read(g_disk_fd, vnodeName, 255);
	  if (res < 255) {
	    perror("read child name failed");
	    return NULL;
	  }

	  if (filenames[j] == string(vnodeName)) {
	    Vnode* newVnode = new Vnode;
	    res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + curPtr) + j * sizeof(Vnode), SEEK_SET);
	    if (res == -1) {
	      perror("lseek child vnode failed");
	      return NULL;
	    }

	    res = read(g_disk_fd, newVnode, sizeof(Vnode));
	    if (res < sizeof(Vnode)) {
	      perror("read child vnode failed");
	      return NULL;
	    }

	    newVnode->address = BLOCKSIZE * (g_superblock.data_offset + curPtr) + j * sizeof(Vnode);
	    newVnode->parent = curVnode;
	    curVnode = newVnode;

	    if (type == 1 && i == filenames.size()-1){
	      return newVnode;
	    }
	    foundDir = true;
	    break;
	  }
	}

	if (foundDir) break;

	curPtr = g_fat_table[curPtr];
      }
    } else {
      if (i == filenames.size() - 1 && type == 0) {
	curFile = curVnode;
	curVnode = curVnode->parent;
	fileExist = true;
	break;
      } else {
	//errno
	return NULL;
      }
    }
  }

  if (! fileExist) {
    return NULL;
  }

  return curFile;
}


int f_open(const char *filename, int flags) {
  int res; 

  vector<string> filenames;
  g_filename_tokenizer.parseString(string(filename), filenames);
  for (auto& s : filenames) 
    cout << "|" << s << "|" << endl;


  Vnode* curFile = findVnode(filenames, 0);
  cout << curFile << endl;
  // cout << string(curFile->name) << endl;
  if (! curFile && ((flags & F_READ) || (flags & F_RDWR))) {
    // errno
    return -1;
  }

  if (! curFile) {
    unsigned short free_fat = g_fat_table.getNextFreeBlock();
    if (! free_fat) {
      //errno
      return -1;
    }

    
    g_fat_table[free_fat] = EOBLOCK;

    int ptr = curVnode->fatPtr;
    while (g_fat_table[ptr] != EOBLOCK) {
      ptr = g_fat_table[ptr];
    }

    int vnodeAddr;
    if (curVnode->size % g_max_num_child_in_block == 0) {
      unsigned short nextFreeBlock = g_fat_table.getNextFreeBlock();
      if (! nextFreeBlock) {
	//errno
	return -1;
      }

      vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + nextFreeBlock);
    }
    else {
      vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + ptr) + curVnode->size % g_max_num_child_in_block * sizeof(Vnode);
    }

    curFile = new Vnode(filenames.back(), 0, 0, 0, vnodeAddr, curVnode, 0666, 0, time(NULL), free_fat);

    res = lseek(g_disk_fd, vnodeAddr, SEEK_SET);
    if (res == -1) {
      perror("lseek new vnode address failed");
      return -1;
    }

    res = write(g_disk_fd, curFile, sizeof(Vnode));
    if (res < sizeof(Vnode)) {
      perror("write new vnode into parent directory data block failed");
      return -1;
    }

    curVnode->size++;
    lseek(g_disk_fd, curVnode->address, SEEK_SET);
    write(g_disk_fd, curVnode->address + 264, sizeof(int));
  }

  int idx = g_file_table.getNextIndex();
  if (idx == -1) {
    // errno;
    return -1;
  }

  g_file_table.addFileEntry(FtEntry(idx, curFile, 0, flags));


  return idx;
}


int f_close(int fd){
  if (fd < 0 || fd >= MAXFTSIZE){
    //error message
    return -1;
  }
  
  if (ftable[fd].index != fd){
    //error message
    return -1;
  }

  Vnode *buffer = ftable[fd].vnode;
  while (buffer != root){
    Vnode *del = buffer;
    buffer = buffer->parent;
    delete(del);
  }
  
  g_file_table.removeFileEntry(fd);

  return 0;
}

static int find_fat(unsigned short& start, int& offset) {
  int n = offset / BLOCKSIZE;
  for (int i = 0; i < n; i++) {
    if (! start) {
      //errno
      return -1;
    }
    start = g_fat_table[start];
  }
  offset %= BLOCKSIZE;
  return 0;
}

size_t 	f_read(void *data, size_t size, int num, int fd) {
  int res;

  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    return 0;
  }

  if (! (entry->flag & F_READ) && ! (entry->flag & F_RDWR)) {
    // errno
    return 0;
  }

  Vnode* vnode = entry->vnode;
  unsigned short fat_pos = vnode->fatPtr;
  int offs = entry->offset;

  if (find_fat(fat_pos, offs) == -1) {
    //errno
    return 0;
  }

  int i = 0, buf_pos = 0;
  do {
    //what's this?
    res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos + offs, SEEK_SET);
    if (res == -1) {
      // errno
      return 0;
    }

    if (size < BLOCKSIZE - offs) {
      res = read(g_disk_fd, (char*)data + buf_pos, size);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

			
    }
    else {
      int size_cur_block = BLOCKSIZE - offs;
      res = read(g_disk_fd, (char*)data + buf_pos, size_cur_block);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

      if (g_fat_table[fat_pos] == EOBLOCK) {
	break;
      }

      fat_pos = g_fat_table[fat_pos];
      offs = 0;
      //what's this
      res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos, SEEK_SET);
      if (res == -1) {
	// errno
	return 0;
      }

      res = read(g_disk_fd, (char*)data + buf_pos, size - size_cur_block);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;
    }

    i++;

  } while (i < num);

  return i;
}

size_t 	f_write(Vnode *vn, void *data, size_t size, int num, int fd) {
  int res;

  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    return 0;
  }

  if (! (entry->flag & F_WRITE) && ! (entry->flag & F_RDWR)) {
    // errno
    return 0;
  }

  Vnode* vnode = entry->vnode;
  unsigned short fat_pos = vnode->fatPtr;
  int offs = entry->offset;

  if (find_fat(fat_pos, offs) == -1) {
    //errno
    return 0;
  }

  int i = 0, buf_pos = 0;
  do {
    //what's this
    res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos + offs, SEEK_SET);
    if (res == -1) {
      // errno
      return 0;
    }

    if (size < BLOCKSIZE - offs) {
      res = write(g_disk_fd, (char*)data + buf_pos, size);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

    }
    else {
      int size_cur_block = BLOCKSIZE - offs;
      res = write(g_disk_fd, (char*)data + buf_pos, size_cur_block);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

      if (g_fat_table[fat_pos] == EOBLOCK && size - size_cur_block > 0) {
	unsigned short free_fat = g_fat_table.getNextFreeBlock();
	if (! free_fat) {
	  // errno
	  return i;
	}
	g_fat_table[fat_pos] = free_fat;
	g_fat_table[free_fat] = EOBLOCK;
      }

      fat_pos = g_fat_table[fat_pos];
      offs = 0;
      //what's this??
      res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos, SEEK_SET);
      if (res == -1) {
	// errno
	return 0;
      }

      res = write(g_disk_fd, (char*)data + buf_pos, size - size_cur_block);
      if (res == -1) {
	// errno
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;
    }

    i++;

  } while (i < num);

  if (entry->offset > vnode->size) {
    vnode->size = entry->offset;
  }
  vnode->timestamp = time(NULL);

  return i;
}



int f_seek(Vnode *vn, int offset, int whence, int fd) {
  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    return -1;
  }

  Vnode* cur_node = entry->vnode;

  if (whence == S_CUR) {
    offset += entry->offset;
  }
  else if (whence == S_END) {
    offset = entry->offset - offset; 
  }

  if (offset <= 0) {
    entry->offset = 0;
    return 0;
  }

  if (offset >= cur_node->size) {
    entry->offset = cur_node->size;
    return 0;
  }

  entry->offset = offset;
  return 0;
}



int f_rewind(Vnode *vn, int fd) {
  return f_seek(vn, 0, S_SET, fd);
}



int f_stat(Vnode *vn, Stat *buf, int fd) {
  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    return -1;
  }

  Vnode* vnode = entry->vnode;
  strncpy(buf->name, vnode->name, 255);
  buf->uid = vnode->uid;
  buf->gid = vnode->gid;
  buf->size = vnode->size;
  buf->permission = vnode->permission;
  buf->type = vnode->type;
  buf->timestamp = vnode->timestamp;
  buf->fatPtr = vnode->fatPtr;

  return 0;
}

int f_remove(const char *filename){
  //check if being opened to close
  vector<string> names = new vector<string>();
  parseLine(filename, names);
  int ftIndex = -1;

  for (int i = 0; i < MAXFTSIZE; i++){
    Vnode* entryVnode = g_file_table.getFileEntry(i)->vnode;
    if (entryVnode->name.compare(names.back()) == 0){
      int j;
      for (j = names.size()-1; j >= 0; j--){
	if (entryVnode->name.compare(names[j]) != 0){
	  break;
	} else {
	  entryVnode = entryVnode->parent;
	}
      }
      if (j < 0){
	ftIndex = i;
      }
    }
  }

  if (ftIndex >= 0){
    //mark data blocks as available
    int ptr = g_file_table.getFileEntry(i)->vnode->fatPtr;
    while (ptr != USMAX){
      ptr = g_FAT_table[ptr];
      g_FAT_table[ptr] = USMAX;
    }
    f_close(ftIndex);
  }

  return 0;
}


int f_opendir(const char *filename) {
  int res; 

  vector<string> filenames;
  g_filename_tokenizer.parseString(string(filename), filenames);

  Vnode* curFile = findVnode(filenames, 1);
  if (! curFile) {
    // errno
    return -1;
  }

  int idx = g_file_table.getNextIndex();
  if (idx == -1) {
    // errno;
    return -1;
  }

  g_file_table.addFileEntry(FtEntry(idx, curFile, 0, 0));

  return idx;
}

Stat f_readdir(int dirfd){
  FtEntry* direntry = g_file_table.getFileEntry(dirfd);
  int nextidx = direntry->offset;
  int curPtr = direntry->vnode->fatPtr;
  Vnode *entry = NULL;
  
  while (curPtr != USMAX && nextidx >= 0){
    for (int j = 0; j < 7; i++){
      if (nextidx == 0){
	entry = (Vnode*) new (sizeof(Vnode));
	lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + curPtr) + j * sizeof(Vnode), SEEK_SET);
	read(g_disk_fd, entry, sizeof(Vnode));
	foundEntry = 1;
	break;
      }
      nextidx--;
    }
    if (entry != NULL) break;
    curPtr = g_FAT_table[curPtr];
  }

  Stat res = {entry->name, entry->uid, entry->gid, entry->size, entry->permission, entry->type, entry->timestamp, entry->fatPtr};
  return res;
}

int f_closedir(int dirfd){
  return f_close(dirfd);
}

//also need mode as permission bits
int f_mkdir(char* filename){
  vector<string> filenames;
  g_filename_tokenizer.parseString(string(filename), filenames);
  char *name = filenames.pop_back();
  Vnode *newDirParent = findVnode(filenames, 1);

  unsigned short free_fat = g_fat_table.getNextFreeBlock();
  if (! free_fat) {
    //errno
    return -1;
  }

  Vnode *curFile = new Vnode(name, 0, 0, 0, newDirParent, 0666, time(NULL), free_fat);
  g_fat_table[free_fat] = EOBLOCK;

  int ptr = newDirParent->fatPtr;
  while (g_fat_table[ptr] != EOBLOCK) {
    ptr = g_fat_table[ptr];
  }

  int vnodeAddr;
  if (newDirParent->size % g_max_num_child_in_block == 0) {
    unsigned short nextFreeBlock = g_fat_table.getNextFreeBlock();
    if (! nextFreeBlock) {
      //errno
      return -1;
    }
    vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + nextFreeBlock);
  }
  else {
    vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + ptr) + newDirParent->size % g_max_num_child_in_block * sizeof(Vnode);
  }

  res = lseek(g_disk_fd, vnodeAddr, SEEK_SET);
  if (res == -1) {
    perror("lseek new vnode address failed");
    return -1;
  }

  res = write(g_disk_fd, curFile, sizeof(Vnode));
  if (res < sizeof(Vnode)) {
    perror("write new vnode into parent directory data block failed");
    return -1;
  }

  newDirParent->size++;
  lseek(g_disk_fd, newDirParent->address + 264, SEEK_SET);
  write(g_disk_fd, newDirParent->size, sizeof(int));
  return 0;
}


int f_rmdir(const char* filename){
  //find parent dir: edit # of children, replace Vnode for current dir with last dir & edit Fat
  //find all data blocks used by current dir and its files?
  
  vector<string> filenames;
  g_filename_tokenizer.parseString(string(filename), filenames);
  Vnode *delDir = findVnode(filenames, 1);
  if (delDir == NULL) return -1;
  Vnode *delDirParent = delDir->parent;
  if (delDirParent == NULL) return -1;

  //find delDir address
  
  int prev;
  int ptr = delDirParent->fatPtr;
  while (g_FAT_table[ptr] != EOBLOCK){
    prev = ptr;
    ptr = g_FAT_table[ptr];
  }
  
  int blockIndex = delDirParent->size % g_max_num_child_in_block;
  if (blockIndex == 0){
    Vnode *last = new Vnode;
    g_FAT_table[prev] = EOBLOCK;
    g_FAT_table[ptr] = USMAX;
    lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr), SEEK_SET);
    read(g_disk_fd, last, sizeof(Vnode));
    lseek(g_disk_fd, delDir->address, SEEK_SET);
    write(g_disk_fd, last, sizeof(Vnode));
    delete(last);
  } else {
    Vnode *last = new Vnode;
    lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + blockIndex * sizeof(Vnode), SEEK_SET);
    read(g_disk_fd, last, sizeof(Vnode));
    lseek(g_disk_fd, delDir->address, SEEK_SET);
    write(g_disk_fd, last, sizeof(Vnode));
    delete(last);
  }
  
  vector<int> usedBlocks = new vector<int>();
  trackBlocks(usedBlocks, delDir);

  for (int i = 0; i < usedBlocks.size(); i++){
    g_FAT_table[usedBlocks[i]] = USMAX;
  }

  return 0;
}

void trackBlocks(vector<int>& blocks, Vnode *node){
  int ptr = node->fatPtr;

  while (ptr != EOBLOCK){
    usedBlocks.push_back(ptr);
    if (node->type == 1){
      for (int i = 0; i < g_max_num_child_in_block; i++){
	Vnode *child = new Vnode;
	lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + i * sizeof(Vnode), SEEK_SET);
	read(g_disk_fd, child, sizeof(Vnode));
	trackBlocks(blocks, child);
	delete(child);
      }
    }
    ptr = g_FAT_table[ptr];
  }

  return;
}
