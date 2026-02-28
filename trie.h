#ifndef TRIE_H
#define TRIE_H

#include "lexerDef.h"

/* Only lowercase letters [a-z] appear in keywords */
#define TRIE_ALPHA_SZ 26

/*
 * A single node in the keyword lookup trie.
 * 'kids' are indexed by (ch - 'a').
 * 'stored_type' holds the TOKEN_TYPE at a terminal node (defaults to
 * TK_FIELDID so that any unrecognised path also returns TK_FIELDID).
 */
typedef struct TrieNode {
    struct TrieNode *kids[TRIE_ALPHA_SZ];
    TOKEN_TYPE       stored_type;
} TrieNode;

typedef TrieNode *trie;

/* Allocate and initialise a fresh trie node */
trie createTrieNode(void);

/* Insert a keyword into the trie, associating it with tok_type */
void insert(trie root, char *keyword, TOKEN_TYPE tok_type);

/* Look up a string; returns its TOKEN_TYPE or TK_FIELDID if not found */
TOKEN_TYPE search(trie root, char *key);

#endif /* TRIE_H */
