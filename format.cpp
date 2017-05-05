
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


#include "vfs.hpp"

#define DISKSIZE	512

using namespace std;

void showHelp(void) {
	cout << "To use format, you can either use 2 or 4 arguments." << endl;
	cout << "For 2, use 'format <diskname>'" << endl;
	cout << "For 4, use 'format <diskname> -s #', where '#' indicates the size of the disk." << endl;
}

int main(int argc, char** argvs) {
	if (argc < 2 || argc == 3 || argc > 4) {
		showHelp();
		return -1;
	}

	vector<string> argv(argvs, argvs + argc);
	
	string diskName = argv[1];
	int diskSize = DISKSIZE;

	if (argv.size() > 2) {
		diskSize = stoi(argv[3]);
	}


	SuperBlock superBlock;
	superBlock.blocksize = BLOCKSIZE;
	superBlock.fat_offset = 1;
	superBlock.data_offset = (superBlock.fat_offset * superBlock.blocksize + sizeof(unsigned short) * FATSIZE) 
								/ superBlock.blocksize + 1;

	FatTable fatTable;

	Vnode vnode;
	strncpy(vnode.name, "~", 255);
	vnode.uid = 0;
	vnode.gid = 0;
	vnode.size = 1;
	vnode.parent = NULL;
	vnode.address = superBlock.blocksize * superBlock.data_offset;
	vnode.permission = 0755;
	vnode.type = 1;
	vnode.timestamp = time(NULL);
	vnode.fatPtr = 1;

	cout << vnode << endl;

	Vnode test;
	strncpy(test.name, "test.txt", 255);
	test.uid = 0;
	test.gid = 0;
	test.size = BLOCKSIZE * 3;
	test.parent = NULL;
	test.address = superBlock.blocksize * (superBlock.data_offset + 2);
	test.permission = 0755;
	test.type = 0;
	test.timestamp = time(NULL);
	test.fatPtr = 2;

	// cout << test << endl;

	fatTable[0] = 1;
	fatTable[1] = fatTable[4] = EOBLOCK;
	fatTable[2] = 3;
	fatTable[3] = 4;



	int fd = open(argv[1].c_str(), O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
		perror("open");
		exit(-1);
	}

	int res;
	res = ftruncate(fd, diskSize * 1024 * 1024);
	if (res == -1) {
		perror("ftruncate disk failed");
		exit(-1);
	}

	res = lseek(fd, 0, SEEK_SET);
	if (res == -1) {
		perror("lseek to the beginning of the file failed");
		exit(-1);
	}

	res = write(fd, &superBlock, sizeof(superBlock));
	if (res < sizeof(superBlock)) {
		perror("write superBlock");
		exit(-1);
	}

	res = lseek(fd, superBlock.fat_offset * superBlock.blocksize, SEEK_SET);
	if (res == -1) {
		perror("lseek fat start");
		exit(-1);
	}

	res = write(fd, &fatTable, sizeof(fatTable));
	if (res < sizeof(fatTable)) {
		perror("write fatTable");
		exit(-1);
	}

	res = lseek(fd, superBlock.data_offset * superBlock.blocksize, SEEK_SET);
	if (res == -1) {
		perror("lseek data block start");
		exit(-1);
	}

	res = write(fd, &vnode, sizeof(vnode));
	if (res < sizeof(vnode)) {
		perror("write root vnode");
		exit(-1);
	}

	res = lseek(fd, (superBlock.data_offset + 1) * superBlock.blocksize, SEEK_SET);
	if (res == -1) {
		perror("lseek data block start");
		exit(-1);
	}

	res = write(fd, &test, sizeof(test));
	if (res < sizeof(test)) {
		perror("write test vnode");
		exit(-1);
	}

	res = lseek(fd, (superBlock.data_offset + 2) * superBlock.blocksize, SEEK_SET);
	if (res == -1) {
		perror("lseek data block 2 start");
		exit(-1);
	}

	char buf[BLOCKSIZE] = {0};
	memset(buf, '.', sizeof(buf));

	res = write(fd, buf, sizeof(buf));
	if (res < sizeof(buf)) {
		perror("write test data failed");
		exit(-1);
	}

	memset(buf, '?', sizeof(buf));

	res = write(fd, buf, sizeof(buf));
	if (res < sizeof(buf)) {
		perror("write test data failed");
		exit(-1);
	}

	memset(buf, '!', sizeof(buf));

	res = write(fd, buf, sizeof(buf));
	if (res < sizeof(buf)) {
		perror("write test data failed");
		exit(-1);
	}

	close(fd);

	return 0;
}