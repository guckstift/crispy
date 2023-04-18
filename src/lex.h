#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#define KEYWORDS(f) \
	f(var) \
	f(print) \
	f(true) \
	f(false) \
	f(null) \
	f(function) \

typedef enum {
	TK_KEYWORD,
	TK_IDENT,
	TK_INT,
	TK_PUNCT,
	TK_EOF,
} TokenType;

typedef enum {
	#define F(x) KW_ ## x,
	KEYWORDS(F)
	#undef F
} Keyword;

typedef struct Token {
	TokenType type;
	int line;
	union {
		Keyword keyword;
		char *text;
		int64_t value;
		char punct;
	};
} Token;

extern char *keywords[];

Token *lex(char *src, char *src_end);

#endif
