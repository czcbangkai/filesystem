#ifndef _VFS_HPP
#define _VFS_HPP



#define MAXFTSIZE				10000
#define FATSIZE					65526
#define BLOCKSIZE				2048


class superblock {
public:
	int blocksize;
	int fat_offset;
	int data_offset;
};

class vnode {
public:
	char name[255];
	int size;
	vnode* parent;
	int permission;	
	int type;
	unsigned int timestamp;
	int fatPtr;
};



class ft_entry {
public:
	int index;
	vnode* vn;
	int offset;
	int flag;
};



class stat {
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