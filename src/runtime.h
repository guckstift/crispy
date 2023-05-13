#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#define NULL_VALUE_INIT       {.type = TY_NULL}
#define NULL_VALUE            ((Value)NULL_VALUE_INIT)
#define BOOL_VALUE_INIT(v)    {.type = TY_BOOL, .value = v}
#define BOOL_VALUE(v)         ((Value)BOOL_VALUE_INIT(v))
#define INT_VALUE_INIT(v)     {.type = TY_INT, .value = v}
#define INT_VALUE(v)          ((Value)INT_VALUE_INIT(v))
#define STRING_VALUE_INIT(v)  {.type = TY_STRING, .string = v}
#define STRING_VALUE(v)       ((Value)STRING_VALUE_INIT(v))

#define ARRAY_VALUE(v)     ((Value){.type = TY_ARRAY, .array = v})
#define NEW_ARRAY(...)     ARRAY_VALUE(new_array(__VA_ARGS__))
#define FUNCTION_VALUE(v)  ((Value){.type = TY_FUNCTION, .func = v})
#define NEW_FUNCTION(...)  FUNCTION_VALUE(new_function(__VA_ARGS__))

#define UNINITIALIZED  {.type = TYX_UNINITIALIZED}

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

#define PUSH_SCOPE(scope, func_name) \
	cur_scope_frame = &(ScopeFrame){ \
		.parent = cur_scope_frame, \
		.funcframe = cur_scope_frame ? cur_scope_frame->funcframe : 0, \
		.values = (Value*)&scope, \
		.length = sizeof(scope) / sizeof(Value), \
		.funcname = func_name, \
	}; \

#define PUSH_EMPTY_SCOPE(func_name) \
	cur_scope_frame = &(ScopeFrame){ \
		.parent = cur_scope_frame, \
		.funcframe = cur_scope_frame ? cur_scope_frame->funcframe : 0, \
		.values = 0, \
		.length = 0, \
		.funcname = func_name, \
	}; \

#define POP_SCOPE() \
	cur_scope_frame = cur_scope_frame->parent \

#define RETURN_SCOPE() \
	cur_scope_frame = cur_scope_frame->funcframe->parent \

#define RETURN(expr) { \
	RETURN_SCOPE(); \
	return expr; \
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
} ScopeFrame;

typedef struct MemBlock {
	struct MemBlock *next;
	int64_t mark;
	char data[];
} MemBlock;

Value *check_var(int64_t cur_line, Value *var, char *name);
void print(int64_t num, ...);
Value check_type(int64_t cur_line, Type mintype, Type maxtype, Value value);
Array *new_array(int64_t length, ...);
Function *new_function(FuncPtr funcptr, int64_t arity);
Value call(int64_t cur_line, Value value, int64_t argcount, ...);
Value *subscript(int64_t cur_line, Value array, Value index);
bool truthy(Value value);

extern ScopeFrame *cur_scope_frame;

#endif
