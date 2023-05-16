#include <stdio.h>
#include <stdlib.h>
#include "runtime.h"

typedef struct PrintFrame {
	struct PrintFrame *parent;
	Value value;
} PrintFrame;

static void print_value(Value value);
static void value_decref(Value value);

ScopeFrame *cur_scope_frame = 0;

static PrintFrame *cur_print_frame = 0;
static MemBlock *first_block = 0;
static MemBlock *last_block = 0;
static int64_t block_count = 0;

static Value *error(int64_t cur_line, char *msg, ...)
{
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

Value *check_var(int64_t cur_line, Value *var, char *name)
{
	if(var->type == TYX_UNINITIALIZED) {
		error(cur_line, "name %s is not defined", name);
	}
	
	return var;
}

static void print_repr(Value value)
{
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

static void print_array(Array *array)
{
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

static void print_value(Value value)
{
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

void print(int64_t num, ...)
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

Value check_type(int64_t cur_line, Type mintype, Type maxtype, Value value)
{
	if(value.type < mintype || value.type > maxtype) {
		error(cur_line, "wrong type");
	}
	
	return value;
}

static void gc_mark(Value value)
{
	if(
		value.type == TY_ARRAY || value.type == TY_FUNCTION ||
		value.type == TYX_REFERENCE
	) {
		MemBlock *block = value.ptr;
		block --;
		
		if(block->mark == 1) {
			return;
		}
		
		block->mark = 1;
		
		DEBUG_printf("  gc marked block %p\n", (void*)block);
	}
	
	if(value.type == TY_ARRAY) {
		Array *array = value.array;
		
		for(int64_t i=0; i < array->length; i++) {
			gc_mark(array->items[i]);
		}
	}
	else if(value.type == TY_FUNCTION) {
		Function *func = value.func;
		
		for(int64_t i=0; i < func->enclosed_count; i++) {
			gc_mark(func->enclosed[i]);
		}
	}
	else if(value.type == TYX_REFERENCE) {
		Value *ref = value.ref;
		gc_mark(*ref);
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
			
			DEBUG_printf("freeing %p\n", (void*)block);
			
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

static void *mem_alloc(int64_t size)
{
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
	
	DEBUG_printf("alloced %p size %li\n", (void*)block, size);
	
	return block->data;
}

Array *new_array(int64_t length, ...)
{
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

Value uplift_var(Value *var)
{
	DEBUG_printf("new uplift\n");
	
	Value *lifted = mem_alloc(sizeof(Value));
	*lifted = *var;
	*var = REFERENCE(lifted);
	return *var;
}

Function *new_function(
	FuncPtr funcptr, int64_t arity, int64_t enclosed_count, ...
) {
	DEBUG_printf(
		"new function %li %li size %li\n",
		arity, enclosed_count,
		sizeof(Function) + enclosed_count * sizeof(Value)
	);
	
	Function *func = mem_alloc(
		sizeof(Function) + enclosed_count * sizeof(Value)
	);
	
	int64_t tmp_count = 1 + enclosed_count;
	Value tmp[tmp_count];
	
	for(int64_t i=0; i < tmp_count; i++) {
		tmp[i].type = TYX_UNINITIALIZED;
	}
	
	tmp[0] = FUNCTION_VALUE(func);
	PUSH_SCOPE(tmp, 0);
	
	func->func = funcptr;
	func->arity = arity;
	func->enclosed_count = enclosed_count;
	va_list args;
	va_start(args, enclosed_count);
	
	for(int64_t i=0; i < enclosed_count; i++) {
		Value *var = va_arg(args, Value*);
		
		if(var->type == TYX_REFERENCE) {
			func->enclosed[i] = *var;
			DEBUG_printf("already uplifted\n");
		}
		else {
			func->enclosed[i] = uplift_var(var);
		}
		
		tmp[1 + i] = func->enclosed[i];
	}
	
	va_end(args);
	POP_SCOPE();
	return func;
}

Value call(int64_t cur_line, Value value, int64_t argcount, ...)
{
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
	
	Function *func = value.func;
	va_list args;
	va_start(args, argcount);
	
	Value result = func->func(func->enclosed, args);
	va_end(args);
	return result;
}

Value *subscript(int64_t cur_line, Value array, Value index)
{
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

bool truthy(Value value)
{
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
