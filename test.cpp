
#include <cstring>
#include <iostream>
#include <string>

using namespace std;

int main(void) {

	unsigned short a[1000];
	for (int i = 0; i < 1000; i++) {
		a[i] = (unsigned short)65535;
	}

	for (int i = 0; i < 1000; i++) {
		cout << a[i] << endl;
	}



	return 0;
}