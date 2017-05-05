
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <bitset>

// #include "vfs.hpp"
// #include "mysh.hpp"

using namespace std;

int main(void) {

	int res;

	// int fd = open("disk", O_RDWR, 0666);
	// if (fd == -1) {
	// 	perror("open");
	// 	return -1;
	// }

	// SuperBlock sb;
	// res = read(fd, &sb, sizeof(SuperBlock));
	// if (res < sizeof(SuperBlock)) {
	// 	perror("read sb");
	// 	return -1;
	// }


	// res = lseek(fd, BLOCKSIZE * (sb.data_offset + 2), SEEK_SET);
	// cout << BLOCKSIZE * (sb.data_offset + 2) << endl;
	// if (res == -1) {
	// 	perror("lseek test failed");
	// 	return -1;
	// }


	// char buf[BLOCKSIZE];
	// res = read(fd, buf, BLOCKSIZE);
	// if (res < BLOCKSIZE) {
	// 	perror("read test block failed");
	// 	return -1;
	// }

	// for (int i = 0; i < BLOCKSIZE; i++) {
	// 	if (buf[i])	
	// 		cout << (char)(buf[i]) << endl;
	// }

	bitset<16> x(15);
	cout << hex << x << endl;

	return 0;
}