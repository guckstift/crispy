#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define NULL_VALUE ((Value){.type = TY_NULL})

typedef enum {
	TY_NULL,
	TY_BOOL,
	TY_INT,
	TY_STRING,
	TY_FUNCTION,
} Type;

typedef struct Value {
	Type type;
	union {
		int64_t value;
		char *string;
		struct Value (*func)();
	};
} Value;

static void error(char *msg) {
	fprintf(stderr, "error: %s\n", msg);
	exit(EXIT_FAILURE);
}

static void print(Value value) {
	switch(value.type) {
		case TY_NULL:
			printf("null");
			break;
		case TY_BOOL:
			printf("%s", value.value ? "true" : "false");
			break;
		case TY_INT:
			printf("%li", value.value);
			break;
		case TY_STRING:
			printf("%s", value.string);
			break;
		case TY_FUNCTION:
			printf("<function %p>", *(void**)&value.func);
			break;
	}
}

static Value check_type(Type mintype, Type maxtype, Value value)
{
	if(value.type < mintype || value.type > maxtype) {
		error("wrong type");
	}
	
	return value;
}
