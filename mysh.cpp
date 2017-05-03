


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
int 					g_data_offset;
FileTable 				g_file_table;
FatTable				g_fat_table;
SuperBlock				g_superblock;
Vnode*					g_root_directory;	


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



void mainLoop(void) {
	string line;
	vector<Command> commands;
	int status;

	do {
		string curPath = string(getcwd(NULL, 0));
		cout << curPath << ": ";
		fflush(stdout);

		line = readLine();

		parseLine(line, commands);

		for (int i = 0; i < commands.size(); i++) {
			status = executeCommand(commands[i]);
		}

		line.clear();
		commands.clear();
	} while (status >= 0);
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
		perror("lseek root directory block failed");
		return -1;
	}

	g_root_directory = new Vnode;

	res = read(g_disk_fd, g_root_directory, sizeof(Vnode));
	if (res < sizeof(Vnode)) {
		perror("read root directory failed");
		return -1;
	}

	// cout << *g_root_directory << endl;

	return 1;
}




int main(int argc, char** argvs) {

	vector<string> argv(argvs, argvs + argc);
	if (argc != 2) {
		cout << "Usage: mysh <diskname>" << endl;
		return -1;
	} 

	if (initFS(argv[1]) == -1) {
		return -1;
	}


	int fd = f_open("~/test.txt", F_READ);
	if (fd == -1) {
		cout << "bad fd: " << fd << endl;
		return -1;
	}

	cout << string(g_file_table[fd].vnode->name) << endl;
	cout << hex << g_file_table[fd].flag << endl;

	char buf[255];
	int res = f_read(buf, 1, 255, fd);
	if (res < 255) {
		cout << "bad read: " <<  res << endl;
		return -1;
	}
	
	cout << string(buf) << endl;


	printf("Welcome to S&J's Shell!"
			" You can type in 'help' to look for some instructions for this shell\n");

	// mainLoop();



	return 0;
}
