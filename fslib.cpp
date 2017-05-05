
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
#include "utilities.hpp"

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
		curVnode = g_root_vnode;
		if (filenames.size() == 1 && type == 1){
		  return g_root_vnode;
		}
	} else {
    	cout << "does not support relative path" << endl;
    	return NULL;
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

					if (filenames[i] == string(vnodeName)) {
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

	return curFile;
}


int f_open(const char *filename, int flags, int permission) {
	int res; 

	vector<string> filenames;
	g_filename_tokenizer.parseString(string(filename), filenames);

	Vnode* curFile = findVnode(filenames, 0);
	if (! curFile && ((flags & F_READ) || (flags & F_RDWR))) {
		// errno
		cout << "bad flags" << endl;
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

		curFile = new Vnode(filenames.back(), 0, 0, 0, vnodeAddr, curVnode, permission, 0, time(NULL), free_fat);

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
		res = curVnode->writeToDisk(g_disk_fd);
		if (res == -1) {
			//errno;
			return -1;
		}
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
	while (it != g_root_vnode) {
		Vnode* toDel = it;
		if (it) {
			it = it->parent;
		}
		if (toDel) {
			delete it;
			it = NULL;
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

	// cout << "total size: " << vnode->size << endl;

	if (offs >= vnode->size) {
		entry->offset = vnode->size;
		return 0;
	}

	if (find_fat(fat_pos, offs) == -1) {
		//errno
		print_error(__LINE__, __FUNCTION__, "cannot find fat ptr");
		return 0;
	}

	int i = 0, buf_pos = 0;
	for (i = 0; i < num; i++) {
		// cout << "i: " << i << endl;
		// cout << "cur offset: " << offs << endl;
		// cout << "cur entry->offset: " << entry->offset << endl;

		res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos) + offs, SEEK_SET);
		if (res == -1) {
			// errno
			print_error(__LINE__, __FUNCTION__, "lseek start block offset failed");
			return 0;
		}

		if (size <= BLOCKSIZE - offs) {
			res = read(g_disk_fd, (char*)data + buf_pos, size);
			// cout << "read: " << *((char*)data + buf_pos) << endl;
			// cout << "res: " << res << endl;

			buf_pos += res;
			offs += res;
			entry->offset += res;

			// cout << "res1: " << res << endl;

			if (res < size) {
				// errno
				print_error(__LINE__, __FUNCTION__, "read less than size");
				return i;
			}
		}
		else {
			int size_cur_block = BLOCKSIZE - offs;
			res = read(g_disk_fd, (char*)data + buf_pos, size_cur_block);

			buf_pos += res;
			offs += res;
			entry->offset += res;

			// cout << i << endl;
			// cout << offs << endl;
			// cout << "res2: " << res << endl;

			if (res < size_cur_block) {
				// errno
				print_error(__LINE__, __FUNCTION__, "read less than size cur block");
				return i;
			}
			

			if (g_fat_table[fat_pos] == EOBLOCK) {
				// cout << "reach EOF" << endl;
				break;
			}

			fat_pos = g_fat_table[fat_pos];
			offs = 0;
			res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos), SEEK_SET);
			if (res == -1) {
				// errno
				return 0;
			}

			// cout << "size - size_cur_block: " << (size - size_cur_block) << endl;
			res = read(g_disk_fd, (char*)data + buf_pos, size - size_cur_block);

			buf_pos += res;
			offs += res;
			entry->offset += res;

			// cout << "res3: " << res << endl;

			if (res < size - size_cur_block) {
				// errno
				return i;
			}


		}

		// cout << "end cur offset: " << offs << endl;
		// cout << "end cur entry->offset: " << entry->offset << endl;
	}

	// cout << "offset after read all:" << dec << entry->offset << endl;

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
		// cout << "here offset: " << offs << endl;
		res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos) + offs, SEEK_SET);
		if (res == -1) {
			// errno
			print_error(__LINE__, __FUNCTION__, "lseek failed");
			return 0;
		}

		if (size <= BLOCKSIZE - offs) {
			res = write(g_disk_fd, (char*)data + buf_pos, size);

			buf_pos += res;
			offs += res;
			entry->offset += res;

			if (res < size) {
				// errno
				print_error(__LINE__, __FUNCTION__, "write less than size");
				return i;
			}

		}
		else {
			int size_cur_block = BLOCKSIZE - offs;
			res = write(g_disk_fd, (char*)data + buf_pos, size_cur_block);

			buf_pos += res;
			offs += res;
			entry->offset += res;

			if (res < size_cur_block) {
				// errno
				print_error(__LINE__, __FUNCTION__, "write less than size_cur_block");
				return i;
			}

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
			res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + fat_pos), SEEK_SET);
			if (res == -1) {
				// errno
				print_error(__LINE__, __FUNCTION__, "lseek failed");
				return 0;
			}

			res = write(g_disk_fd, (char*)data + buf_pos, size - size_cur_block);

			buf_pos += res;
			offs += res;
			entry->offset += res;

			if (res < size - size_cur_block) {
				// errno
				print_error(__LINE__, __FUNCTION__, "write less than size - size_cur_block");
				return i;
			}
		}

		i++;

	} while (i < num);

	if (entry->offset > vnode->size) {
		vnode->size = entry->offset;
	}
	vnode->timestamp = time(NULL);

	vnode->writeToDisk(g_disk_fd);

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

	Vnode* delFile = findVnode(filenames, 0);
	if (! delFile) {
		//errno
		perror("Trying to remove non existing file.");
		return -1;
	}

	unsigned short ptr = delFile->fatPtr;
	while (ptr != EOBLOCK) {
		unsigned short oldPtr = ptr;
		ptr = g_fat_table[ptr];
		g_fat_table[oldPtr] = USMAX;
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + oldPtr * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[oldPtr], sizeof(unsigned short));
	}

	Vnode *delDirParent = delFile->parent;
	if (delDirParent == NULL) {
		//errno
		return -1;
	}

	//find delDir address

	int prev;
	int filePtr = delDirParent->fatPtr;
	while (g_fat_table[filePtr] != EOBLOCK){
		prev = filePtr;
		filePtr = g_fat_table[filePtr];
	}

	int blockIndex = delDirParent->size % g_max_num_child_in_block;
	if (blockIndex == 0){
		Vnode last;
		g_fat_table[prev] = EOBLOCK;
		g_fat_table[ptr] = USMAX;
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + prev * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[prev], sizeof(unsigned short));
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + ptr * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[ptr], sizeof(unsigned short));
		lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr), SEEK_SET);
		read(g_disk_fd, &last, sizeof(Vnode));
		lseek(g_disk_fd, delFile->address, SEEK_SET);
		write(g_disk_fd, &last, sizeof(Vnode));
	} 
	else {
		Vnode last;
		lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + blockIndex * sizeof(Vnode), SEEK_SET);
		read(g_disk_fd, &last, sizeof(Vnode));
		lseek(g_disk_fd, delFile->address, SEEK_SET);
		write(g_disk_fd, &last, sizeof(Vnode));

		// cout << "delFile address: " << delFile->address << endl;
		// cout << "last node address: " << last->address << endl;
	}

	if (delFile) {
		delete delFile;
		delFile = NULL;
	}

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


	if (direntry->offset < direntry->vnode->size){
		Stat* retStat = new Stat(&vnodeEntry);
    	direntry->offset++;
		return retStat;
  	} 
  	else {
    	return NULL;
  	}
}



int f_closedir(int dd){
	return f_close(dd);
}



int f_mkdir(char const* dir){
	int res;

	vector<string> dirs;
	g_filename_tokenizer.parseString(string(dir), dirs);

	string dirName = dirs.back();
	dirs.pop_back();

	Vnode* newDirParent = findVnode(dirs, 1);
	if (! newDirParent) {
		//errno
		return -1;
	}

	unsigned short free_fat = g_fat_table.getNextFreeBlock();	
	if (! free_fat) {
		// errno
		return -1;
	}

	g_fat_table[free_fat] = EOBLOCK;

	unsigned short prev = -1;
	unsigned short ptr = newDirParent->fatPtr;
	int numchild = newDirParent->size;
	while (ptr != EOBLOCK) {
		for (int i = 0; i < g_max_num_child_in_block; i++){
			if (numchild > 0){
				Vnode cur;
				lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + i * sizeof(Vnode), SEEK_SET);
				read(g_disk_fd, &cur, sizeof(Vnode));
				if (string(cur.name) == dirName){
				//errno
				// cout << "Name: " << cur->name << endl;
				// cout << "Directory name already existed." << endl;
				return -1;
				}
				numchild--;
			} 
			else {
				break;
			}
		}
		prev = ptr;
		ptr = g_fat_table[ptr];
	}

  	ptr = prev;


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

	Vnode* newCurDir = new Vnode(dirName, 0, 0, 0, vnodeAddr, newDirParent, 0666, 1, time(NULL), free_fat);
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

	newDirParent->size++;
	res = newDirParent->writeToDisk(g_disk_fd);
	if (res == -1) {
		//errno
		return -1;
	}

	res = lseek(g_disk_fd, newDirParent->address, SEEK_SET);
	if (res == -1) {
		perror("lseek new dir parent addr");
		return -1;
	}
  	res = write(g_disk_fd, newDirParent, sizeof(Vnode));
  	if (res < sizeof(Vnode)) {
  		perror("write new dir parent vnode failed");
  		return -1;
  	}

	Vnode *it = newCurDir;
	while (it != g_root_vnode){
		Vnode *del = it;
		if (it) {
			it = it->parent;
		}
		if (del) {
			delete del;
			del = NULL;
			break;
		}
  	}


	return 0;
}



int f_rmdir(const char* dir){
	int res;

	vector<string> dirs;
	g_filename_tokenizer.parseString(string(dir), dirs);

	Vnode *delDir = findVnode(dirs, 1);
	if (delDir == NULL) {
		//errno
		return -1;
	}

	Vnode *delDirParent = delDir->parent;
	if (delDirParent == NULL) {
		//errno
		return -1;
	}

	int prev;
	int ptr = delDirParent->fatPtr;
		while (g_fat_table[ptr] != EOBLOCK){
		prev = ptr;
		ptr = g_fat_table[ptr];
	}

	int blockIndex = delDirParent->size % g_max_num_child_in_block;
	if (blockIndex == 0){
		Vnode last;
		g_fat_table[prev] = EOBLOCK;
		g_fat_table[ptr] = USMAX;
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + prev * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[prev], sizeof(unsigned short));
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + ptr * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[ptr], sizeof(unsigned short));
		lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr), SEEK_SET);
		read(g_disk_fd, &last, sizeof(Vnode));
		lseek(g_disk_fd, delDir->address, SEEK_SET);
		write(g_disk_fd, &last, sizeof(Vnode));
	} 
	else {
		Vnode last;
		lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + blockIndex * sizeof(Vnode), SEEK_SET);
		read(g_disk_fd, &last, sizeof(Vnode));
		lseek(g_disk_fd, delDir->address, SEEK_SET);
		write(g_disk_fd, &last, sizeof(Vnode));
  	}

	vector<int> usedBlocks;
	res = trackBlocks(usedBlocks, delDir);
	if (res == -1) {
		//errno
		return -1;
	}
	// cout << "size of usedBlocks: " << usedBlocks.size() << endl;

	for (int i = 0; i < usedBlocks.size(); i++){
		//cout << "usedblocks: " << usedBlocks[i] << endl;
		g_fat_table[usedBlocks[i]] = USMAX;
		lseek(g_disk_fd, BLOCKSIZE * g_superblock.fat_offset + usedBlocks[i] * sizeof(unsigned short), SEEK_SET);
		write(g_disk_fd, &g_fat_table[usedBlocks[i]], sizeof(unsigned short));
	}

  return 0;
}



static int trackBlocks(vector<int>& blocks, Vnode *node){
	int res;
	int ptr = node->fatPtr;

	int numchild = node->size;

	while (ptr != EOBLOCK){
		blocks.push_back(ptr);
		if (node->type == 1){
		 	for (int i = 0; i < g_max_num_child_in_block; i++){
				if (numchild < 1) {
					break;
				}
				Vnode child;
				res = lseek(g_disk_fd, BLOCKSIZE * (g_superblock.data_offset + ptr) + i * sizeof(Vnode), SEEK_SET);
				if (res == -1) {
					perror("lseek failed");
					return -1;
				}
				
				res = read(g_disk_fd, &child, sizeof(Vnode));
				if (res < sizeof(Vnode)) {
					perror("read child vnode failed");
					return -1;
				}

				trackBlocks(blocks, &child);
				numchild--;
			}
		}
		ptr = g_fat_table[ptr];
	}

	return 0;
}
