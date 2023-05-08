#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lex.h"
#include "print.h"

char *keywords[] = {
	#define F(x) #x,
	KEYWORDS(F)
	#undef F
};

static int line = 1;
static char *linep = 0;

Tokens *lex(char *src, char *src_end)
{
	Token *tokens = 0;
	int num_tokens = 0;
	line = 1;
	linep = src;
	
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
			char *start = src;
			
			while(isalnum(*src) || *src == '_') {
				src++;
			}
			
			int length = src - start;
			char *text = calloc(length + 1, 1);
			memcpy(text, start, length);
			token.type = TK_IDENT;
			token.length = length;
			token.text = text;
			
			for(int i=0; i < sizeof(keywords) / sizeof(char*); i++) {
				if(strcmp(keywords[i], text) == 0) {
					token.type = TK_KEYWORD;
					token.keyword = i;
					break;
				}
			}
		}
		else if(src[0] == '0' && src[1] == 'x') {
			src += 2;
			int64_t value = 0;
			
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
			
			token.type = TK_INT;
			token.length = src - token.start;
			token.value = value;
		}
		else if(src[0] == '0' && src[1] == 'b') {
			src += 2;
			int64_t value = 0;
			
			while(*src == '0' || *src == '1') {
				value *= 2;
				value += *src - '0';
				src ++;
			}
			
			token.type = TK_INT;
			token.length = src - token.start;
			token.value = value;
		}
		else if(isdigit(*src)) {
			int64_t value = 0;
			
			while(isdigit(*src)) {
				value *= 10;
				value += *src - '0';
				src ++;
			}
			
			token.type = TK_INT;
			token.length = src - token.start;
			token.value = value;
		}
		else if(*src == '"') {
			src ++;
			char *start = src;
			
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
			
			int length = src - start;
			char *text = calloc(length + 1, 1);
			memcpy(text, start, length);
			src ++;
			token.type = TK_STRING;
			token.length = src - token.start;
			token.text = text;
		}
		else if(
			src[0] == '=' && src[1] == '=' || src[0] == '!' && src[1] == '=' ||
			src[0] == '<' && src[1] == '=' || src[0] == '>' && src[1] == '='
		) {
			token.type = TK_PUNCT;
			token.length = 2;
			token.punct = src[0] | src[1] << 8;
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
		
		num_tokens ++;
		tokens = realloc(tokens, sizeof(Token) * num_tokens);
		tokens[num_tokens - 1] = token;
	}
	
	Tokens *list = calloc(1, sizeof(Tokens));
	list->tokens = tokens;
	list->count = num_tokens;
	list->eof_line = line;
	
	return list;
}
