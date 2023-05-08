#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#define NULL_VALUE ((Value){.type = TY_NULL})
#define NULL_VALUE_INIT {.type = TY_NULL}
#define BOOL_VALUE(v) ((Value){.type = TY_BOOL, .value = v})
#define BOOL_VALUE_INIT(v) {.type = TY_BOOL, .value = v}
#define INT_VALUE(v) ((Value){.type = TY_INT, .value = v})
#define INT_VALUE_INIT(v) {.type = TY_INT, .value = v}
#define STRING_VALUE(v) ((Value){.type = TY_STRING, .string = v})
#define STRING_VALUE_INIT(v) {.type = TY_STRING, .string = v}
#define ARRAY_VALUE(v) ((Value){.type = TY_ARRAY, .array = v})
#define NEW_ARRAY(...) ARRAY_VALUE(new_array(__VA_ARGS__))
#define FUNCTION_VALUE(v) ((Value){.type = TY_FUNCTION, .func = v})
#define NEW_FUNCTION(...) FUNCTION_VALUE(new_function(__VA_ARGS__))

#define UNINITIALIZED {.type = TYX_UNINITIALIZED}

#define BINOP(l, t, left, op, right) \
	(Value){ \
		.type = t, \
		.value = check_type( \
			l, TY_NULL, TY_INT, (left) \
		).value op check_type( \
			l, TY_NULL, TY_INT, (right) \
		).value \
	} \

#define INT_UNARY(l, op, right) \
	(Value){ \
		.type = TY_INT, \
		.value = op check_type( \
			l, TY_NULL, TY_INT, (right) \
		).value \
	} \

#define PUSH_SCOPE(scope, fn) \
	cur_scope_frame = &(ScopeFrame){ \
		.parent = cur_scope_frame, \
		.funcframe = cur_scope_frame ? cur_scope_frame->funcframe : 0, \
		.values = (Value*)&scope, \
		.length = sizeof(scope) / sizeof(Value), \
		.funcname = fn, \
	}; \

#define PUSH_EMPTY_SCOPE(fn) \
	cur_scope_frame = &(ScopeFrame){ \
		.parent = cur_scope_frame, \
		.funcframe = cur_scope_frame ? cur_scope_frame->funcframe : 0, \
		.values = 0, \
		.length = 0, \
		.funcname = fn, \
	}; \

#define POP_SCOPE() \
	cur_scope_frame = cur_scope_frame->parent \

#define RETURN_SCOPE() \
	cur_scope_frame = cur_scope_frame->funcframe->parent \

#define RETURN(expr) { \
	RETURN_SCOPE(); \
	return expr; \
} \

#define TRACK_TEMP(tmp_expr) \
	((cur_scope_frame->temps = &(Temp){ \
		.prev = cur_scope_frame->temps, \
		.value = tmp_expr, \
	})->value) \

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
		struct Function *func;
		void *ptr;
	};
} Value;

typedef struct Array {
	int64_t length;
	Value items[];
} Array;

typedef Value (*FuncPtr)(va_list args);

typedef struct Function {
	FuncPtr func;
	int64_t arity;
} Function;

typedef struct PrintFrame {
	struct PrintFrame *parent;
	Value value;
} PrintFrame;

typedef struct Temp {
	struct Temp *prev;
	Value value;
} Temp;

typedef struct ScopeFrame {
	struct ScopeFrame *parent;
	struct ScopeFrame *funcframe;
	Value *values;
	int64_t length;
	char *funcname;
	Temp *temps;
} ScopeFrame;

typedef struct MemBlock {
	struct MemBlock *next;
	int64_t mark;
	char data[];
} MemBlock;

static void print_value(Value value);
static void value_decref(Value value);

static PrintFrame *cur_print_frame = 0;
static ScopeFrame *cur_scope_frame = 0;
static MemBlock *first_block = 0;
static MemBlock *last_block = 0;
static int64_t block_count = 0;

static Value *error(int64_t cur_line, char *msg, ...) {
	va_list args;
	va_start(args, msg);
	fprintf(stderr, "error at line %li: ", cur_line);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	
	for(ScopeFrame *frame = cur_scope_frame; frame; frame = frame->parent) {
		if(frame->funcname) {
			fprintf(stderr, "\t""in %s\n", frame->funcname);
		}
	}
	
	exit(EXIT_FAILURE);
	return &NULL_VALUE;
}

static Value *check_var(int64_t cur_line, Value *var, char *name) {
	if(var->type == TYX_UNINITIALIZED) {
		error(cur_line, "name %s is not defined", name);
	}
	
	return var;
}

static void print_repr(Value value) {
	if(value.type == TY_STRING) {
		printf("\"");
		
		for(char *c = value.string; *c; c++) {
			if(*c >= 0 && *c <= 0x1f || *c == '"') {
				if(*c == '\\') {
					printf("\\\\");
				}
				else if(*c == '"') {
					printf("\\\"");
				}
				else if(*c == '\n') {
					printf("\\n");
				}
				else if(*c == '\t') {
					printf("\\t");
				}
			}
			else {
				printf("%c", *c);
			}
		}
		
		printf("\"");
	}
	else {
		print_value(value);
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

static void print_value(Value value) {
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

static void print(int64_t num, ...)
{
	va_list args;
	va_start(args, num);
	
	for(int64_t i=0; i < num; i++) {
		if(i > 0) {
			printf(" ");
		}
		
		print_value(va_arg(args, Value));
	}
	
	va_end(args);
	printf("\n");
}

static Value check_type(
	int64_t cur_line, Type mintype, Type maxtype, Value value
) {
	if(value.type < mintype || value.type > maxtype) {
		error(cur_line, "wrong type");
	}
	
	return value;
}

static void gc_mark(Value value) {
	if(value.type == TY_ARRAY) {
		MemBlock *block = (MemBlock*)value.ptr;
		block --;
		
		if(block->mark == 1) {
			return;
		}
		
		block->mark = 1;
		Array *array = value.array;
		
		for(int64_t i=0; i < array->length; i++) {
			gc_mark(array->items[i]);
		}
	}
	else if(value.type == TY_FUNCTION) {
		MemBlock *block = (MemBlock*)value.ptr;
		block --;
		block->mark = 1;
	}
}

static void collect_garbage() {
	for(ScopeFrame *frame = cur_scope_frame; frame; frame = frame->parent) {
		for(Temp *temp = frame->temps; temp; temp = temp->prev) {
			gc_mark(temp->value);
		}
		
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

static void unwind_temps()
{
	cur_scope_frame->temps = 0;
}

static Array *new_array(int64_t length, ...) {
	Value items[length];
	PUSH_SCOPE(items, 0);
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
	
	POP_SCOPE();
	return array;
}

static Function *new_function(FuncPtr funcptr, int64_t arity) {
	Function *func = mem_alloc(sizeof(Function));
	func->func = funcptr;
	func->arity = arity;
	return func;
}

static Value call(int64_t cur_line, Value value, int64_t argcount, ...) {
	if(value.type == TYX_UNINITIALIZED) {
		error(cur_line, "function is not yet initialized");
	}
	else if(value.type != TY_FUNCTION) {
		error(cur_line, "callee is not callable");
	}
	else if(value.func->arity != argcount) {
		error(
			cur_line,
			"callee needs %li arguments but got %li",
			value.func->arity, argcount
		);
	}
	
	va_list args;
	va_start(args, argcount);
	Value result = value.func->func(args);
	va_end(args);
	return result;
}

static Value *subscript(int64_t cur_line, Value array, Value index) {
	if(array.type != TY_ARRAY) {
		error(cur_line, "this is not an array");
	}
	
	if(index.type != TY_INT) {
		error(cur_line, "subscript index is not an integer");
	}
	
	if(index.value < 0 || index.value >= array.array->length) {
		error(cur_line, "array index out of range");
	}
	
	return array.array->items + index.value;
}

static bool truthy(Value value) {
	if(value.type == TY_STRING) {
		return value.string[0] != 0;
	}
	else if(value.type == TY_ARRAY) {
		return value.array->length != 0;
	}
	else if(value.type == TY_FUNCTION) {
		return true;
	}
	
	return value.value;
}
