#ifndef PARSER_H
#define PARSER_H

#include "parserDef.h"
#include <stdio.h>

/*
 * Populate the LL(1) parse table from precomputed First/Follow sets.
 * Entries are rule indices; -1 = error, -2 = synchronisation point.
 */
void buildParseTable(FirstFollow ff, ParseTable *pt);

/*
 * Compute FIRST and FOLLOW sets for every non-terminal in the grammar.
 */
FirstFollow computeFirstFollow(Grammar g);

/*
 * Run the LL(1) parser on the source file, using the provided table
 * and grammar. Returns the root of the resulting parse tree.
 */
ParseTreeNode *parseSourceCode(ParseTable pt, FirstFollow ff, Grammar g, FILE *src);

/*
 * Write a formatted parse tree listing to 'out'.
 * Columns: lexeme | line | token/NT | numValue | parent | isLeaf | symbol
 */
void printParseTree(ParseTreeNode *root, FILE *out);

#endif /* PARSER_H */
