#ifndef _UTLITIES_HPP
#define _UTLITIES_HPP

#include <cstdio>
#include <errno.h>
#include <iostream>

using namespace std;


void print_error(int line_no, const char* func_name, const char* custom);
bool regexMatch(string str, string pattern);
bool isDigits(string const& str);
string decToOct(int dec);
char** stringVec2CharDoublePtr(vector<string> const& vec);



#endif // _UTLITIES_HPP