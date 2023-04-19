#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#define NULL_VALUE ((Value){.type = TY_NULL})

#define INT_BINOP(left, op, right) \
	(Value){ \
		.type = TY_INT, \
		.value = check_type( \
			TY_NULL, TY_INT, (left) \
		).value op check_type( \
			TY_NULL, TY_INT, (right) \
		).value \
	} \

typedef enum {
	TY_NULL,
	TY_BOOL,
	TY_INT,
	TY_STRING,
	TY_ARRAY,
	TY_FUNCTION,
} Type;

typedef struct Value {
	Type type;
	union {
		int64_t value;
		char *string;
		struct Array *array;
		struct Value (*func)();
	};
} Value;

typedef struct Array {
	int64_t length;
	Value items[];
} Array;

static void print(Value value);

static void error(char *msg, ...) {
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "error: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

static void print_repr(Value value) {
	if(value.type == TY_STRING) {
		printf("\"");
		print(value);
		printf("\"");
	}
	else {
		print(value);
	}
}

static void print_array(Array *array) {
	printf("[");
	
	for(int64_t i=0; i < array->length; i++) {
		if(i > 0) {
			printf(", ");
		}
		
		print_repr(array->items[i]);
	}
	
	printf("]");
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
		case TY_ARRAY:
			print_array(value.array);
			break;
		case TY_FUNCTION:
			printf("<function %p>", *(void**)&value.func);
			break;
	}
}

static Value check_type(Type mintype, Type maxtype, Value value) {
	if(value.type < mintype || value.type > maxtype) {
		error("wrong type");
	}
	
	return value;
}

static Array *new_array(int64_t length, ...) {
	Array *array = calloc(1, sizeof(Array) + length * sizeof(Value));
	array->length = length;
	
	va_list args;
	va_start(args, length);
	
	for(int64_t i=0; i < length; i++) {
		array->items[i] = va_arg(args, Value);
	}
	
	va_end(args);
	return array;
}

static Value call(Value value, char *name) {
	if(value.type != TY_FUNCTION) {
		error("%s is not callable", name);
	}
	
	return value.func();
}
