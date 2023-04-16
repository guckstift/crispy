#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum {
	TY_NULL,
	TY_BOOL,
	TY_INT,
	TY_FUNCTION,
} Type;

typedef struct {
	Type type;
	union {
		int64_t value;
		void (*func)();
	};
} Value;

void print(Value value) {
	switch(value.type) {
		case TY_NULL:
			printf("null\n");
			break;
		case TY_BOOL:
			printf("%s\n", value.value ? "true" : "false");
			break;
		case TY_INT:
			printf("%li\n", value.value);
			break;
		case TY_FUNCTION:
			printf("<function %p>\n", *(void**)&value.func);
			break;
	}
}

static void error(char *msg) {
	fprintf(stderr, "error: %s\n", msg);
	exit(EXIT_FAILURE);
}
