#ifndef _FSLIB_HPP
#define _FSLIB_HPP

#include "vfs.hpp"

extern ft_entry					g_file_table[MAXFTSIZE];



fd_t 				f_open(Vnode *vn, const char *filename, int flags);
size_t 				f_read(Vnode *vn, void *data, size_t size, int num, fd_t fd);
size_t 				f_write(Vnode *vn, void *data, size_t size, int num, fd_t fd);
int 				f_close(Vnode *vn, fd_t fd);
int 				f_seek(Vnode *vn, int offset, int whence, fd_t fd);
int 				f_rewind(Vnode *vn, fd_t fd);
int					f_stat(Vnode *vn, Stat *buf, fd_t fd);
int					f_remove(Vnode *vn, const char *filename);
dir_t				f_opendir(Vnode *vn, const char *filename);
struct dirent_t*	f_readdir(Vnode *vn, dir_t* dirp);
int					f_closedir(Vnode *vn, dir_t* dirp);
int					f_mkdir(Vnode *vn, const char *filename, int mode);
int					f_rmdir(Vnode *vn, const char *filename);
int					f_mount(Vnode *vn, const char *type, const char *dir, int flags, void *data);
int					f_umount(Vnode *vn, const char *dir, int flags);



#endif //_FSLIB_HPP