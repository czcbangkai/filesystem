#include <cstdlib>

#include <unistd.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include "builtin.hpp"
#include "fslib.hpp"
#include "mysh.hpp"

using namespace std;


BuiltinList g_builtinList;


BuiltinList::BuiltinList(void) {
  createBuiltinFunc("exit", &builtin_exit);
  createBuiltinFunc("help", &builtin_help);
  createBuiltinFunc("ls", &builtin_ls);
  //createBuiltinFunc("chmod", &buitin_chmod);
  createBuiltinFunc("mkdir", &builtin_mkdir);
  createBuiltinFunc("rmdir", &builtin_rmdir);
  createBuiltinFunc("cd", &builtin_cd);
  createBuiltinFunc("pwd", &builtin_pwd);
  //createBuiltinFunc("cat", &builtin_cat);
  //createBuiltinFunc("more", &builtin_more);
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

string build_path(Vnode *node){
  Vnode *temp = node;
  string res = "/root";
  while (temp != g_root_directory){
    string name = "/" + string(temp->name);
    res.insert(5, temp->name);
    temp = temp->parent;
  }
}


int builtin_exit(vector<string> const& argv) {
  exit(0);
  return 0;
}

int builtin_help(vector<string> const& argv) {
  return 0;
}

int builtin_ls(vector<string> const& argv){
  if (argv.size() > 1) return -1;
  string filepath = build_path(g_cur_directory);
  int dd = f_opendir(filepath.c_str());
  Stat* nodeStat = f_readdir(dd);
  while (nodeStat != NULL){
    if (argv.size() == 1){
      if (argv[0] == "-F"){
	if (nodeStat->type == 1){
	  cout << nodeStat->name << "/ /t" << endl;
	} else {
	  cout << nodeStat->name << "/t" << endl;
	}
      } else if (argv[0] == "-l"){
	//flag -l
      } else {
	//errno
	return -1;
      }
    } else {
      cout << nodeStat->name << "/t" << endl;
    }
    nodeStat = f_readdir(dd);
  }
  return 0;
}

int builtin_mkdir(vector<string> const& argv){
  if (argv.size() > 1) return -1;
  string filepath = build_path(g_cur_directory);
  filepath.append("/" + argv[0]);
  return f_mkdir(filepath.c_str());
}

int builtin_rmdir(vector<string> const& argv){
  if (argv.size() > 1) return -1;
  string filepath = build_path(g_cur_directory);
  filepath.append("/" + argv[0]);
  return f_rmdir(filepath.c_str());
}

int builtin_cd(vector<string> const& argv){
  if (argv.size() > 1) return -1;
  if (argv[0] == "."){
    //?? do nothing?
    return 0;
  } else if (argv[0] == ".."){
    g_cur_directory = g_root_directory;
    return 0;
  } else {
    vector<string> filenames;
    g_filename_tokenizer.parseString(argv[0], filenames);
    Vnode *nextDir = findVnode(filenames, 1);
    if (!nextDir){
      //errno
      return -1;
    }
    g_cur_directory = nextDir;
    return 0;
  }
}

int builtin_pwd(vector<string> const& argv){
  string filepath = build_path(g_cur_directory);
  cout << filepath << endl;
  return 0;
}

int builtin_rm(vector<string> const& argv){
  if (argv.size() == 0 || argv.size() > 2 || (argv.size() == 2 && argv[0] != "-r")) return -1;
  string filepath = build_path(g_cur_directory);
  if (argv.size() == 1){
    filepath.append("/" + argv[0]);
    return f_remove(filepath.c_str());
  } else {
    filepath.append("/" + argv[1]);
    return f_rmdir(filepath.c_str());
  }
}
