


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <errno.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <vector>

#include "mysh.hpp"
#include "tokenizer.hpp"
#include "builtin.hpp"
#include "utilities.hpp"
#include "vfs.hpp"
#include "fslib.hpp"

using namespace std;



// GLOBAL 

Tokenizer 				g_tokenizer(DELIM, SPECIAL_DELIM);
int						g_disk_fd;
FileTable 				g_file_table;
FatTable				g_fat_table;
SuperBlock				g_superblock;
Vnode*					g_root_vnode;	
Vnode*					g_cur_vnode;
vector<string>			g_cur_dir;
int						g_super_user;


// SIGNAL

void resetSignalHandler(void) {
	for (int i = SIGHUP; i<= SIGUSR2; i++) {
		signal(i, SIG_DFL);
	}
}



// COMMAND 

Command::Command(void) {
	argv.clear();
	bg_flag = false;
}

Command::~Command(void) {}

ostream& operator<<(ostream& os, Command const& cmd) {
	for (int i = 0; i < cmd.argv.size(); i++) {
		os << cmd.argv[i] << " ";
	}
	return os << endl;
}


string readLine(void) {
	char* line = NULL;
	size_t size = 0;
	if (getline(&line, &size, stdin) == -1) {
		//error handling
	}
	line[strcspn(line, "\r\n")] = 0;
	return string(line);
}



void parseLine(string& line, vector<Command>& output) {
	g_tokenizer.setString(line);

	Command cmd;
	string token;

	while ((token = g_tokenizer.getNextToken()) != "") {
		if (token == "&") {
			cmd.bg_flag = true;
		}
		
		if (token == "&" || token == ";") {
			output.push_back(cmd);

			cmd.argv.clear();
			cmd.bg_flag = false;

			continue;
		}

		cmd.argv.push_back(token);
	}

	if (! cmd.argv.empty()) {
		output.push_back(cmd);
	}
}



int executeCommand(Command const& cmd) {
	if (cmd.argv.empty() || cmd.argv[0] == "") {
		return 1;
	}

	BuiltinFunc func = g_builtinList.findBuiltinFunc(cmd.argv[0]);
	if (func) {
		return (*func)(cmd.argv);
	}

	return executeSystem(cmd);
}



int executeSystem(Command const& cmd) {
	if (cmd.argv.empty() || cmd.argv[0] == "") {
		return 1;
	}

	pid_t pid;
	int status;

	pid = fork();
	if (! pid) {
		resetSignalHandler();

		char** argv = stringVec2CharDoublePtr(cmd.argv);

		if (execvp(argv[0], argv) == -1) {
			//error handling
		}
	}
	else if (pid > 0) {
		do {
			if (waitpid(pid, &status, WUNTRACED) == -1) {
				//error handling
			}
		} while (! WIFEXITED(status) && ! WIFSIGNALED(status));
	}
	else {
		//error handling				
	}

	return 1;
}



string getCurrentPath(void) {
	string res;
	for (int i = 0; i < g_cur_dir.size(); i++) {
		res += g_cur_dir[i] + "/";
	}
	return res;
}



void mainLoop(void) {
	string line;
	vector<Command> commands;
	int status;

	do {
		string curPath = getCurrentPath();
		cout << curPath << ": ";
		fflush(stdout);

		line = readLine();

		parseLine(line, commands);

		for (int i = 0; i < commands.size(); i++) {
			status = executeCommand(commands[i]);
		}

		line.clear();
		commands.clear();
	} while (true);
}




int initFS(string diskname) {
	g_disk_fd = open(diskname.c_str(), O_RDWR, 0666);
	if (g_disk_fd == -1) {
		perror("open disk file failed");
		return -1;
	}

	int res;
	res = read(g_disk_fd, &g_superblock, sizeof(SuperBlock));
	if (res < sizeof(SuperBlock)) {
		perror("read superblock failed");
		return -1;
	}

	// cout << g_superblock << endl;

	res = lseek(g_disk_fd, g_superblock.blocksize * g_superblock.fat_offset, SEEK_SET);
	if (res == -1) {
		perror("lseek fat table failed");
		return -1;
	}

	res = read(g_disk_fd, &g_fat_table, sizeof(g_fat_table));
	if (res < sizeof(g_fat_table)) {
		perror("read fat table failed");
		return -1;
	}

	// cout << g_fat_table << endl;

	res = lseek(g_disk_fd, g_superblock.blocksize * g_superblock.data_offset, SEEK_SET);
	if (res == -1) {
		perror("lseek root vnode block failed");
		return -1;
	}

	g_root_vnode = new Vnode;

	res = read(g_disk_fd, g_root_vnode, sizeof(Vnode));
	if (res < sizeof(Vnode)) {
		perror("read root vnode failed");
		return -1;
	}

	// cout << *g_root_vnode << endl;

	g_cur_vnode = g_root_vnode;

	g_cur_dir.push_back("~");

	return 1;
}




int main(int argc, char** argvs) {

	vector<string> argv(argvs, argvs + argc);
	if (argc != 2) {
		cout << "Usage: mysh <diskname>" << endl;
		return -1;
	} 

	g_super_user = 1;
	signal(SIGINT, SIG_IGN);




	if (initFS(argv[1]) == -1) {
		return -1;
	}

	// cout << "root children: " << g_root_vnode->size << endl;

	// int res;
	// int fd = f_open("~/hello.txt", F_READ | F_WRITE);
	// if (fd == -1) {
	// 	cout << "bad fd: " << fd << endl;
	// 	return -1;
	// }

	// cout << string(g_file_table[fd].vnode->name) << endl;
	// cout << hex << g_file_table[fd].flag << endl;
	// cout << dec << "offset now: " << g_file_table[fd].offset << endl;


	// char buf2[512] = "yeah right some stuff!!!!!!!!!!!!!!!!!!!!!!!!!!";
	// res = f_write(buf2, sizeof(buf2), 1, fd);
	// cout << "written bytes: " << res << endl;
	// if (res < 1) {
	// 	cout << "bad write" << endl;
	// }

	// cout << dec << "offset now2: " << g_file_table[fd].offset << endl;

	// res = f_seek(0, S_SET, fd);

	// cout << dec << "offset now3: " << g_file_table[fd].offset << endl;


	// do {
	// 	char buf1[BLOCKSIZE + 1] = {0};
	// 	res = f_read(buf1, BLOCKSIZE, 1, fd);
	// 	if (res < 1) {
	// 		cout << "bad read: " <<  res << endl;
	// 	}
		
	// 	cout << string(buf1) << endl;
	// } while (res > 0);


	// cout << dec << "offset now4: " << g_file_table[fd].offset << endl;

	// char buf2[255] = "Some new string to test.";

	// res = f_seek(50, S_SET, fd);
	// if (fd == -1) {
	// 	cout << "fseek failed" << endl;
	// 	return -1;
	// }
	// cout << dec << "offset now: " << g_file_table[fd].offset << endl;

	// res = f_write(buf2, 1, 50, fd);
	// if (res < 50) {
	// 	cout << "bad write: " <<  res << endl;
	// 	return -1;
	// }
	// cout << "write done: " << res << endl;



	// int fd = f_open("~/hello.txt", F_APPEND);
	// cout << "cur fd: " << fd << endl;

	// if (fd == -1) {
	// 	return -1;
	// }

	// char buf3[20] = "testing ENDSSSS";

	// res = f_write(buf3, 1, sizeof(buf3), fd);
	// cout << "written: " << res << endl;
	// if (res < sizeof(buf3)) {
	// 	cout << "write less" << endl;
	// 	return -1;
	// }
	// cout << "write done" << endl;



	printf("Welcome to S&J's Shell!"
			" You can type in 'help' to look for some instructions for this shell\n");

	mainLoop();



	return 0;
}
