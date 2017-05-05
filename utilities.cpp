#include <cstring>
#include <errno.h>
#include <regex.h>

#include <iostream>
#include <sstream>
#include <vector>

#include "utilities.hpp"

using namespace std;



void print_error(int line_no, const char* func_name, const char* custom) {
	fprintf(stderr, "[Line:%d][%s] %s: %s\n", line_no, func_name, custom, strerror(errno));
}


bool regexMatch(string str, string pattern) {
	int status;
	regex_t reg;

	if (regcomp(&reg, pattern.c_str(), REG_EXTENDED | REG_NOSUB)) {
	    return false;
	}
	status = regexec(&reg, str.c_str(), (size_t)0, NULL, 0);
	regfree(&reg);
	
	return (bool)! status;
}

bool isDigits(string const& str) {
	return str.find_first_not_of("0123456789") == string::npos;
}


string decToOct(int dec) {
	ostringstream o;
    o << oct << dec;
    return o.str();
}


char** stringVec2CharDoublePtr(vector<string> const& vec) {
	if (vec.empty()) {
		return NULL;
	}

	char** cstrs = new char*[vec.size() + 1];
	for (int i = 0; i < vec.size(); i++) {
		cstrs[i] = new char[vec[i].length() + 1];
		strcpy(cstrs[i], vec[i].c_str());
	}
	cstrs[vec.size()] = NULL;

	return cstrs;
}
