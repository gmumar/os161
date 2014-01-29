/*
 * Simple program to add two numbers (given in as arguments). Used to
 * test argument passing to child processes.
 *
 * Intended for the basic system calls assignment; this should work
 * once execv() argument handling is implemented.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
static int tmp =5;
static int tmp2 =4;

int helo(int tmp3){
	if (tmp2 == 4096){
		return 0;
	}
	printf("%x %x \n",tmp, tmp2);
	helo(tmp2++);
}

int
main(int argc, char *argv[])
{
	int i, j;
	//helo(tmp2);
	printf("Addr Tmp: %x Tmp2: %x \n", &tmp,&tmp2);

	if (argc != 3) {
		//errx(1, "Usage: add num1 num2");
	}

	i = atoi(argv[1]);
	j = atoi(argv[2]);

	printf("Answer: %d\n", i+j);

	return 0;
}
