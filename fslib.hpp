#ifndef _FSLIB_HPP
#define _FSLIB_HPP



extern ft_entry					g_file_table[MAXFTSIZE];



fd_t 				f_open(vnode_t *vn, const char *filename, int flags);
size_t 				f_read(vnode_t *vn, void *data, size_t size, int num, fd_t fd);
size_t 				f_write(vnode_t *vn, void *data, size_t size, int num, fd_t fd);
int 				f_close(vnode_t *vn, fd_t fd);
int 				f_seek(vnode_t *vn, int offset, int whence, fd_t fd);
int 				f_rewind(vnode_t *vn, fd_t fd);
int					f_stat(vnode_t *vn, struct stat_t *buf, fd_t fd);
int					f_remove(vnode_t *vn, const char *filename);
dir_t				f_opendir(vnode_t *vn, const char *filename);
struct dirent_t*	f_readdir(vnode_t *vn, dir_t* dirp);
int					f_closedir(vnode_t *vn, dir_t* dirp);
int					f_mkdir(vnode_t *vn, const char *filename, int mode);
int					f_rmdir(vnode_t *vn, const char *filename);
int					f_mount(vnode_t *vn, const char *type, const char *dir, int flags, void *data);
int					f_umount(vnode_t *vn, const char *dir, int flags);



#endif //_FSLIB_HPP