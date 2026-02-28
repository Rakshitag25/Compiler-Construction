#ifndef STRUTIL_H
#define STRUTIL_H

/*
 * Returns 1 if strings p and q are identical, 0 otherwise.
 * Does not rely on the standard library strcmp.
 */
int stringcmp(char *p, char *q);

#endif /* STRUTIL_H */
