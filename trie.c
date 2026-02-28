#include "trie.h"
#include <stdlib.h>

/*
 * createTrieNode — allocate a trie node with no children.
 * The default token type is TK_FIELDID so that any key that does not
 * match a keyword still returns a sensible value.
 */
trie createTrieNode(void) {
    trie nd = (trie)malloc(sizeof(TrieNode));

    for (int ch = 0; ch < TRIE_ALPHA_SZ; ch++)
        nd->kids[ch] = NULL;

    nd->stored_type = TK_FIELDID;
    return nd;
}

/*
 * insert — walk the trie, creating nodes as needed, then mark the
 * terminal node with the given token type.
 */
void insert(trie root, char *keyword, TOKEN_TYPE tok_type) {
    trie cur = root;

    for (int pos = 0; keyword[pos] != '\0'; pos++) {
        int idx = keyword[pos] - 'a';

        if (cur->kids[idx] == NULL)
            cur->kids[idx] = createTrieNode();

        cur = cur->kids[idx];
    }

    cur->stored_type = tok_type;
}

/*
 * search — traverse the trie following each character of key.
 * Returns TK_FIELDID as soon as a missing edge is encountered
 * (i.e. the key is not a keyword).
 */
TOKEN_TYPE search(trie root, char *key) {
    trie cur = root;

    for (int pos = 0; key[pos] != '\0'; pos++) {
        int idx = key[pos] - 'a';

        if (cur->kids[idx] == NULL)
            return TK_FIELDID;

        cur = cur->kids[idx];
    }

    return cur->stored_type;
}
