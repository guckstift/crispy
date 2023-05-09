#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lex.h"
#include "print.h"
#include "array.h"

typedef struct Ident {
	struct Ident *next;
	int64_t length;
	char id[];
} Ident;

Token *lex(char *src, char *src_end)
{
	static const char *keywords[] = {
		#define F(x) #x,
		KEYWORDS(F)
		#undef F
	};
	
	Token *tokens = 0;
	int64_t line = 1;
	char *linep = src;
	
	while(src <= src_end) {
		Token token = {.line = line, .linep = linep, .start = src};
		
		if(*src == '#') {
			while(src < src_end && *src != '\n') {
				src++;
			}
			
			continue;
		}
		else if(src[0] == '/' && src[1] == '*') {
			src += 2;
			
			while(src <= src_end) {
				if(*src == '\n') {
					line ++;
					linep = src + 1;
				}
				else if(src[0] == '*' && src[1] == '/') {
					src += 2;
					break;
				}
				else if(src == src_end) {
					error_at(
						&token, "mult-line comment not terminated with */"
					);
				}
				
				src++;
			}
			
			continue;
		}
		else if(isspace(*src)) {
			if(*src == '\n') {
				line ++;
				linep = src + 1;
			}
			
			src ++;
			continue;
		}
		else if(isalpha(*src) || *src == '_') {
			while(isalnum(*src) || *src == '_') {
				src++;
			}
			
			token.type = TK_IDENT;
			token.length = src - token.start;
			
			for(int i=0; i < KEYWORD_COUNT; i++) {
				if(
					strlen(keywords[i]) == token.length &&
					memcmp(keywords[i], token.start, token.length) == 0
				) {
					token.type = TK_KEYWORD;
					token.keyword = i;
				}
			}
		}
		else if(isdigit(*src)) {
			int64_t value = 0;
			
			if(src[0] == '0' && src[1] == 'x') {
				src += 2;
				
				while(isxdigit(*src)) {
					value *= 16;
					
					if(*src >= '0' && *src <= '9') {
						value += *src - '0';
					}
					else if(*src >= 'a' && *src <= 'f') {
						value += *src - 'a' + 10;
					}
					else if(*src >= 'A' && *src <= 'F') {
						value += *src - 'A' + 10;
					}
					
					src ++;
				}
			}
			else if(src[0] == '0' && src[1] == 'b') {
				src += 2;
				
				while(*src == '0' || *src == '1') {
					value *= 2;
					value += *src - '0';
					src ++;
				}
			}
			else {
				while(isdigit(*src)) {
					value *= 10;
					value += *src - '0';
					src ++;
				}
			}
			
			token.type = TK_INT;
			token.length = src - token.start;
			token.value = value;
		}
		else if(*src == '"') {
			src ++;
			
			while(*src != '"') {
				if(src == src_end) {
					error_at(&token, "string not terminated with \"");
				}
				else if(*src >= 0 && *src <= 0x1f) {
					error_at(
						&token, "no control characters allowed in string"
					);
				}
				else if(*src == '\\') {
					src ++;
					
					if(
						*src == '\\' || *src == '"' || *src == 'n' ||
						*src == 't'
					) {
						src ++;
					}
					else {
						error_at(&token, "unsupported escape sequence");
					}
				}
				else {
					src++;
				}
			}
			
			src ++;
			token.type = TK_STRING;
			token.length = src - token.start;
		}
		else if(
			src[0] == '=' && src[1] == '=' || src[0] == '!' && src[1] == '=' ||
			src[0] == '<' && src[1] == '=' || src[0] == '>' && src[1] == '='
		) {
			token.type = TK_PUNCT;
			token.length = 2;
			token.punct = IPUNCT(src);
			src += 2;
		}
		else if(
			*src == ';' || *src == '=' || *src == '(' || *src == ')' ||
			*src == '{' || *src == '}' || *src == '+' || *src == '-' ||
			*src == '[' || *src == ']' || *src == '*' || *src == '%' ||
			*src == '<' || *src == '>' || *src == ','
		) {
			token.type = TK_PUNCT;
			token.length = 1;
			token.punct = *src;
			src ++;
		}
		else if(src == src_end) {
			token.type = TK_EOF;
			token.length = 0;
			src++;
		}
		else {
			error_at(&token, "unknown token");
		}
		
		array_push(tokens, token);
	}
	
	Ident *ident_table = 0;
	
	array_for_ptr(tokens, Token, token) {
		if(token->type == TK_IDENT) {
			for(Ident *ident = ident_table; ident; ident = ident->next) {
				if(
					ident->length == token->length &&
					memcmp(ident->id, token->start, token->length) == 0
				) {
					token->id = ident->id;
					break;
				}
			}
			
			if(token->id == 0) {
				Ident *ident = calloc(1, sizeof(Ident) + token->length + 1);
				ident->next = ident_table;
				ident->length = token->length;
				ident->id[token->length] = 0;
				memcpy(ident->id, token->start, token->length);
				token->id = ident->id;
				ident_table = ident;
			}
		}
		else if(token->type == TK_STRING) {
			int64_t length = token->length - 2;
			token->text = malloc(length + 1);
			token->text[length] = 0;
			memcpy(token->text, token->start + 1, length);
		}
	}
	
	return tokens;
}
