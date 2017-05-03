
#include <cstring>
#include <iostream>
#include <vector>

#include "vfs.hpp"

using namespace std;





Vnode::Vnode(string name_, int uid_, int gid_, int size_, Vnode* parent_, int permission_, int type_, int timestamp_, int fatPtr_) {
	strncpy(name, name_.c_str(), 255);
	uid = uid_;
	gid = gid_;
	size = size_;
	parent = parent_;
	permission = permission_;
	type = type_;
	timestamp = timestamp_;
	fatPtr = fatPtr_;
}



FtEntry::FtEntry(int idx, Vnode* vn, int offs, int f) 
	: index(idx), vnode(vn), offset(offs), flag(f) {}

FtEntry::~FtEntry(void) {}

void FtEntry::removeSelf(void) {
	index = -1;
	delete vnode;
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
	return -1;
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
