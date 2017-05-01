#ifndef _VFS_HPP
#define _VFS_HPP



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




class superblock {
public:
	int blocksize;
	int fat_offset;
	int data_offset;
};

class Vnode {
public:
	char name[255];
	int size;
	vnode* parent;
	int permission;	
	int type;
	unsigned int timestamp;
	int fatPtr;
};



class FtEntry {
public:
	int index;
	vnode* vn;
	int offset;
	int flag;
};

class FileTable {
public:
	FileTable(void);
	~FileTable(void);

	FtEntry& getFileEntry(int fd);
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
	int fatPtr;
};



#endif //_VFS_HPP