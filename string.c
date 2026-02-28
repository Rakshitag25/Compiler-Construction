#include "string.h"

/*
 * stringcmp — compare two null-terminated strings character by character.
 * Returns 1 if they are equal, 0 if they differ in any position or length.
 */
int stringcmp(char *p, char *q) {
    int pos = 0;

    while (p[pos] != '\0' && q[pos] != '\0') {
        if (p[pos] != q[pos])
            return 0;
        pos++;
    }

    /* Both must have ended at the same point */
    return (p[pos] == '\0' && q[pos] == '\0') ? 1 : 0;
}
