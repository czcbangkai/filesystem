#ifndef _FSLIB_HPP
#define _FSLIB_HPP

#include "vfs.hpp"
#include "tokenizer.hpp"

// extern FileTable 	g_file_table;
// extern FatTable		g_fat_table;
extern Tokenizer 	g_filename_tokenizer;


static int find_fat(unsigned short& start, int& offset);

int 				f_open(Vnode *vn, const char *filename, int flags);
size_t 				f_read(Vnode *vn, void *data, size_t size, int num, int fd);
size_t 				f_write(Vnode *vn, void *data, size_t size, int num, int fd);
int 				f_close(Vnode *vn, int fd);
int 				f_seek(Vnode *vn, int offset, int whence, int fd);
int 				f_rewind(Vnode *vn, int fd);
int					f_stat(Vnode *vn, Stat *buf, int fd);
// int					f_remove(Vnode *vn, const char *filename);
// dir_t				f_opendir(Vnode *vn, const char *filename);
// struct dirent_t*	f_readdir(Vnode *vn, dir_t* dirp);
// int					f_closedir(Vnode *vn, dir_t* dirp);
// int					f_mkdir(Vnode *vn, const char *filename, int mode);
// int					f_rmdir(Vnode *vn, const char *filename);
// int					f_mount(Vnode *vn, const char *type, const char *dir, int flags, void *data);
// int					f_umount(Vnode *vn, const char *dir, int flags);



#endif //_FSLIB_HPP