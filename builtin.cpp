#include <cstdlib>

#include <unistd.h>

#include <iostream>
#include <vector>
#include <unordered_map>

#include "builtin.hpp"

using namespace std;


BuiltinList g_builtinList;


BuiltinList::BuiltinList(void) {
  createBuiltinFunc("exit", &builtin_exit);
  createBuiltinFunc("help", &builtin_help);
  createBuiltinFunc("ls", &builtin_ls);
  createBuiltinFunc("chmod", &buitin_chmod);
  createBuiltinFunc("mkdir", &builtin_mkdir);
  createBuiltinFunc("rmdir", &builtin_rmdir);
  createBuiltinFunc("cd", &builtin_cd);
  createBuiltinFunc("pwd", &builtin_pwd);
  createBuiltinFunc("cat", &builtin_cat);
  createBuiltinFunc("more", &builtin_more);
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

int builtin_exit(vector<string> const& argv) {
  exit(0);
  return 0;
}

int builtin_help(vector<string> const& argv) {
  return 0;
}

int builtin_ls(vector<string> const& argv){
  
}
