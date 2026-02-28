CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11

# Final executable
stage1exe: driver.o lexer.o string.o trie.o 
	$(CC) $(CFLAGS) -o $@ $^

# Object files
driver.o: driver.c lexer.h parser.h parserDef.h utils.h
	$(CC) $(CFLAGS) -c driver.c

lexer.o: lexer.c lexer.h lexerDef.h
	$(CC) $(CFLAGS) -c lexer.c

parser.o: parser.c parserDef.h lexer.h
	$(CC) $(CFLAGS) -c parser.c

trie.o: trie.c trie.h
	$(CC) $(CFLAGS) -c trie.c

string.o: string.c string.h
	$(CC) $(CFLAGS) -c string.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

# Build and run a lexer-only test binary
run_lexer: lexer.o trie.o string.o
	$(CC) $(CFLAGS) -o $@ $^
	./$@

# Build a parser-only test binary (no driver)
run_parser: lexer.o trie.o string.o parser.o utils.o
	$(CC) $(CFLAGS) -o $@ $^

run: run_parser
	./run_parser

clean:
	rm -f *.o stage1exe run_lexer run_parser
