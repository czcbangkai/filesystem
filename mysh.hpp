#ifndef _MYSH_HPP
#define _MYSH_HPP



#include <iostream>
#include <vector>

#include "tokenizer.hpp"
#include "builtin.hpp"
#include "vfs.hpp"

using namespace std;

#define DELIM 					" \n\t\r\a\0"
#define SPECIAL_DELIM			"|&;()<>"




class Command {
public:
	Command(void);
	~Command(void);
	friend ostream& operator<<(ostream& os, Command const& cmd);

	vector<string> argv;
	bool bg_flag;
};

extern Tokenizer				g_tokenizer;
extern unsigned short			g_FAT_table[FATSIZE];
extern int						g_disk_fd;
extern int 						g_data_offset;
extern superblock				g_superblock;




string readLine(void);
void parseLine(string& line, vector<Command>& output);
int executeCommand(Command const& cmd);
int executeSystem(Command const& cmd);



#endif //_MYSH_HPP