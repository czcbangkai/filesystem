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
#define DNODESIZE				288

#define F_READ					0x0000
#define F_WRITE					0x0001
#define F_RDWR					0x0002
#define F_APPEND				0x0003

#define S_SET					0
#define S_CUR					1
#define S_END					2



class SuperBlock {
public:
	int blocksize;
	int fat_offset;
	int data_offset;
};

class Vnode {
public:
	Vnode(void);
	~Vnode(void);

	char name[255];
	int uid;
	int gid;
	int size;
	int permission;	
	int type;
	unsigned int timestamp;
	int fatPtr;
	Vnode* parent;
};



class FtEntry {
public:
	FtEntry(int i = -1, Vnode* vn = NULL, int offs = 0, int f = 0);
	~FtEntry(void);

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

private:
	vector<FtEntry> _fileTable;
};



class FatTable {
public:
	FatTable(void);
	~FatTable(void);

	unsigned short getNextFreeBlock(void);
	unsigned short& operator[](size_t i);
	unsigned short const& operator[](size_t i) const;

private:
	vector<unsigned short> _fatTable;
};





class Stat {
public:
	char name[255];
	int uid;
	int gid;
	size_t size;
	int permission;
	int type;
	unsigned int timestamp;
	int fatPtr;
};



#endif //_VFS_HPP
