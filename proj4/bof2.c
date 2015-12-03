#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int fnc()
{
		char buffer[256] = "";
		gets(buffer);
		printf(buffer,buffer);
}

int main(int argc, char **argv) {

		
		for(;;)
		{
				fnc();
		}
		return 0;
}
