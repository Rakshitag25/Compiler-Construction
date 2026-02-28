#ifndef LEXER_HEADER
#define LEXER_HEADER

#include "lexerDef.h"
#include <stdio.h>

/* Print all tokens from the source file to stdout */
void getStream(FILE *src);

/* Return the next meaningful token; called internally */
tokenInfo getNextToken(twinBuffer tb, FILE *src);

/* Strip comment lines and print the cleaned source */
void removeComments(char *filepath);

/* Return the next token suitable for the parser */
tokenInfo nextToken(twinBuffer tb, FILE *src);

/* Map a TOKEN_TYPE enum value to its string name */
char *getTokenName(TOKEN_TYPE kind);

/* Fill the inactive half of the twin buffer from the file */
void populate_buffer(twinBuffer tb, FILE *src);

/* Build the keyword trie used for identifier classification */
void initializeLookupTable(void);

/* Check identifier length constraints; free and return false if violated */
bool handle_valid_error(tokenInfo tok);

#endif /* LEXER_HEADER */
