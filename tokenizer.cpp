#include <iostream>
#include <string>
#include <vector>
#include <cstring>

#include "tokenizer.hpp"

Tokenizer::Tokenizer(string delim, string special) : _pos(0) {
	_delim = delim;
	_special = special;
}

Tokenizer::~Tokenizer(void) {}

void Tokenizer::setString(string input) {
	_input = input + ' ';
	_pos = 0;
}

string Tokenizer::getNextToken(void) {
	string res = "";

	while (_pos < _input.length()) {
		bool in_delim = strchr(_delim.c_str(), _input[_pos]);
		bool in_special = strchr(_special.c_str(), _input[_pos]);

		if (in_delim) {
			if (res != "") {
				_pos++;
				return res;
			}
		}
		else if (in_special) {
			if (res == "") {
				res += _input[_pos];
				_pos++;

				if (res == ">") {
					if (_pos < _input.length() && _input[_pos] == '>') {
						res += _input[_pos];
						_pos++;
					}
				}
			}
			return res;
		}
		else {
			res += _input[_pos];
		}

		_pos++;
	}

	return "";
}

void Tokenizer::parseString(string input, vector<string>& tokens) {
	setString(input);
	string token;
	do {
		token = getNextToken();
		if (token != "") {
			tokens.push_back(token);
		}
		else {
			return;
		}
	} while (token != "");
}



// int main(void) {

// 	Tokenizer tok(" /", "");
// 	string s = "~/d1/d2/d3/file";
// 	tok.setString(s);
// 	string t;
// 	do {
// 		t = tok.getNextToken();
// 		cout << "|" << t << "|" << endl;
// 	} while (t != "");


// 	return 0;
// }
