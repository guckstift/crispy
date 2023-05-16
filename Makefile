CFILES = \
	main.c lex.c parse.c analyze.c generate.c print.c array.c build.c

HFILES = \
	lex.h parse.h analyze.h generate.h print.h ast.h array.h build.h

RUNTIMEFILES = \
	runtime.h runtime.c

CFLAGS = \
	-std=c17 -pedantic-errors

SRCS  = $(patsubst %.c,src/%.c,$(CFILES))
HDRS  = $(patsubst %.h,src/%.h,$(HFILES))
XXDIS = $(patsubst %,src/%.xxdi,$(RUNTIMEFILES))

crispy: $(SRCS) $(HDRS) $(XXDIS)
	gcc -o $@ $(CFLAGS) $(SRCS)

test: crispy test.cr
	./crispy test.cr

tests:
	make -C tests

src/%.xxdi: src/%
	xxd -i $^ > $@

.PHONY: test tests
