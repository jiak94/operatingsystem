#include <stdio.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char **argv) {
		char buffer[256] = "";
		if(argc != 2) {
				exit(0);
		}
		printf("buffer address: %p\n", buffer);
		strcpy(buffer, argv[1]);
		return 0;
}
