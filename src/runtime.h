#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>
#include <stdarg.h>

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

#endif
