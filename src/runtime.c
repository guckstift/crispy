#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#define NULL_VALUE ((Value){.type = TY_NULL})
#define NULL_VALUE_INIT {.type = TY_NULL}
#define BOOL_VALUE(v) ((Value){.type = TY_BOOL, .value = v})
#define BOOL_VALUE_INIT(v) {.type = TY_BOOL, .value = v}
#define INT_VALUE(v) ((Value){.type = TY_INT, .value = v})
#define INT_VALUE_INIT(v) {.type = TY_INT, .value = v}
#define STRING_VALUE(v) ((Value){.type = TY_STRING, .string = v})
#define STRING_VALUE_INIT(v) {.type = TY_STRING, .string = v}
#define ARRAY_VALUE(v) ((Value){.type = TY_ARRAY, .array = v})
#define FUNCTION_VALUE(v) ((Value){.type = TY_FUNCTION, .func = v})
#define FUNCTION_VALUE_INIT(v) {.type = TY_FUNCTION, .func = v}

#define UNINITIALIZED {.type = TYX_UNINITIALIZED}

#define INT_BINOP(left, op, right) \
	(Value){ \
		.type = TY_INT, \
		.value = check_type( \
			TY_NULL, TY_INT, (left) \
		).value op check_type( \
			TY_NULL, TY_INT, (right) \
		).value \
	} \

#define RETURN(x) { \
	Value result = x; \
	cur_scope_frame = cur_scope_frame->parent; \
	return result; \
} \

typedef enum {
	TY_NULL,
	TY_BOOL,
	TY_INT,
	TY_STRING,
	TY_ARRAY,
	TY_FUNCTION,
	
	TYX_UNINITIALIZED,
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

typedef struct PrintFrame {
	struct PrintFrame *parent;
	Value value;
} PrintFrame;

typedef struct ScopeFrame {
	struct ScopeFrame *parent;
	Value *values;
	int64_t length;
} ScopeFrame;

typedef struct MemBlock {
	struct MemBlock *next;
	int64_t mark;
	char data[];
} MemBlock;

static void print(Value value);
static void value_decref(Value value);

static PrintFrame *cur_print_frame = 0;
static ScopeFrame *cur_scope_frame = 0;
static MemBlock *first_block = 0;
static MemBlock *last_block = 0;
static int64_t block_count = 0;

static Value *error(char *msg, ...) {
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "error: ");
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
	return &NULL_VALUE;
}

static Value *check_var(Value *var, char *name)
{
	if(var->type == TYX_UNINITIALIZED) {
		error("name %s is not defined", name);
	}
	
	return var;
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
	for(
		PrintFrame *frame = cur_print_frame->parent;
		frame; frame = frame->parent
	) {
		if(frame->value.type == TY_ARRAY && frame->value.array == array) {
			printf("[...]");
			return;
		}
	}
	
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
	PrintFrame frame = {.parent = cur_print_frame, .value = value};
	cur_print_frame = &frame;
	
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
	
	cur_print_frame = cur_print_frame->parent;
}

static Value check_type(Type mintype, Type maxtype, Value value) {
	if(value.type < mintype || value.type > maxtype) {
		error("wrong type");
	}
	
	return value;
}

static void gc_mark(Value value)
{
	if(value.type == TY_ARRAY) {
		Array *array = value.array;
		MemBlock *block = (MemBlock*)array;
		block --;
		
		if(block->mark == 1) {
			return;
		}
		
		block->mark = 1;
		
		for(int64_t i=0; i < array->length; i++) {
			gc_mark(array->items[i]);
		}
	}
}

static void collect_garbage()
{
	for(ScopeFrame *frame = cur_scope_frame; frame; frame = frame->parent) {
		for(int64_t i=0; i < frame->length; i++) {
			Value value = frame->values[i];
			gc_mark(value);
		}
	}
	
	for(MemBlock *block = first_block, *prev = 0; block;) {
		if(block->mark == 0) {
			((int64_t*)block->data)[0] = 0;
			
			if(block == first_block) {
				first_block = block->next;
				free(block);
				block = first_block;
			}
			else {
				prev->next = block->next;
				free(block);
				block = prev->next;
			}
			
			block_count --;
		}
		else {
			block->mark = 0;
			prev = block;
			block = block->next;
		}
		
		last_block = prev;
	}
}

static void *mem_alloc(int64_t size) {
	collect_garbage();
	MemBlock *block = calloc(1, sizeof(MemBlock) + size);
	
	if(first_block) {
		last_block->next = block;
	}
	else {
		first_block = block;
	}
	
	last_block = block;
	block_count ++;
	return block->data;
}

static Array *new_array(int64_t length, ...) {
	Value items[length];
	cur_scope_frame = &(ScopeFrame){cur_scope_frame, items, length};
	va_list args;
	va_start(args, length);
	
	for(int64_t i=0; i < length; i++) {
		items[i] = va_arg(args, Value);
	}
	
	va_end(args);
	Array *array = mem_alloc(sizeof(Array) + length * sizeof(Value));
	array->length = length;
	
	for(int64_t i=0; i < length; i++) {
		array->items[i] = items[i];
	}
	
	cur_scope_frame = cur_scope_frame->parent;
	return array;
}

static Value call(Value value) {
	if(value.type == TYX_UNINITIALIZED) {
		error("function is not yet initialized");
	}
	else if(value.type != TY_FUNCTION) {
		error("callee is not callable");
	}
	
	return value.func();
}

static Value *subscript(Value array, Value index) {
	if(array.type != TY_ARRAY) {
		error("this is not an array");
	}
	
	if(index.type != TY_INT) {
		error("subscript index is not an integer");
	}
	
	if(index.value < 0 || index.value >= array.array->length) {
		error("array index out of range");
	}
	
	return array.array->items + index.value;
}
