#ifndef _BUILTIN_HPP
#define _BUILTIN_HPP


#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "utilities.hpp"

using namespace std;



typedef int (*BuiltinFunc)(vector<string> const& argv);

class BuiltinList {
public:
	BuiltinList(void);
	~BuiltinList(void);

	void createBuiltinFunc(string const& funcName, BuiltinFunc funcPtr);
	BuiltinFunc findBuiltinFunc(string const& funcName);

private:
	unordered_map<string, BuiltinFunc> _builtinList;
};



extern BuiltinList g_builtinList;



int builtin_exit(vector<string> const& argv);
int builtin_help(vector<string> const& argv);
int builtin_ls(vector<string> const& argv);
int builtin_chmod(vector<string> const& argv);
int builtin_mkdir(vector<string> const& argv);
int builtin_rmdir(vector<string> const& argv);
int builtin_cd(vector<string> const& argv);
int builtin_pwd(vector<string> const& argv);
int builtin_cat(vector<string> const& argv);
int builtin_more(vector<string> const& argv);
int builtin_rm(vector<string> const& argv);



#endif // _BUILTIN_HPP
