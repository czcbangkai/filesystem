#ifndef _FSLIB_HPP
#define _FSLIB_HPP

#include "vfs.hpp"
#include "tokenizer.hpp"

// extern FileTable 	g_file_table;
// extern FatTable		g_fat_table;
extern Tokenizer 	g_filename_tokenizer;


static int find_fat(unsigned short& start, int& offset);
static void trackBlocks(vector<int>& blocks, Vnode *node);
Vnode *findVnode(vector<string>& filenames, int type);
int 				f_open(const char *filename, int flags);
size_t 				f_read(void *data, size_t size, int num, int fd);
size_t 				f_write(void *data, size_t size, int num, int fd);
int 				f_close(int fd);
int 				f_seek(int offset, int whence, int fd);
int 				f_rewind(int fd);
int					f_stat(Stat *buf, int fd);
int					f_remove(const char *filename);
int					f_opendir(const char *filename);
Stat*				f_readdir(int ddv);
int					f_closedir(int dd);
int					f_mkdir(const char *filename);
int					f_rmdir(const char *filename);
// int					f_mount(const char *type, const char *dir, int flags, void *data);
// int					f_umount(const char *dir, int flags);



#endif //_FSLIB_HPP
