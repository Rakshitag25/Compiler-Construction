#ifndef PARSER_DEF_H
#define PARSER_DEF_H

#include "lexerDef.h"

/* Total number of non-terminals in the grammar */
#define NON_TERMINAL_COUNT 53

/* Maximum number of symbols on the right-hand side of any single rule */
#define MAX_RHS_LEN 15

/*
 * All non-terminals in the language grammar.
 * Order matters — indices are used directly in the parse table.
 */
typedef enum NON_TERMINAL {
    NT_PROGRAM,
    NT_MAINFUNCTION,
    NT_OTHERFUNCTIONS,
    NT_FUNCTION,
    NT_INPUT_PAR,
    NT_OUTPUT_PAR,
    NT_PARAMETER_LIST,
    NT_DATATYPE,
    NT_PRIMITIVEDATATYPE,
    NT_CONSTRUCTEDDATATYPE,
    NT_REMAINING_LIST,
    NT_STMTS,
    NT_TYPEDEFINITIONS,
    NT_ACTUALORREDEFINED,
    NT_TYPEDEFINITION,
    NT_FIELDDEFINITIONS,
    NT_FIELDDEFINITION,
    NT_FIELDTYPE,
    NT_MOREFIELDS,
    NT_DECLATRATIONS,
    NT_DECLATRATION,
    NT_GLOBAL_OR_NOT,
    NT_OTHERSTMTS,
    NT_STMT,
    NT_ASSIGNMENTSTMT,
    NT_SINGLEORRECID,
    NT_OPTION_SINGLE_CONSTRUCTED,
    NT_ONEEXPANSION,
    NT_MOREEXPANSIONS,
    NT_FUNCALLSTMT,
    NT_OUTPUTPARAMETERS,
    NT_INPUTPARAMETERS,
    NT_ITERATIVESTMT,
    NT_CONDITIONALSTMT,
    NT_ELSEPART,
    NT_IOSTMT,
    NT_ARITHMETICEXPRESSION,
    NT_EXPPRIME,
    NT_TERM,
    NT_TERMPRIME,
    NT_FACTOR,
    NT_HIGHPRECEDENCEOPERATORS,
    NT_LOWPRECEDENCEOPERATORS,
    NT_BOOLEANEXPRESSION,
    NT_VAR,
    NT_LOGICALOP,
    NT_RELATIONALOP,
    NT_RETURNSTMT,
    NT_OPTIONALRETURN,
    NT_IDLIST,
    NT_MORE_IDS,
    NT_DEFINETYPESTMT,
    NT_A,
} NON_TERMINAL;

/*
 * One symbol in a grammar rule — either a terminal token or a non-terminal.
 * The 'isTerminal' flag selects which union member is valid.
 */
typedef struct {
    bool isTerminal;
    union {
        TOKEN_TYPE  t;
        NON_TERMINAL nt;
    } sym;
} GrammarSymbol;

/* One production rule: a sequence of grammar symbols */
typedef struct {
    GrammarSymbol rhs[MAX_RHS_LEN];
    int           rhs_len;
} ProductionRule;

/* Full grammar: for each non-terminal, a list of its production rules */
typedef struct {
    ProductionRule prods[NON_TERMINAL_COUNT][MAX_RHS_LEN];
    int            prod_count[NON_TERMINAL_COUNT];
    bool           has_eps[NON_TERMINAL_COUNT];   /* true if ε is derivable */
} Grammar;

/*
 * Precomputed First and Follow sets.
 * first[][]   — terminal tokens in FIRST(NT)
 * follow[][]  — terminal tokens in FOLLOW(NT)
 * rule_no[][] — which production index put this terminal in FIRST
 * follow_rule — if ≥ 0, the ε-rule index that causes FOLLOW to apply
 */
typedef struct {
    TOKEN_TYPE first[NON_TERMINAL_COUNT][MAX_RHS_LEN];
    int        first_count[NON_TERMINAL_COUNT];
    bool       first_has_eps[NON_TERMINAL_COUNT];
    int        rule_no[NON_TERMINAL_COUNT][MAX_RHS_LEN];

    TOKEN_TYPE follow[NON_TERMINAL_COUNT][MAX_RHS_LEN];
    int        follow_count[NON_TERMINAL_COUNT];
    int        follow_rule[NON_TERMINAL_COUNT];  /* -1 = no ε rule */
} FirstFollow;

/* LL(1) parse table: row = non-terminal, column = terminal token */
typedef struct {
    int cell[NON_TERMINAL_COUNT][NUM_TOKENS];
} ParseTable;

/* Data attached to a single node in the parse tree */
typedef struct {
    GrammarSymbol sym;
    int           line;
    char         *lexeme;
    int           lexemeSize;
} TreeNodeData;

/* Forward declaration so children can point back to the struct */
typedef struct ParseTreeNode ParseTreeNode;

struct ParseTreeNode {
    TreeNodeData    data;
    ParseTreeNode  *parent;
    int             child_count;
    ParseTreeNode  *children[MAX_RHS_LEN];
};

#endif /* PARSER_DEF_H */
