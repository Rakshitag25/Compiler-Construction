#ifndef LEXER_DEF_HEADER
#define LEXER_DEF_HEADER

#include <stdbool.h>

/* Twin buffer chunk size — each half holds this many chars */
#define CHUNK_SIZE 50
#define NUM_STATES 64
#define NUM_TOKENS 63

/* All characters that the language alphabet recognises */
static const char lang_alphabet[] = {
    'a', 'b', 'c', 'd',  'e',  'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q',  'r',  's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D',  'E',  'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q',  'R',  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3',  '4',  '5', '6', '7', '8', '9', ' ', '\n','\t',
    '~', '(', '[', ']',  ')',  '<', '>', '!', '@', '#', '%', '&', '*',
    '_', '+', '-', '/',  '=',  ';', ':', '.', ','
};
static const int lang_alphabet_sz = sizeof(lang_alphabet) / sizeof(lang_alphabet[0]);

/* DFA states */
typedef enum DFA_STATE {
    START,
    S1,  S2,  S3,  S4,  S5,  S6,  S7,  S8,  S9,  S10,
    S11, S12, S13, S14, S15, S16, S17, S18, S19, S20,
    S21, S22, S23, S24, S25, S26, S27, S28, S29, S30,
    S31, S32, S33, S34, S35, S36, S37, S38, S39, S40,
    S41, S42, S43, S44, S45, S46, S47, S48, S49, S50,
    S51, S52, S53, S54, S55, S56, S57, S58, S59, S60,
    S61,
    INVALID,
    EXIT,
} DFA_STATE;

/* All token types the language supports */
typedef enum TOKEN_TYPE {
    TK_ASSIGNOP,
    TK_COMMENT,
    TK_FIELDID,
    TK_ID,
    TK_NUM,
    TK_RNUM,
    TK_FUNID,
    TK_RUID,
    TK_WITH,
    TK_PARAMETERS,
    TK_END,
    TK_WHILE,
    TK_UNION,
    TK_ENDUNION,
    TK_DEFINETYPE,
    TK_AS,
    TK_TYPE,
    TK_MAIN,
    TK_GLOBAL,
    TK_PARAMETER,
    TK_LIST,
    TK_SQL,
    TK_SQR,
    TK_INPUT,
    TK_OUTPUT,
    TK_INT,
    TK_REAL,
    TK_COMMA,
    TK_SEM,
    TK_COLON,
    TK_DOT,
    TK_ENDWHILE,
    TK_OP,
    TK_CL,
    TK_IF,
    TK_THEN,
    TK_ENDIF,
    TK_READ,
    TK_WRITE,
    TK_RETURN,
    TK_PLUS,
    TK_MINUS,
    TK_MUL,
    TK_DIV,
    TK_CALL,
    TK_RECORD,
    TK_ENDRECORD,
    TK_ELSE,
    TK_AND,
    TK_OR,
    TK_NOT,
    TK_LT,
    TK_LE,
    TK_EQ,
    TK_GT,
    TK_GE,
    TK_NE,
    NULL_TOKEN,
    NEWLINE,
    EXIT_TOKEN,
    BLANK,
    EPSILLON,   /* used during First/Follow computation */
    DOLLAR,     /* used during First/Follow computation */
} TOKEN_TYPE;

/* A single lexical token */
typedef struct TOKEN {
    TOKEN_TYPE  type;
    char       *lexeme;
    int         lexemeSize;
    int         line;
} TOKEN;

typedef TOKEN *tokenInfo;

/* What the transition function returns for each (state, char) pair */
typedef struct TRANS_RESULT {
    DFA_STATE  nextState;
    bool       emitsToken;
    TOKEN_TYPE tokType;
    int        retract;   /* how many chars to give back to buffer */
    int        errCode;   /* 0 = no error */
} TRANS_RESULT;

/* The two-half circular input buffer */
typedef struct TWIN_BUFFER {
    char buf[2 * CHUNK_SIZE];
    int  pos;   /* current read head */
    int  line;  /* current source line number */
} TWIN_BUFFER;

typedef TWIN_BUFFER *twinBuffer;

#endif /* LEXER_DEF_HEADER */
