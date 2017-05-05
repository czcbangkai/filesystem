#include <cstdlib>

#include <unistd.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include "builtin.hpp"
#include "mysh.hpp"
#include "vfs.hpp"
#include "fslib.hpp"

using namespace std;



BuiltinList g_builtinList;



BuiltinList::BuiltinList(void) {
	createBuiltinFunc("exit", &builtin_exit);
	createBuiltinFunc("help", &builtin_help);
	createBuiltinFunc("ls", &builtin_ls);
	createBuiltinFunc("chmod", &builtin_chmod);
	createBuiltinFunc("mkdir", &builtin_mkdir);
	createBuiltinFunc("rmdir", &builtin_rmdir);
	createBuiltinFunc("cd", &builtin_cd);
	createBuiltinFunc("pwd", &builtin_pwd);
	createBuiltinFunc("cat", &builtin_cat);
	createBuiltinFunc("rm", &builtin_rm); 
}

BuiltinList::~BuiltinList(void) {}

void BuiltinList::createBuiltinFunc(string const& funcName, BuiltinFunc funcPtr) {
	_builtinList[funcName] = funcPtr;
}

BuiltinFunc BuiltinList::findBuiltinFunc(string const& funcName) {
	for (auto& p : _builtinList) {
		if (p.first == funcName) {
			return p.second;
		}
	}
	return NULL;
}



string build_path(vector<string>& path){
	string res;
	for (int i = 0; i < path.size(); i++) {
		res += path[0] + "/";
	}
	return res;
}



int builtin_exit(vector<string> const& argv) {
	exit(0);
	return 0;
}

int builtin_help(vector<string> const& argv) {
	return 0;
}

int builtin_chmod(vector<string> const& argv) {
	if (argv.size() != 3) {
		return -1;
	}

	vector<string> filenames;
	g_filename_tokenizer.parseString(argv[2], filenames);
	Vnode *curFile = findVnode(filenames, 1);
	if (! curFile){
	  //errno
	  return -1;
	}

	curFile->permission = strtol(argv[1].c_str(), NULL, 8);
	int res = curFile->writeToDisk(g_disk_fd);
	if (res == -1) {
		return -1;
	}

	return 0;
}


int builtin_ls(vector<string> const& argv){
	string filepath = build_path(g_cur_dir);
  	int dd = f_opendir(filepath.c_str());
  
	if (dd < 0) {
		//errno
		return -1;
	}
 
 	Stat* nodeStat = f_readdir(dd);
  	while (nodeStat != NULL){
    	if (argv.size() == 1){
    		if (argv[1] == "-l"){
				//flag -l
      		} 
      		else {
				if (nodeStat->type == 1){
 					cout << nodeStat->name << "/\t";
				} 
				else {
 					cout << nodeStat->name << "\t";
				}
      		}
    	} 
    	else {
      		cout << nodeStat->name << "\t";
    	}
    	nodeStat = f_readdir(dd);
  	}
  	cout << endl;
	  
	int res = f_closedir(dd);
	if (res != 0){
		//errno
		cout << "failed to close dir" << endl;
		return -1;
	}

	//check if dd is still in file table
	//FtEntry *blank = g_file_table.getFileEntry(dd);
	//if (blank == NULL) cout << "succeeded closing" << endl;
	return 0;
}

int builtin_mkdir(vector<string> const& argv){
	return f_mkdir(argv[1].c_str());
}


int builtin_rmdir(vector<string> const& argv){
  return f_rmdir(argv[1].c_str());
}



int builtin_cd(vector<string> const& argv){
	if (argv[1] == "."){
		return 0;
	} 
	else if (argv[1] == ".."){
		if (g_cur_vnode == g_root_vnode) {
			return 0;
		}
		Vnode* toDel = g_cur_vnode;
		g_cur_vnode = g_cur_vnode->parent;
		if (toDel) {
			delete toDel;
		}
		g_cur_dir.pop_back();
	} 
	else {
		vector<string> dirs;
		g_filename_tokenizer.parseString(argv[1], dirs);
		Vnode *nextDir = findVnode(dirs, 1);
		if (!nextDir){
		  //errno
		  return -1;
		}
		g_cur_vnode = nextDir;
		g_cur_dir = dirs;
		return 0;
	}

	return 0;
}



static int cat(int in_fd, int out_fd) {
	// Temporary buffer
	char buffer[2048] = {0};
	// Record bytes already read
	ssize_t count_read = 0;

	// Keep reading from the input file until reaches EOF
	while ((count_read = f_read(buffer, 1, sizeof(buffer), in_fd)) > 0) {
		// Record bytes already written
		ssize_t count_total_written = 0;
		// Keep writing until all read data is copied
		while (count_total_written < count_read) {
			// Within this loop, every iteration should write successfully
			ssize_t count_written = f_write(buffer + count_total_written, 1, count_read - count_total_written, out_fd);
			if (count_written == -1) {
				perror("write");
				return -1;
			}
			// Update the total written bytes
			count_total_written += count_written;
		}
	}

	if (in_fd) {
		f_close(in_fd);
	}

	// Return -1 when not all data is written, otherwise 0
	return count_read ? -1 : 0;
}


int builtin_cat(vector<string> const& argv){
	if (argv.size() != 2 && argv.size() != 4) {
		return -1;
	}

	if (argv.size() == 4) {
		if (argv[1].length() > 1 && argv[1][0] == '\"' && argv[1].back() == '\"') {
			int out_fd = f_open(argv[3].c_str(), F_WRITE);
			if (out_fd == -1) {
				return -1;
			}

			string content = argv[1].substr(1, argv[1].length() - 2);
			char c_content[content.length() + 1];
			strcpy(c_content, content.c_str());

			if (content.length() > 0) {
				int res = f_write(c_content, 1, content.length(), out_fd);
				if (res < content.length()) {
					return -1;
				}
			}
		}
		else {
			int in_fd = f_open(argv[1].c_str(), F_READ);
			if (in_fd == -1) {
				return -1;
			}

			int out_fd = f_open(argv[3].c_str(), F_WRITE);
			if (out_fd == -1) {
				return -1;
			}

			cat(in_fd, out_fd);

			// f_close(out_fd);
		}
	}
	else if (argv.size() == 2) {
		int in_fd = f_open(argv[1].c_str(), F_READ);
		if (in_fd == -1) {
			return -1;
		}

		char buf[BLOCKSIZE + 1] = {0};
		int res;
		do {
			res = f_read(buf, 1, BLOCKSIZE, in_fd);
			if (res == -1) {
				cout << "bad read: " << res << endl;
			}
			if (res <= 0) {
				break;
			}

			cout << string(buf);
		} while (res > 0);

		cout << endl;
	}

	return 0;
}




int builtin_pwd(vector<string> const& argv){
	string curPath = getCurrentPath();
	cout << curPath << endl;
	return 0;
}


int builtin_rm(vector<string> const& argv){ 
	if (argv.size() == 2){
		return f_remove(argv[1].c_str());
  	} 
  	else if (argv[1] == "-r"){
		return f_rmdir(argv[2].c_str());
  	}
  	return 0;
}