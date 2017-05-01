
#include <iostream>
#include <vector>

#include "vfs.hpp"

using namespace std;

FtEntry::FtEntry(int idx, Vnode* vn, int offs, int f) 
	: index(idx), vnode(vn), offset(offs), flag(f) {}

FtEntry::~FtEntry(void) {}

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
	int nextIndex = getNextIndex();
	if (nextIndex == -1) {
		return -1;
	}

	_fileTable[nextIndex] = ftEntry;
	return 0;
}




FatTable::FatTable(void) : _fatTable(FATSIZE, 0) {}

FatTable::~FatTable(void) {}

unsigned short FatTable::getNextFreeBlock(void) {
	for (int i = 1; i < FATSIZE; i++) {
		if (! _fatTable[i]) {
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
