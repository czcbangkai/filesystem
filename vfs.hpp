#ifndef _VFS_HPP
#define _VFS_HPP


#include <iostream>
#include <string>
#include <vector>


#define MAXFTSIZE				10000
#define FATSIZE					65526
#define BLOCKSIZE				2048

#define O_READ					0x0000
#define O_WRITE					0x0001
#define O_RDWR					0x0002
#define O_APPEND				0x0003

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
	Vnode* parent;
	vector<Vnode*>* children;
	int permission;	
	int type;
	unsigned int timestamp;
	int fatPtr;
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







class Stat {
public:
	char name[255];
	int uid;
	int gid;
	size_t size;
	int permission;
	int type;
	unsigned int timestamp;
};



#endif //_VFS_HPP