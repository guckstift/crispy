#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "build.h"
#include "print.h"
#include "lex.h"
#include "parse.h"
#include "analyze.h"
#include "generate.h"
#include "runtime.h.xxdi"
#include "runtime.c.xxdi"

static char *cache_dir = 0;

static void error(char *msg)
{
	fprintf(stderr, P_ERROR "error:" P_RESET " %s\n", msg);
	exit(EXIT_FAILURE);
}

static char *string_concat(char *first, ...)
{
	int64_t length = 0;
	char *string = 0;
	char *item = first;
	va_list args;
	va_start(args, first);
	
	while(item) {
		int64_t item_length = strlen(item);
		string = realloc(string, length + item_length + 1);
		memcpy(string + length, item, item_length);
		length += item_length;
		string[length] = 0;
		item = va_arg(args, char*);
	}
	
	va_end(args);
	return string;
}

static char *create_path_id(char *path)
{
	int64_t path_length = strlen(path);
	int64_t buffer_length = path_length * 3 + 1;
	char *buffer = malloc(buffer_length);
	char *inptr = path;
	char *outptr = buffer;
	
	while(*inptr) {
		if(isalnum(*inptr)) {
			*outptr = *inptr;
			outptr ++;
		}
		else {
			uint8_t code = *inptr;
			outptr[0] = '_';
			outptr[1] = (code >>  4) + 'A';
			outptr[2] = (code & 0xf) + 'A';
			outptr += 3;
		}
		
		inptr ++;
	}
	
	*outptr = 0;
	return buffer;
}

Module *build_module(char *filename)
{
	Module *module = calloc(1, sizeof(Module));
	module->filename = filename;
	module->pathid = create_path_id(filename);
	module->cfilename = string_concat(cache_dir, "/", module->pathid, ".c", 0);
	
	FILE *file = fopen(module->filename, "r");
	
	if(file == NULL) {
		error("could not open input file");
	}
	
	fseek(file, 0, SEEK_END);
	module->srcsize = ftell(file);
	rewind(file);
	
	module->src = malloc(module->srcsize + 1);
	module->src[module->srcsize] = 0;
	fread(module->src, 1, module->srcsize, file);
	
	lex(module);
	print_tokens(module->tokens);
	
	parse(module);
	analyze(module);
	print_module_block(module->body);
	
	generate(module);
	
	return module;
}

static bool make_dir(char *dirpath)
{
	DIR *dir = opendir(dirpath);
	
	if(dir == 0) {
		mkdir(dirpath, 0700);
	}
	else {
		closedir(dir);
	}
}

static char *copy_runtime_file(char *name, unsigned char *text, int64_t size)
{
	char *path = string_concat(cache_dir, "/", name, 0);
	FILE *fs = fopen(path, "wb");
	fwrite(text, 1, size, fs);
	fclose(fs);
	return path;
}

Project *build_project(char *filename)
{
	cache_dir = string_concat(getenv("HOME"), "/.crispy", 0);
	make_dir(cache_dir);
	copy_runtime_file("runtime.h", src_runtime_h, src_runtime_h_len);
	
	char *runtime_c_path = copy_runtime_file(
		"runtime.c", src_runtime_c, src_runtime_c_len
	);
	
	filename = realpath(filename, NULL);
	Project *project = calloc(1, sizeof(Project));
	project->main = build_module(filename);
	project->exename = string_concat(cache_dir, "/", project->main->pathid, 0);
	
	char *gcc_cmd = string_concat(
		"gcc -o ", project->exename, " -std=c17 -pedantic-errors ",
		runtime_c_path, " ", project->main->cfilename, 0
	);
	
	system(gcc_cmd);
	system(project->exename);
	return project;
}
