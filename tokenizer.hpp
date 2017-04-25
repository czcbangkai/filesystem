#ifndef _TOKENIZER_HPP
#define _TOKENIZER_HPP

#include <iostream>
#include <string>

using namespace std;



class Tokenizer {
public:
	Tokenizer(string delim, string special);
	~Tokenizer(void);

	void setString(string input);
	string getNextToken(void);

private:
	string _delim;
	string _special;
	string _input;
	int _pos;
};

#endif //_TOKENIZER_HPP