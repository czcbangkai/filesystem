#ifndef _UTLITIES_HPP
#define _UTLITIES_HPP

#include <iostream>

using namespace std;



bool regexMatch(string str, string pattern);
bool isDigits(string const& str);
char** stringVec2CharDoublePtr(vector<string> const& vec);



#endif // _UTLITIES_HPP