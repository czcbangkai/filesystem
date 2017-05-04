
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



Tokenizer 	g_filename_tokenizer(" /", "");
int 		g_max_num_child_in_block = BLOCKSIZE / sizeof(Vnode);
Vnode* 		curVnode;


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
      cout << "numChild: " << numChild << endl;
      cout << "curPtr: " << curPtr << endl;


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
	  cout << __LINE__ << ": " << "vnodeName: |" << string(vnodeName) << "|" << endl; 

	  if (filenames[i] == string(vnodeName)) {
	    cout << "same name here!" << endl;
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
	    cout << "got here" << endl;
	    break;
	  }
	}

	if (foundDir) {
	  break;
	}

	curPtr = g_fat_table[curPtr];
      }
    }
		
    if (i == filenames.size() - 1 && ! curVnode->type) {
      curFile = curVnode;
      curVnode = curVnode->parent;
      fileExist = true;
      break;
    }
    else {
      //errno
      return NULL;
    }
  }

  if (! fileExist) {
    return NULL;
  }

  cout << "got here!" << endl;

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
    write(g_disk_fd, curVnode, sizeof(Vnode));		
  }

  int idx = g_file_table.getNextIndex();
  if (idx == -1) {
    // errno;
    return -1;
  }

  g_file_table.addFileEntry(FtEntry(idx, curFile, 0, flags));

  return idx;
}

int f_close(int fd) {
  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    print_error(__LINE__, __FUNCTION__, "no such entry");
    return 0;
  }

  Vnode* it = entry->vnode;
  while (it != g_root_directory) {
    Vnode* toDel = it;
    it = it->parent;
    if (it) {
      delete it;
    }
  }

  int res = g_file_table.removeFileEntry(fd);
  if (res == -1) {
    //errno
    return -1;
  }

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
    print_error(__LINE__, __FUNCTION__, "no such entry");
    return 0;
  }

  if (! (entry->flag & F_READ) && ! (entry->flag & F_RDWR)) {
    // errno
    print_error(__LINE__, __FUNCTION__, "flags error");
    return 0;
  }

  Vnode* vnode = entry->vnode;
  unsigned short fat_pos = vnode->fatPtr;
  int offs = entry->offset;

  if (find_fat(fat_pos, offs) == -1) {
    //errno
    print_error(__LINE__, __FUNCTION__, "cannot find fat ptr");
    return 0;
  }

  int i = 0, buf_pos = 0;
  do {
    res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos) + offs, SEEK_SET);
    if (res == -1) {
      // errno
      print_error(__LINE__, __FUNCTION__, "lseek start block offset failed");
      return 0;
    }

    if (size < BLOCKSIZE - offs) {
      res = read(g_disk_fd, (char*)data + buf_pos, size);
      if (res < size) {
	// errno
	print_error(__LINE__, __FUNCTION__, "read less than size");
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

			
    }
    else {
      int size_cur_block = BLOCKSIZE - offs;
      res = read(g_disk_fd, (char*)data + buf_pos, size_cur_block);
      if (res < size_cur_block) {
	// errno
	print_error(__LINE__, __FUNCTION__, "read less than size cur block");
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
      res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos), SEEK_SET);
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

size_t 	f_write(void *data, size_t size, int num, int fd) {
  int res;

  FtEntry* entry = g_file_table.getFileEntry(fd);
  if (! entry) {
    //errno
    print_error(__LINE__, __FUNCTION__, "cannot find fd");
    return 0;
  }

  if (! (entry->flag & F_WRITE) && ! (entry->flag & F_RDWR) && ! (entry->flag & F_APPEND)) {
    // errno
    print_error(__LINE__, __FUNCTION__, "no write flag specified");
    return 0;
  }

  if (entry->flag & F_APPEND) {
    entry->offset = entry->vnode->size;
  }

  Vnode* vnode = entry->vnode;
  unsigned short fat_pos = vnode->fatPtr;
  int offs = entry->offset;

  if (find_fat(fat_pos, offs) == -1) {
    //errno
    print_error(__LINE__, __FUNCTION__, "cannot find free fat");
    return 0;
  }

  int i = 0, buf_pos = 0;
  do {
    res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos + offs, SEEK_SET);
    if (res == -1) {
      // errno
      print_error(__LINE__, __FUNCTION__, "lseek failed");
      return 0;
    }

    if (size < BLOCKSIZE - offs) {
      res = write(g_disk_fd, (char*)data + buf_pos, size);
      if (res < size) {
	// errno
	print_error(__LINE__, __FUNCTION__, "write less than size");
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

    }
    else {
      int size_cur_block = BLOCKSIZE - offs;
      res = write(g_disk_fd, (char*)data + buf_pos, size_cur_block);
      if (res < size_cur_block) {
	// errno
	print_error(__LINE__, __FUNCTION__, "write less than size_cur_block");
	return i;
      }

      buf_pos += res;
      offs += res;
      entry->offset += res;

      if (g_fat_table[fat_pos] == EOBLOCK && size - size_cur_block > 0) {
	unsigned short free_fat = g_fat_table.getNextFreeBlock();
	if (! free_fat) {
	  // errno
	  print_error(__LINE__, __FUNCTION__, "cannot find free fat");
	  return i;
	}
	g_fat_table[fat_pos] = free_fat;
	g_fat_table[free_fat] = EOBLOCK;
      }

      fat_pos = g_fat_table[fat_pos];
      offs = 0;
      res = lseek(g_disk_fd, g_superblock.data_offset * fat_pos, SEEK_SET);
      if (res == -1) {
	// errno
	print_error(__LINE__, __FUNCTION__, "lseek failed");
	return 0;
      }

      res = write(g_disk_fd, (char*)data + buf_pos, size - size_cur_block);
      if (res < size - size_cur_block) {
	// errno
	print_error(__LINE__, __FUNCTION__, "write less than size - size_cur_block");
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



int f_seek(int offset, int whence, int fd) {
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



int f_rewind(int fd) {
  return f_seek(0, S_SET, fd);
}



int	f_stat(Stat *buf, int fd) {
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



int f_remove(const char* filename) {
  vector<string> filenames;
  g_filename_tokenizer.parseString(string(filename), filenames);

  Vnode* vnode = findVnode(filenames, 0);
  if (! vnode) {
    //errno
    return -1;
  }

  unsigned short ptr = vnode->fatPtr;
  while (ptr != EOBLOCK) {
    unsigned short oldPtr = ptr;
    ptr = g_fat_table[ptr];
    g_fat_table[oldPtr] = USMAX;
  }

  delete vnode;
  return 0;
}




int f_opendir(const char* dir) {
  int res;

  vector<string> dirs;
  g_filename_tokenizer.parseString(string(dir), dirs);

  Vnode* curDir = findVnode(dirs, 1); 
  if (! curDir) {
    //errno
    return -1;
  }

  int dd = g_file_table.getNextIndex();
  if (dd == -1) {
    //errno
    return -1;
  }

  g_file_table.addFileEntry(FtEntry(dd, curDir, 0, 0));

  return dd;
}



Stat* f_readdir(int dd) {
  int res;

  FtEntry* direntry = g_file_table.getFileEntry(dd);
  if (! direntry) {
    //errno
    return NULL;
  }

  int nextIdx = direntry->offset;
  unsigned short curPtr = direntry->vnode->fatPtr;

  int numBlock = nextIdx / g_max_num_child_in_block;
  int offsetIdx = nextIdx % g_max_num_child_in_block;
  for (int i = 0; i < numBlock; i++) {
    curPtr = g_fat_table[curPtr];
  }

  res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + curPtr) + offsetIdx * sizeof(Vnode), SEEK_SET);
  if (res == -1) {
    print_error(__LINE__, __FUNCTION__, "lseek vnode direntry failed");
    return NULL;
  }

  Vnode vnodeEntry;
  res = read(g_disk_fd, &vnodeEntry, sizeof(Vnode));
  if (res < sizeof(Vnode)) {
    print_error(__LINE__, __FUNCTION__, "read vnode direntry failed");
    return NULL;
  }

  Stat* retStat = new Stat(&vnodeEntry);

  return retStat;
}



int f_closedir(int dd){
  return f_close(dd);
}



int f_mkdir(char const* dir){
  int res;

  Vnode* temp = curVnode;
	
  vector<string> dirs;
  g_filename_tokenizer.parseString(string(dir), dirs);

  string dirName = dirs.back();
  dirs.pop_back();

  Vnode* newDirParent = findVnode(dirs, 1);
  if (! newDirParent) {
    //errno
    curVnode = temp;
    return -1;
  }

  unsigned short free_fat = g_fat_table.getNextFreeBlock();	
  if (! free_fat) {
    // errno
    return -1;
  }

  g_fat_table[free_fat] = EOBLOCK;

  unsigned short ptr = curVnode->fatPtr;
  while (g_fat_table[ptr] != EOBLOCK) {
    ptr = g_fat_table[ptr];
  }

  int vnodeAddr;
  if (curVnode->size % g_max_num_child_in_block == 0) {
    unsigned short nextFreeBlock = g_fat_table.getNextFreeBlock();
    if (! nextFreeBlock) {
      //errno
      curVnode = temp;
      return -1;
    }
    vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + nextFreeBlock);
  }
  else {
    vnodeAddr = BLOCKSIZE * (g_superblock.data_offset + ptr) + curVnode->size % g_max_num_child_in_block * sizeof(Vnode);
  }

  Vnode* newCurDir = new Vnode(dirName, 0, 0, 0, vnodeAddr, newDirParent, 0666, 1, time(NULL), free_fat);

  curVnode = temp;
  res = lseek(g_disk_fd, vnodeAddr, SEEK_SET);
  if (res == -1) {
    //errno;
    return -1;
  }

  res = write(g_disk_fd, newCurDir, sizeof(Vnode));
  if (res < sizeof(Vnode)) {
    //errno;
    return -1;
  }

  curVnode->size++;

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
  while (g_fat_table[ptr] != EOBLOCK){
    prev = ptr;
    ptr = g_fat_table[ptr];
  }
  
  int blockIndex = delDirParent->size % g_max_num_child_in_block;
  if (blockIndex == 0){
    Vnode *last = new Vnode;
    g_fat_table[prev] = EOBLOCK;
    g_fat_table[ptr] = USMAX;
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
  
  vector<int> usedBlocks;
  trackBlocks(usedBlocks, delDir);

  for (int i = 0; i < usedBlocks.size(); i++){
    g_fat_table[usedBlocks[i]] = USMAX;
  }

  return 0;
}

void trackBlocks(vector<int>& blocks, Vnode *node){
  int ptr = node->fatPtr;

  while (ptr != EOBLOCK){
    blocks.push_back(ptr);
    if (node->type == 1){
      for (int i = 0; i < g_max_num_child_in_block; i++){
	Vnode *child = new Vnode;
	lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + i * sizeof(Vnode), SEEK_SET);
	read(g_disk_fd, child, sizeof(Vnode));
	trackBlocks(blocks, child);
	delete(child);
      }
    }
    ptr = g_fat_table[ptr];
  }

  return;
}
