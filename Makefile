CFILES = \
	main.c lex.c parse.c analyze.c generate.c

HFILES = \
	lex.h parse.h analyze.h generate.h

CFLAGS = \
	-std=c17 -pedantic-errors

SRCS = $(patsubst %.c,src/%.c,$(CFILES))
HDRS = $(patsubst %.h,src/%.h,$(HFILES))

crispy: $(SRCS) $(HDRS)
	gcc -o $@ $(CFLAGS) $(SRCS)
