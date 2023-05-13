#include <stdio.h>
#include <stdlib.h>
#include "print.h"
#include "build.h"
#include "string.h"

static void error(char *msg)
{
	fprintf(stderr, P_ERROR "error:" P_RESET " %s\n", msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		error("not enough arguments");
	}
	
	char *filename = argv[1];
	build_project(filename);
	return 0;
}
