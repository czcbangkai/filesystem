#ifndef _VFS_HPP
#define _VFS_HPP


#include <iostream>
#include <string>
#include <vector>

using namespace std;


#define MAXFTSIZE				10000
#define FATSIZE					65526
#define BLOCKSIZE				2048
#define USMAX					65535
#define EOBLOCK					65534

#define F_READ					1
#define F_WRITE					2
#define F_RDWR					4
#define F_APPEND				8

#define S_SET					0
#define S_CUR					1
#define S_END					2



class SuperBlock {
public:
	friend ostream& operator<<(ostream& os, SuperBlock const& superBlock) {
		os << "block size: " << superBlock.blocksize << endl;
		os << "fat offset: " << superBlock.fat_offset << endl;
		os << "data offset: " << superBlock.data_offset << endl;
		return os;
	}

	int blocksize;
	int fat_offset;
	int data_offset;
};

class Vnode {
public:
	Vnode(void);
	Vnode(string name_, int uid_, int gid_, int size_, int addr_, Vnode* parent_, int permission_, int type_, int timestamp_, int fatPtr_);
	~Vnode(void);

	int writeToDisk(int fd);

	friend ostream& operator<<(ostream& os, Vnode const& vnode) {
		os << "name: " << string(vnode.name) << endl;
		os << "uid: " << dec << vnode.uid << endl;
		os << "gid: " << dec << vnode.gid << endl;
		os << "size: " << dec << vnode.size << endl;
		os << "permission: " << oct << vnode.permission << endl;
		os << "type: " << vnode.type << endl;
		os << "timestamp: " << vnode.timestamp << endl;
		os << "fatPtr: " << vnode.fatPtr << endl; 
		return os;
	}

	char name[255];
	int uid;
	int gid;
	int size;
	Vnode* parent;
	int permission;	
	int type;
	int timestamp;
	int fatPtr;
	int address;
};



class FtEntry {
public:
	FtEntry(int i = -1, Vnode* vn = NULL, int offs = 0, int f = 0);
	~FtEntry(void);
	void removeSelf(void);

	int index;
	Vnode* vnode;
	int offset;
	int flag;
};

class FileTable {
public:
	FileTable(void);
	~FileTable(void);

	FtEntry* getFileEntry(int fd);
	int getNextIndex(void);
	int addFileEntry(FtEntry const& ftEntry);
	int removeFileEntry(int fd);

	FtEntry& operator[](size_t i) {
		return _fileTable[i];
	}
	FtEntry const& operator[](size_t i) const {
		return (*this)[i];
	}

private:
	vector<FtEntry> _fileTable;
};



class FatTable {
public:
	FatTable(void);
	~FatTable(void);

	friend ostream& operator<<(ostream& os, FatTable const& fatTable) {
		for (int i = 0; i < FATSIZE; i++) {
			os << static_cast<unsigned>(fatTable._fatTable[i]) << endl;
		}
		return os;
	}

	unsigned short getNextFreeBlock(void);
	int writeToDisk(int fd, int fat_offset, int index);
	unsigned short& operator[](size_t i);
	unsigned short const& operator[](size_t i) const;

private:
	unsigned short _fatTable[FATSIZE];
};





class Stat {
public:
	Stat(void);
	Stat(Vnode const* vnode);
	~Stat(void);

	char name[255];
	int uid;
	int gid;
	size_t size;
	int permission;
	int type;
	int timestamp;
	int fatPtr;
};



#endif //_VFS_HPP