#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "analyze.h"
#include "print.h"

static void a_block(Block *block);
static void a_expr(Expr *expr);

static Scope *cur_scope = 0;
static Decl *cur_funcdecl = 0;

static void add_used_var_to_func(Decl *decl)
{
	DeclItem *vars = cur_funcdecl->used_vars;
	
	for(DeclItem *item = cur_funcdecl->used_vars; item; item = item->next) {
		if(decl == item->decl) {
			return;
		}
	}
	
	DeclItem *item = calloc(1, sizeof(DeclItem));
	item->decl = decl;
	item->next = cur_funcdecl->used_vars;
	cur_funcdecl->used_vars = item;
}

static void make_temporary(Expr *expr)
{
	if(expr->tmp_id == 0) {
		cur_scope->tmp_count ++;
		expr->has_tmps = true;
		expr->tmp_id = cur_scope->tmp_count;
	}
}

static void a_var(Expr *var)
{
	Token *ident = var->ident;
	var->decl = lookup(ident, cur_scope);
	
	if(!var->decl) {
		error_at(var->ident, "%T is not declared", var->ident);
	}
	
	if(
		var->decl->scope->parent &&
		cur_scope->hosting_func != var->decl->scope->hosting_func
	) {
		error_at(
			var->ident, "can not use local variable %T in enclosed function",
			var->ident
		);
	}
	else if(ident < var->decl->end) {
		if(var->decl->scope == cur_scope) {
			error_at(var->ident, "%T is declared later", var->ident);
		}
		else {
			add_used_var_to_func(var->decl);
		}
	}
}

static void a_callexpr(Expr *call)
{
	a_expr(call->callee);
	
	for(Expr *arg = call->args; arg; arg = arg->next) {
		a_expr(arg);
	}
	
	make_temporary(call);
}

static void a_array(Expr *array)
{
	for(Expr *item = array->items; item; item = item->next) {
		a_expr(item);
		array->has_tmps = array->has_tmps || item->has_tmps;
	}
	
	make_temporary(array);
}

static Expr *array_get(Expr *array, int64_t index)
{
	Expr *item = array->items;
	
	while(index) {
		item = item->next;
		index --;
	}
	
	return item;
}

static void a_subscript(Expr *subscript)
{
	Expr *array = subscript->array;
	Expr *index = subscript->index;
	
	a_expr(array);
	a_expr(index);
	subscript->has_tmps = array->has_tmps || index->has_tmps;
	
	if(
		array->type == EX_ARRAY && index->isconst && index->type == EX_INT &&
		index->value >= 0 && index->value < array->length
	) {
		*subscript = *array_get(array, index->value);
		subscript->next = 0;
	}
}

static void a_binop(Expr *binop)
{
	Expr *left = binop->left;
	Expr *right = binop->right;
	
	a_expr(left);
	a_expr(right);
	
	if(binop->isconst) {
		Token *op = binop->op;
		int64_t oplevel = binop->oplevel;
		
		binop->value =
			op->punct == '+' ? left->value + right->value :
			op->punct == '-' ? left->value - right->value :
			op->punct == '*' ? left->value * right->value :
			op->punct == '%' ? left->value % right->value :
			op->punct == '<' ? left->value < right->value :
			op->punct == '>' ? left->value > right->value :
			op->punct == IPUNCT("==") ? left->value == right->value :
			op->punct == IPUNCT("!=") ? left->value != right->value :
			op->punct == IPUNCT("<=") ? left->value <= right->value :
			op->punct == IPUNCT(">=") ? left->value >= right->value :
			0 /* should never happen */;
		
		binop->type = oplevel == OP_CMP ? EX_BOOL : EX_INT;
	}
	else {
		binop->has_tmps = left->has_tmps || right->has_tmps;
	}
}

static void a_unary(Expr *unary)
{
	a_expr(unary->subexpr);
	
	if(unary->isconst && unary->subexpr->type == EX_INT) {
		unary->type = EX_INT;
		
		unary->value =
			unary->op->punct == '+' ? +unary->subexpr->value :
			unary->op->punct == '-' ? -unary->subexpr->value :
			0 /* should never happen */;
	}
	else {
		unary->has_tmps = unary->subexpr->has_tmps;
	}
}

static void a_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_VAR:
			a_var(expr);
			break;
		case EX_BINOP:
			a_binop(expr);
			break;
		case EX_CALL:
			a_callexpr(expr);
			break;
		case EX_ARRAY:
			a_array(expr);
			break;
		case EX_SUBSCRIPT:
			a_subscript(expr);
			break;
		case EX_UNARY:
			a_unary(expr);
			break;
	}
}

static void a_vardecl(Decl *vardecl)
{
	if(vardecl->init) {
		a_expr(vardecl->init);
	}
}

static void a_assign(Stmt *assign)
{
	a_expr(assign->target);
	a_expr(assign->value);
	
	if(assign->target->islvalue == 0) {
		error_at(assign->start, "target is not assignable");
	}
}

static void a_print(Stmt *print)
{
	for(Expr *value = print->values; value; value = value->next) {
		a_expr(value);
	}
}

static void a_funcdecl(Decl *funcdecl)
{
	Decl *old_funcdecl = cur_funcdecl;
	cur_funcdecl = funcdecl;
	funcdecl->used_vars = calloc(1, sizeof(DeclItem));
	funcdecl->body->scope->hosting_func = funcdecl;
	a_block(funcdecl->body);
	cur_funcdecl = old_funcdecl;
}

static void a_call(Stmt *call)
{
	a_expr(call->call);
	call->call->tmp_id = 0;
}

static void a_return(Stmt *returnstmt)
{
	if(!cur_funcdecl) {
		error_at(
			returnstmt->start, "return can only be used inside a function"
		);
	}
	
	if(returnstmt->value) {
		a_expr(returnstmt->value);
	}
}

static void a_if(Stmt *ifstmt)
{
	a_expr(ifstmt->cond);
	a_block(ifstmt->body);
	
	if(ifstmt->else_body) {
		a_block(ifstmt->else_body);
	}
}

static void a_while(Stmt *stmt)
{
	a_expr(stmt->cond);
	a_block(stmt->body);
}

static void a_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			a_vardecl(stmt->decl);
			break;
		case ST_ASSIGN:
			a_assign(stmt);
			break;
		case ST_PRINT:
			a_print(stmt);
			break;
		case ST_FUNCDECL:
			a_funcdecl(stmt->decl);
			break;
		case ST_CALL:
			a_call(stmt);
			break;
		case ST_RETURN:
			a_return(stmt);
			break;
		case ST_IF:
			a_if(stmt);
			break;
		case ST_WHILE:
			a_while(stmt);
			break;
	}
}

static void a_block(Block *block)
{
	if(!block->scope->hosting_func) {
		block->scope->hosting_func = cur_scope ? cur_scope->hosting_func : 0;
	}
	
	cur_scope = block->scope;
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
	
	cur_scope = cur_scope->parent;
}

void analyze(Module *module)
{
	cur_scope = 0;
	a_block(module->body);
}
