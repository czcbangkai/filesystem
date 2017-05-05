
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>

#include "vfs.hpp"

using namespace std;




Vnode::Vnode(void) {}

Vnode::~Vnode(void) {}

Vnode::Vnode(string name_, int uid_, int gid_, int size_, int addr_, Vnode* parent_, int permission_, int type_, int timestamp_, int fatPtr_) {
	strncpy(name, name_.c_str(), 255);
	uid = uid_;
	gid = gid_;
	size = size_;
	address = addr_;
	parent = parent_;
	permission = permission_;
	type = type_;
	timestamp = timestamp_;
	fatPtr = fatPtr_;
}

int Vnode::writeToDisk(int fd) {
	int res;
	res = lseek(fd, address, SEEK_SET);
	if (res == -1) {
		//errno;
		return -1;
	}
	res = write(fd, this, sizeof(Vnode));
	if (res < sizeof(Vnode)) {
		//errno
		return -1;
	}
	return 0;
}




FtEntry::FtEntry(int i, Vnode* vn, int offs, int f) 
	: index(i), vnode(vn), offset(offs), flag(f) {}

FtEntry::~FtEntry(void) {}

void FtEntry::removeSelf(void) {
	index = -1;
	vnode = NULL;
}



FileTable::FileTable(void) : _fileTable(MAXFTSIZE, FtEntry()) {}

FileTable::~FileTable(void) {}

FtEntry* FileTable::getFileEntry(int fd) {
	if (fd >= MAXFTSIZE || fd < 0) {
		//errno
		return NULL;
	}
	if (_fileTable[fd].index == -1) {
		//errno
		return NULL;
	}
	return &(_fileTable[fd]);
}

int FileTable::getNextIndex(void) {
	for (int i = 3; i < MAXFTSIZE; i++) {
		if (_fileTable[i].index == -1) {
			return i;
		}
	}
	return -1;
}

int FileTable::addFileEntry(FtEntry const& ftEntry) {
	_fileTable[ftEntry.index] = ftEntry;
	return 0;
}

int FileTable::removeFileEntry(int fd) {
	FtEntry* ftEntry = getFileEntry(fd);
	if (! ftEntry) {
		// errno
		return -1;
	}
	ftEntry->removeSelf();
	return 0;
}




FatTable::FatTable(void) {
	for (int i = 0; i < FATSIZE; i++) {
		_fatTable[i] = (unsigned short)USMAX;
	}	
}

FatTable::~FatTable(void) {}

unsigned short FatTable::getNextFreeBlock(void) {
	for (unsigned short i = 1; i < FATSIZE; i++) {
		if (_fatTable[i] == USMAX) {
			return i;
		}
	}

	return 0;
}

int FatTable::writeToDisk(int fd, int fat_offset, int index) {
	int res;
	res = lseek(fd, BLOCKSIZE * fat_offset + index * sizeof(unsigned short), SEEK_SET);
	if (res == -1) {
		//errno
		return -1;
	}
	res = write(fd, &_fatTable[index], sizeof(unsigned short));
	if (res < sizeof(unsigned short)) {
		//errno
		return -1;
	}

	return 0;
}


unsigned short& FatTable::operator[](size_t i) {
	if (i >= FATSIZE) {
		// errno
		return _fatTable[0];
	}

	return _fatTable[i];
}

unsigned short const& FatTable::operator[](size_t i) const {
	return (*this)[i];
}



Stat::Stat(void) {}

Stat::Stat(Vnode const* vnode) {
	strncpy(name, vnode->name, 255);
	uid = vnode->uid;
	gid = vnode->gid;
	size = vnode->size;
	permission = vnode->permission;
	type = vnode->type;
	timestamp = vnode->timestamp;
	fatPtr = vnode->fatPtr;
}

Stat::~Stat(void) {}



