#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "filesys.h"

// gcc -o mycheck mycheck.c filesys.c -Wall -Werror -lssl -lcrypto -g

int main(int argc, char const *argv[])
{
	int fd1, fd2;
	fd1 = open ("foo_0.txt", O_WRONLY, 0);

	int eof = lseek(fd1,0,SEEK_END);
	printf("Actual eof is %d\n",eof);
	
	fd2 = s_open ("foo_0.txt", O_WRONLY, 0);//entry of foo_0 created in secure.txt

	int myeof = s_lseek (fd2, 0, SEEK_END);
	
	printf("My eof is %d\n",myeof);//should differ by 1

	return 0;
}