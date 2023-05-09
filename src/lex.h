#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

#define IPUNCT(s) (s[0] | s[1] << 8)

#define KEYWORDS(f) \
	f(var) \
	f(print) \
	f(true) \
	f(false) \
	f(null) \
	f(function) \
	f(return) \
	f(if) \
	f(else) \
	f(while) \

typedef enum {
	TK_KEYWORD,
	TK_IDENT,
	TK_INT,
	TK_STRING,
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
	int64_t line;
	char *linep;
	char *start;
	int64_t length;
	union {
		Keyword keyword;
		char *id;
		char *text;
		int64_t value;
		int64_t punct;
	};
} Token;

extern char *keywords[];

Token *lex(char *src, char *src_end);

#endif
