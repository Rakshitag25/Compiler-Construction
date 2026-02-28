#include "lexer.h"
#include "string.h"
#include "trie.h"
#include <stdbool.h>
#include <stdlib.h>

/* Trie that stores all language keywords */
static trie kwTable;

/*
 * transition()
 * Core DFA function: given the current state and the character just read,
 * return what to do next (new state, whether a token is complete, etc.).
 */
static TRANS_RESULT transition(DFA_STATE cur, char ch) {
    switch (cur) {

    case START: {
        /* Single-character tokens — emit immediately */
        if      (ch == ';')  return (TRANS_RESULT){START, true,  TK_SEM,   0, 0};
        else if (ch == ',')  return (TRANS_RESULT){START, true,  TK_COMMA, 0, 0};
        else if (ch == '.')  return (TRANS_RESULT){START, true,  TK_DOT,   0, 0};
        else if (ch == '(')  return (TRANS_RESULT){START, true,  TK_OP,    0, 0};
        else if (ch == ')')  return (TRANS_RESULT){START, true,  TK_CL,    0, 0};
        else if (ch == '[')  return (TRANS_RESULT){START, true,  TK_SQL,   0, 0};
        else if (ch == ']')  return (TRANS_RESULT){START, true,  TK_SQR,   0, 0};
        else if (ch == '*')  return (TRANS_RESULT){START, true,  TK_MUL,   0, 0};
        else if (ch == '/')  return (TRANS_RESULT){START, true,  TK_DIV,   0, 0};
        else if (ch == '+')  return (TRANS_RESULT){START, true,  TK_PLUS,  0, 0};
        else if (ch == '-')  return (TRANS_RESULT){START, true,  TK_MINUS, 0, 0};
        else if (ch == '~')  return (TRANS_RESULT){START, true,  TK_NOT,   0, 0};
        else if (ch == ':')  return (TRANS_RESULT){START, true,  TK_COLON, 0, 0};

        /* Multi-character tokens — move to intermediate states */
        else if (ch == '@')  return (TRANS_RESULT){S13,   false, NULL_TOKEN, 0, 0};
        else if (ch == '!')  return (TRANS_RESULT){S16,   false, NULL_TOKEN, 0, 0};
        else if (ch == '&')  return (TRANS_RESULT){S18,   false, NULL_TOKEN, 0, 0};
        else if (ch == '=')  return (TRANS_RESULT){S21,   false, NULL_TOKEN, 0, 0};
        else if (ch == '%')  return (TRANS_RESULT){S23,   false, NULL_TOKEN, 0, 0};
        else if (ch == '<')  return (TRANS_RESULT){S26,   false, NULL_TOKEN, 0, 0};
        else if (ch == '>')  return (TRANS_RESULT){S33,   false, NULL_TOKEN, 0, 0};
        else if (ch == '_')  return (TRANS_RESULT){S37,   false, NULL_TOKEN, 0, 0};
        else if (ch == '#')  return (TRANS_RESULT){S41,   false, NULL_TOKEN, 0, 0};

        /* Numeric literals */
        else if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S44, false, NULL_TOKEN, 0, 0};

        /* Identifiers — b/c/d can start a TK_ID with digit suffix */
        else if (ch >= 'b' && ch <= 'd')
            return (TRANS_RESULT){S57, false, NULL_TOKEN, 0, 0};

        /* General letter — field identifier / keyword path */
        else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
            return (TRANS_RESULT){S55, false, NULL_TOKEN, 0, 0};

        /* Whitespace */
        else if (ch == ' ' || ch == '\t')
            return (TRANS_RESULT){START, true, BLANK,   0, 0};
        else if (ch == '\n')
            return (TRANS_RESULT){START, true, NEWLINE, 0, 0};
        else if (ch == '\0')
            return (TRANS_RESULT){START, true, BLANK,   0, 0};

        /* EOF marker */
        else if (ch == EOF)
            return (TRANS_RESULT){START, true, EXIT_TOKEN, 0, 0};

        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 0};
    }

    /* @@@ — TK_OR */
    case S13:
        if (ch == '@') return (TRANS_RESULT){S14,   false, NULL_TOKEN, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 1};

    case S14:
        if (ch == '@') return (TRANS_RESULT){START, true,  TK_OR, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 1};

    /* != — TK_NE */
    case S16:
        if (ch == '=') return (TRANS_RESULT){START, true,  TK_NE, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 2};

    /* &&& — TK_AND */
    case S18:
        if (ch == '&') return (TRANS_RESULT){S19,   false, NULL_TOKEN, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 3};

    case S19:
        if (ch == '&') return (TRANS_RESULT){START, true,  TK_AND, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 3};

    /* == — TK_EQ */
    case S21:
        if (ch == '=') return (TRANS_RESULT){START, true,  TK_EQ, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 4};

    /* % comment — consume until newline */
    case S23:
        if (ch != '\n' && ch != '\0')
            return (TRANS_RESULT){S23,  false, NULL_TOKEN,  0, 0};
        else
            return (TRANS_RESULT){START, true, TK_COMMENT, 0, 0};

    /* < series: <, <=, <--- */
    case S26:
        if      (ch == '-') return (TRANS_RESULT){S28,  false, NULL_TOKEN, 0, 0};
        else if (ch == '=') return (TRANS_RESULT){START, true, TK_LE, 0, 0};
        else                return (TRANS_RESULT){START, true, TK_LT, 1, 0};

    case S28:
        if (ch == '-') return (TRANS_RESULT){S29,  false, NULL_TOKEN, 0, 0};
        else           return (TRANS_RESULT){START, true,  TK_LT, 2, 0};

    case S29:
        if (ch == '-') return (TRANS_RESULT){START, true, TK_ASSIGNOP, 0, 0};
        else           return (TRANS_RESULT){INVALID,false, NULL_TOKEN, 0, 5};

    /* > series: >, >= */
    case S33:
        if (ch == '=') return (TRANS_RESULT){START, true, TK_GE, 0, 0};
        else           return (TRANS_RESULT){START, true, TK_GT, 1, 0};

    /* _<letters><digits> — TK_FUNID */
    case S37:
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
            return (TRANS_RESULT){S38, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 6};

    case S38:
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
            return (TRANS_RESULT){S38, false, NULL_TOKEN, 0, 0};
        else if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S40, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_FUNID, 1, 0};

    case S40:
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S40, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_FUNID, 1, 0};

    /* #<lowercase> — TK_RUID */
    case S41:
        if (ch >= 'a' && ch <= 'z')
            return (TRANS_RESULT){S42, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 7};

    case S42:
        if (ch >= 'a' && ch <= 'z')
            return (TRANS_RESULT){S42, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_RUID, 1, 0};

    /* Integer / real number states */
    case S44:
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S44, false, NULL_TOKEN, 0, 0};
        else if (ch == '.')
            return (TRANS_RESULT){S46, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_NUM, 1, 0};

    case S46:
        /* Need exactly two digits after decimal */
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S47, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_NUM, 2, 0};

    case S47:
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S48, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 8};

    case S48:
        /* Optional exponent part */
        if (ch == 'E')
            return (TRANS_RESULT){S50, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_RNUM, 1, 0};

    case S50:
        if (ch == '+' || ch == '-')
            return (TRANS_RESULT){S51, false, NULL_TOKEN, 0, 0};
        else if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S52, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 9};

    case S51:
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){S52, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 10};

    case S52:
        if (ch >= '0' && ch <= '9')
            return (TRANS_RESULT){START, true, TK_RNUM, 0, 0};
        else
            return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 11};

    /* Pure lowercase field identifier / keyword */
    case S55:
        if (ch >= 'a' && ch <= 'z')
            return (TRANS_RESULT){S55, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_FIELDID, 1, 0};

    /* b/c/d — may be TK_ID with digit suffix [2-7] */
    case S57:
        if (ch >= 'a' && ch <= 'z')
            return (TRANS_RESULT){S55, false, NULL_TOKEN, 0, 0};
        else if (ch >= '2' && ch <= '7')
            return (TRANS_RESULT){S58, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_FIELDID, 1, 0};

    case S58:
        if (ch >= '2' && ch <= '7')
            return (TRANS_RESULT){S59, false, NULL_TOKEN, 0, 0};
        else if (ch >= 'b' && ch <= 'd')
            return (TRANS_RESULT){S58, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_ID, 1, 0};

    case S59:
        if (ch >= '2' && ch <= '7')
            return (TRANS_RESULT){S59, false, NULL_TOKEN, 0, 0};
        else
            return (TRANS_RESULT){START, true, TK_ID, 1, 0};

    default:
        return (TRANS_RESULT){INVALID, false, NULL_TOKEN, 0, 0};
    }
}

/* ------------------------------------------------------------------
 * initializeLookupTable
 * Insert every language keyword into the trie so we can distinguish
 * keyword lexemes from plain field identifiers.
 * ------------------------------------------------------------------ */
void initializeLookupTable(void) {
    kwTable = createTrieNode();

    insert(kwTable, "as",         TK_AS);
    insert(kwTable, "call",       TK_CALL);
    insert(kwTable, "definetype", TK_DEFINETYPE);
    insert(kwTable, "else",       TK_ELSE);
    insert(kwTable, "end",        TK_END);
    insert(kwTable, "endunion",   TK_ENDUNION);
    insert(kwTable, "endif",      TK_ENDIF);
    insert(kwTable, "endrecord",  TK_ENDRECORD);
    insert(kwTable, "endwhile",   TK_ENDWHILE);
    insert(kwTable, "global",     TK_GLOBAL);
    insert(kwTable, "if",         TK_IF);
    insert(kwTable, "input",      TK_INPUT);
    insert(kwTable, "int",        TK_INT);
    insert(kwTable, "list",       TK_LIST);
    insert(kwTable, "output",     TK_OUTPUT);
    insert(kwTable, "parameter",  TK_PARAMETER);
    insert(kwTable, "parameters", TK_PARAMETERS);
    insert(kwTable, "read",       TK_READ);
    insert(kwTable, "real",       TK_REAL);
    insert(kwTable, "record",     TK_RECORD);
    insert(kwTable, "return",     TK_RETURN);
    insert(kwTable, "then",       TK_THEN);
    insert(kwTable, "type",       TK_TYPE);
    insert(kwTable, "union",      TK_UNION);
    insert(kwTable, "while",      TK_WHILE);
    insert(kwTable, "with",       TK_WITH);
    insert(kwTable, "write",      TK_WRITE);
}

/* ------------------------------------------------------------------
 * getTokenName — return a printable string for a TOKEN_TYPE value
 * ------------------------------------------------------------------ */
char *getTokenName(TOKEN_TYPE kind) {
    switch (kind) {
    case TK_ASSIGNOP:   return "TK_ASSIGNOP";
    case TK_COMMENT:    return "TK_COMMENT";
    case TK_FIELDID:    return "TK_FIELDID";
    case TK_ID:         return "TK_ID";
    case TK_NUM:        return "TK_NUM";
    case TK_RNUM:       return "TK_RNUM";
    case TK_FUNID:      return "TK_FUNID";
    case TK_RUID:       return "TK_RUID";
    case TK_WITH:       return "TK_WITH";
    case TK_PARAMETERS: return "TK_PARAMETERS";
    case TK_END:        return "TK_END";
    case TK_WHILE:      return "TK_WHILE";
    case TK_UNION:      return "TK_UNION";
    case TK_ENDUNION:   return "TK_ENDUNION";
    case TK_DEFINETYPE: return "TK_DEFINETYPE";
    case TK_AS:         return "TK_AS";
    case TK_TYPE:       return "TK_TYPE";
    case TK_MAIN:       return "TK_MAIN";
    case TK_GLOBAL:     return "TK_GLOBAL";
    case TK_PARAMETER:  return "TK_PARAMETER";
    case TK_LIST:       return "TK_LIST";
    case TK_SQL:        return "TK_SQL";
    case TK_SQR:        return "TK_SQR";
    case TK_INPUT:      return "TK_INPUT";
    case TK_OUTPUT:     return "TK_OUTPUT";
    case TK_INT:        return "TK_INT";
    case TK_REAL:       return "TK_REAL";
    case TK_COMMA:      return "TK_COMMA";
    case TK_SEM:        return "TK_SEM";
    case TK_COLON:      return "TK_COLON";
    case TK_DOT:        return "TK_DOT";
    case TK_ENDWHILE:   return "TK_ENDWHILE";
    case TK_OP:         return "TK_OP";
    case TK_CL:         return "TK_CL";
    case TK_IF:         return "TK_IF";
    case TK_THEN:       return "TK_THEN";
    case TK_ENDIF:      return "TK_ENDIF";
    case TK_READ:       return "TK_READ";
    case TK_WRITE:      return "TK_WRITE";
    case TK_RETURN:     return "TK_RETURN";
    case TK_PLUS:       return "TK_PLUS";
    case TK_MINUS:      return "TK_MINUS";
    case TK_MUL:        return "TK_MUL";
    case TK_DIV:        return "TK_DIV";
    case TK_CALL:       return "TK_CALL";
    case TK_RECORD:     return "TK_RECORD";
    case TK_ENDRECORD:  return "TK_ENDRECORD";
    case TK_ELSE:       return "TK_ELSE";
    case TK_AND:        return "TK_AND";
    case TK_OR:         return "TK_OR";
    case TK_NOT:        return "TK_NOT";
    case TK_LT:         return "TK_LT";
    case TK_LE:         return "TK_LE";
    case TK_EQ:         return "TK_EQ";
    case TK_GT:         return "TK_GT";
    case TK_GE:         return "TK_GE";
    case TK_NE:         return "TK_NE";
    case EPSILLON:      return "EPSILLON";
    default:            return "UNKNOWN";
    }
}

/* ------------------------------------------------------------------
 * populate_buffer
 * Read CHUNK_SIZE bytes from the file into whichever half of the twin
 * buffer is NOT currently being consumed (determined by tb->pos).
 * ------------------------------------------------------------------ */
void populate_buffer(twinBuffer tb, FILE *src) {
    int fill_start, fill_end;

    if (tb->pos >= CHUNK_SIZE) {
        /* read head is in second half → refill first half */
        fill_start = 0;
        fill_end   = CHUNK_SIZE;
    } else {
        /* read head is in first half → refill second half */
        fill_start = CHUNK_SIZE;
        fill_end   = 2 * CHUNK_SIZE;
    }

    for (int k = fill_start; k < fill_end; k++) {
        int c = fgetc(src);
        if (c == EOF) {
            /* pad the rest of this half with null bytes */
            for (int m = k; m < fill_end; m++)
                tb->buf[m] = '\0';
            return;
        }
        tb->buf[k] = (char)c;
    }
}

/* ------------------------------------------------------------------
 * skip_comment_in_buffer
 * Advance tb->pos past a '%' comment, refilling the buffer as needed.
 * ------------------------------------------------------------------ */
static void skip_comment_in_buffer(twinBuffer tb, FILE *src) {
    int prev = tb->pos;

    while (tb->buf[tb->pos] != '\n' && tb->buf[tb->pos] != '\0') {
        tb->pos = (tb->pos + 1) % (2 * CHUNK_SIZE);
        int cur = tb->pos;

        /* crossed a buffer boundary — refill the half we left */
        if ((prev < CHUNK_SIZE && cur >= CHUNK_SIZE) ||
            (prev >= CHUNK_SIZE && cur < CHUNK_SIZE)) {
            populate_buffer(tb, src);
        }
        prev = cur;
    }

    /* step past the '\n' itself */
    tb->pos = (tb->pos + 1) % (2 * CHUNK_SIZE);
    int cur = tb->pos;
    if ((prev < CHUNK_SIZE && cur >= CHUNK_SIZE) ||
        (prev >= CHUNK_SIZE && cur < CHUNK_SIZE)) {
        populate_buffer(tb, src);
    }
}

/* ------------------------------------------------------------------
 * report_invalid
 * Print a descriptive lexical error, then advance the buffer past the
 * offending characters.
 * ------------------------------------------------------------------ */
static void report_invalid(TRANS_RESULT res, twinBuffer tb, int head, int tail) {
    printf("Line %02d: Lexical Error: Error: ", tb->line);

    if (head == tail) {
        printf("Unknown symbol <%c>\n", tb->buf[head]);
        tb->pos = (tail + 1) % (2 * CHUNK_SIZE);
        return;
    }

    printf("Unknown pattern <");
    int idx = head;
    while (idx != tail) {
        printf("%c", tb->buf[idx]);
        idx = (idx + 1) % (2 * CHUNK_SIZE);
    }
    printf("> ");
    tb->pos = tail;

    switch (res.errCode) {
    case 1:  printf(": Expected @@@\n");                                break;
    case 2:  printf(": Expected !=\n");                                 break;
    case 3:  printf(": Expected &&&\n");                                break;
    case 4:  printf(": Expected ==\n");                                 break;
    case 5:  printf(": Expected <---\n");                               break;
    case 6:  printf(": Expected a letter [a-z]|[A-Z] after _\n");      break;
    case 7:  printf(": Expected a lowercase letter [a-z] after #\n");  break;
    case 8:  printf(": Expected two digits after decimal point\n");     break;
    case 9:  printf(": Expected a digit [0-9] or +|- after E\n");       break;
    case 10: printf(": Expected a digit [0-9] after sign/E\n");         break;
    case 11: printf(": Expected two digits in exponent\n");             break;
    default: printf("\n");                                              break;
    }
}

/* ------------------------------------------------------------------
 * handle_valid_error
 * Enforce maximum lexeme lengths for identifiers.
 * Returns false (and frees the token) if the limit is exceeded.
 * ------------------------------------------------------------------ */
bool handle_valid_error(tokenInfo tok) {
    if (tok->type == TK_ID && tok->lexemeSize > 20) {
        printf("Line %02d: Lexical Error: Variable identifier \"%s\" exceeds "
               "the maximum length of 20 characters\n",
               tok->line, tok->lexeme);
        free(tok->lexeme);
        free(tok);
        return false;
    }
    if (tok->type == TK_FUNID && tok->lexemeSize > 30) {
        printf("Line %02d: Lexical Error: Function identifier \"%s\" exceeds "
               "the maximum length of 30 characters\n",
               tok->line, tok->lexeme);
        free(tok->lexeme);
        free(tok);
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------
 * getNextToken
 * Run the DFA from the current buffer position.
 * Returns a heap-allocated tokenInfo, or NULL for whitespace/comment/error.
 * ------------------------------------------------------------------ */
tokenInfo getNextToken(twinBuffer tb, FILE *src) {
    /* Handle '%' comment lines directly */
    if (tb->buf[tb->pos] == '%') {
        skip_comment_in_buffer(tb, src);

        tokenInfo ct  = (tokenInfo)malloc(sizeof(TOKEN));
        ct->lexeme    = (char *)malloc(2);
        ct->lexeme[0] = '%';
        ct->lexeme[1] = '\0';
        ct->lexemeSize = 1;
        ct->line       = tb->line;
        ct->type       = TK_COMMENT;
        return ct;
    }

    int head = tb->pos;
    int tail = tb->pos;

    TRANS_RESULT res = transition(START, tb->buf[head]);

    /* Keep going until the DFA wants to emit or hits an error */
    while (!res.emitsToken && res.nextState != INVALID) {
        tail = (tail + 1) % (2 * CHUNK_SIZE);
        res  = transition(res.nextState, tb->buf[tail]);
    }

    if (res.nextState == INVALID) {
        report_invalid(res, tb, head, tail);
        return NULL;
    }

    /* Whitespace — just advance the position */
    if (res.tokType == BLANK) {
        tb->pos = (tail + 1) % (2 * CHUNK_SIZE);
        return NULL;
    }

    if (res.tokType == NEWLINE) {
        tb->pos = (tail + 1) % (2 * CHUNK_SIZE);
        tb->line++;
        return NULL;
    }

    /* Valid token — extract the lexeme from the buffer */
    int retract = res.retract;
    int lex_end = (tail - retract + 2 * CHUNK_SIZE) % (2 * CHUNK_SIZE);

    int lex_len;
    if (lex_end >= head)
        lex_len = lex_end - head + 1;
    else
        lex_len = 2 * CHUNK_SIZE - head + lex_end + 1;

    char *word = (char *)malloc(lex_len + 1);
    int   wi   = 0;
    int   rd   = head;
    while (rd != lex_end) {
        word[wi++] = tb->buf[rd];
        rd = (rd + 1) % (2 * CHUNK_SIZE);
    }
    word[wi]     = tb->buf[rd];
    word[wi + 1] = '\0';

    /* Advance buffer past the consumed lexeme */
    tb->pos = (lex_end + 1) % (2 * CHUNK_SIZE);

    tokenInfo tok  = (tokenInfo)malloc(sizeof(TOKEN));
    tok->lexeme    = word;
    tok->lexemeSize = lex_len;
    tok->line       = tb->line;

    /* Determine the precise token type */
    if (res.tokType == TK_FIELDID) {
        /* Look up in keyword trie; falls back to TK_FIELDID if not found */
        tok->type = search(kwTable, word);
    } else if (res.tokType == TK_FUNID) {
        tok->type = stringcmp(word, "_main") ? TK_MAIN : TK_FUNID;
    } else {
        tok->type = res.tokType;
    }

    return tok;
}

/* ------------------------------------------------------------------
 * nextToken
 * Wrapper around getNextToken that the parser calls directly.
 * Automatically refills the buffer at half-boundaries and skips
 * whitespace / comments, returning only meaningful tokens.
 * ------------------------------------------------------------------ */
tokenInfo nextToken(twinBuffer tb, FILE *src) {
    int before = tb->pos;
    tokenInfo tok = getNextToken(tb, src);
    bool keep = false;

    if (tok != NULL) {
        if (tok->type == NEWLINE || tok->type == TK_COMMENT)
            tb->line++;

        /* Comments: recurse or return DOLLAR at end of file */
        if (tok->type == TK_COMMENT) {
            if (tb->buf[tb->pos] != '\0')
                return nextToken(tb, src);

            tokenInfo eofTok = (tokenInfo)malloc(sizeof(TOKEN));
            eofTok->type = DOLLAR;
            return eofTok;
        }

        if (tok->type != NULL_TOKEN  &&
            tok->type != NEWLINE     &&
            tok->type != EXIT_TOKEN  &&
            tok->type != BLANK) {
            if (handle_valid_error(tok))
                keep = true;
        }
    }

    int after = tb->pos;
    /* Refill the idle half when the read head crosses a boundary */
    if ((before < CHUNK_SIZE && after >= CHUNK_SIZE) ||
        (before >= CHUNK_SIZE && after < CHUNK_SIZE)) {
        populate_buffer(tb, src);
    }

    if (keep)
        return tok;

    if (tb->buf[tb->pos] != '\0')
        return nextToken(tb, src);

    tokenInfo eofTok = (tokenInfo)malloc(sizeof(TOKEN));
    eofTok->type = DOLLAR;
    return eofTok;
}

/* ------------------------------------------------------------------
 * getStream
 * Tokenise the entire source file and print each token to stdout.
 * ------------------------------------------------------------------ */
void getStream(FILE *src) {
    twinBuffer tb = (twinBuffer)malloc(sizeof(TWIN_BUFFER));

    /* Clear both halves */
    for (int i = 0; i < 2 * CHUNK_SIZE; i++)
        tb->buf[i] = '\0';

    tb->line = 1;

    /* Bootstrap: fill second half first, then first half */
    tb->pos = CHUNK_SIZE;           /* pretend we're in second half */
    populate_buffer(tb, src);       /* fills first half */
    tb->pos = 0;                    /* now in first half */
    populate_buffer(tb, src);       /* fills second half */

    initializeLookupTable();

    while (tb->buf[tb->pos] != '\0') {
        int before = tb->pos;
        tokenInfo tok = getNextToken(tb, src);

        if (tok != NULL) {
            if (tok->type == NEWLINE || tok->type == TK_COMMENT)
                tb->line++;

            if (tok->type != NULL_TOKEN  &&
                tok->type != NEWLINE     &&
                tok->type != EXIT_TOKEN  &&
                tok->type != BLANK) {
                if (handle_valid_error(tok)) {
                    printf("Line no. %d  Lexeme %-20s  Token %s\n",
                           tok->line, tok->lexeme, getTokenName(tok->type));
                }
            }

            if (tok->type == TK_COMMENT)
                continue;
        }

        int after = tb->pos;
        if ((before < CHUNK_SIZE && after >= CHUNK_SIZE) ||
            (before >= CHUNK_SIZE && after < CHUNK_SIZE)) {
            populate_buffer(tb, src);
        }
    }

    free(tb);
}

/* ------------------------------------------------------------------
 * removeComments
 * Open the source file, strip comment lines (from '%' to newline),
 * and print the cleaned code to stdout.
 * ------------------------------------------------------------------ */
void removeComments(char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        printf("Error: Cannot open file \"%s\"\n", filepath);
        return;
    }

    int c = fgetc(fp);
    while (c != EOF) {
        if (c == '%') {
            /* Skip everything until end of line */
            while (c != '\n' && c != EOF)
                c = fgetc(fp);
            if (c == '\n')
                c = fgetc(fp);
            printf("\n");   /* preserve line count */
        } else {
            printf("%c", (char)c);
            c = fgetc(fp);
        }
    }

    fclose(fp);
}
