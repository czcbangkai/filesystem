
#include <iostream>
#include <vector>

#include "vfs.hpp"

FtEntry::FtEntry(int idx = -1, Vnode* vn = NULL, int offs = 0, int f = 0) 
	: index(idx), vnode(vn), offset(offs), flag(f) {}

FtEntry::~FtEntry(void) {}

FileTable::FileTable(void) : _fileTable(FtEntry(), MAXFTSIZE) {}

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
	int nextIndex = getNextIndex();
	if (nextIndex == -1) {
		return -1;
	}

	_fileTable[nextIndex] = ftEntry;
	return 0;
}