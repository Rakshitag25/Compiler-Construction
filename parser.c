#include "lexer.h"
#include "lexerDef.h"
#include "parserDef.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------------
 * buildParseTable
 *
 * Fill every cell of the LL(1) parse table.
 *   -1  → error (no rule applies)
 *   -2  → synchronisation point (token is in FOLLOW but not FIRST)
 *   ≥0  → rule index to expand
 * ------------------------------------------------------------------ */
void buildParseTable(FirstFollow ff, ParseTable *pt) {
    /* Default all entries to error */
    for (int row = 0; row < NON_TERMINAL_COUNT; row++)
        for (int col = 0; col < NUM_TOKENS; col++)
            pt->cell[row][col] = -1;

    for (int nt = 0; nt < NON_TERMINAL_COUNT; nt++) {
        /* Fill from FIRST set */
        for (int k = 0; k < ff.first_count[nt]; k++)
            pt->cell[nt][ff.first[nt][k]] = ff.rule_no[nt][k];

        /* Fill from FOLLOW set */
        if (ff.follow_rule[nt] != -1) {
            /* There is an ε-rule — use it wherever FOLLOW tokens appear */
            for (int k = 0; k < ff.follow_count[nt]; k++)
                pt->cell[nt][ff.follow[nt][k]] = ff.follow_rule[nt];
        } else {
            /* No ε-rule — mark FOLLOW positions as sync points if still empty */
            for (int k = 0; k < ff.follow_count[nt]; k++) {
                if (pt->cell[nt][ff.follow[nt][k]] == -1)
                    pt->cell[nt][ff.follow[nt][k]] = -2;
            }
        }
    }
}

/* ------------------------------------------------------------------
 * computeFirstFollow
 *
 * Compute FIRST and FOLLOW sets for all non-terminals.
 * FIRST is computed recursively; FOLLOW uses a dependency pass.
 * ------------------------------------------------------------------ */
FirstFollow computeFirstFollow(Grammar g) {
    FirstFollow ff;

    bool          firstDone[NON_TERMINAL_COUNT];
    NON_TERMINAL *depList[NON_TERMINAL_COUNT];
    int           depLen[NON_TERMINAL_COUNT];

    for (int i = 0; i < NON_TERMINAL_COUNT; i++) {
        ff.first_count[i]   = 0;
        ff.follow_count[i]  = 0;
        ff.first_has_eps[i] = false;
        ff.follow_rule[i]   = -1;
        firstDone[i]        = false;
        depLen[i]           = 0;
        depList[i] = (NON_TERMINAL *)calloc(MAX_RHS_LEN, sizeof(NON_TERMINAL));
    }

    /* Phase 1: FIRST sets */
    for (int i = 0; i < NON_TERMINAL_COUNT; i++)
        computeFirstRec(&ff, (NON_TERMINAL)i, g, firstDone);

    /* Seed FOLLOW for the start symbol with $ */
    ff.follow[NT_PROGRAM][0] = DOLLAR;
    ff.follow_count[NT_PROGRAM] = 1;

    /* Phase 2: FOLLOW sets — scan every rule body */
    for (int nt = 0; nt < NON_TERMINAL_COUNT; nt++)
        for (int r = 0; r < g.prod_count[nt]; r++)
            followHelper(&ff, g.prods[nt][r], (NON_TERMINAL)nt, depList, depLen);

    /* Phase 3: resolve inter-non-terminal FOLLOW dependencies */
    for (int i = 0; i < NON_TERMINAL_COUNT; i++)
        if (depLen[i] > 0)
            clearDependency(i, depList, depLen, &ff);

    for (int i = 0; i < NON_TERMINAL_COUNT; i++)
        free(depList[i]);

    return ff;
}

/* ------------------------------------------------------------------
 * makeSymNode  (internal helper)
 *
 * Allocate a fresh parse-tree node that corresponds to a particular
 * grammar symbol and attach it to the given parent.
 * ------------------------------------------------------------------ */
static ParseTreeNode *makeSymNode(GrammarSymbol sym, ParseTreeNode *par) {
    ParseTreeNode *nd  = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
    nd->data.sym       = sym;
    nd->data.line      = -1;
    nd->data.lexeme    = NULL;
    nd->data.lexemeSize = 0;
    nd->parent         = par;
    nd->child_count    = 0;
    return nd;
}

/* ------------------------------------------------------------------
 * parseSourceCode
 *
 * LL(1) table-driven parser.  Maintains a symbol stack and a parallel
 * tree-node stack so that the parse tree is built in one pass.
 * ------------------------------------------------------------------ */
ParseTreeNode *parseSourceCode(ParseTable pt, FirstFollow ff, Grammar g, FILE *src) {
    /* Parallel stacks: grammar symbols and their matching tree nodes */
    GrammarSymbol *symStack[200];
    ParseTreeNode *nodeStack[200];

    int symTop  = 0;
    int ndTop   = 0;
    bool hadError = false;
    int  lastErrLine = -1;

    /* ----- Initialise twin buffer ----- */
    twinBuffer tb = (twinBuffer)malloc(sizeof(TWIN_BUFFER));
    for (int i = 0; i < 2 * CHUNK_SIZE; i++)
        tb->buf[i] = '\0';
    tb->pos  = CHUNK_SIZE;   /* pretend we're in second half so first fills */
    tb->line = 1;
    populate_buffer(tb, src);
    tb->pos  = 0;
    populate_buffer(tb, src);

    initializeLookupTable();

    /* ----- Build the root node ($program) ----- */
    ParseTreeNode *root = (ParseTreeNode *)malloc(sizeof(ParseTreeNode));
    root->data.sym.isTerminal    = false;
    root->data.sym.sym.nt        = NT_PROGRAM;
    root->data.line              = -1;
    root->data.lexeme            = NULL;
    root->data.lexemeSize        = 0;
    root->parent                 = NULL;
    root->child_count            = 0;

    nodeStack[ndTop] = root;

    /* ----- Push $ then <program> onto the symbol stack ----- */
    GrammarSymbol *dollarSym = (GrammarSymbol *)malloc(sizeof(GrammarSymbol));
    dollarSym->isTerminal    = true;
    dollarSym->sym.t         = DOLLAR;

    GrammarSymbol *startSym  = (GrammarSymbol *)malloc(sizeof(GrammarSymbol));
    startSym->isTerminal     = false;
    startSym->sym.nt         = NT_PROGRAM;

    symStack[symTop] = dollarSym;
    symTop++;
    symStack[symTop] = startSym;

    /* ----- Fetch the first lookahead token ----- */
    tokenInfo lookahead = nextToken(tb, src);

    /* ----- Main parsing loop ----- */
    while (tb->buf[tb->pos] != '\0' && symTop >= 0) {
        GrammarSymbol *top = symStack[symTop];

        if (top->isTerminal) {
            /* ---- Terminal on top of stack ---- */
            if (top->sym.t == DOLLAR && lookahead->type == DOLLAR) {
                break;  /* successful parse */
            }

            if (top->sym.t == lookahead->type) {
                /* Match: populate the tree node with token info */
                ParseTreeNode *cur = nodeStack[ndTop];
                cur->data.sym.isTerminal = true;
                cur->data.sym.sym.t      = lookahead->type;
                cur->data.line           = lookahead->line;
                cur->data.lexeme         = lookahead->lexeme;
                cur->data.lexemeSize     = lookahead->lexemeSize;

                free(top);
                symStack[symTop] = NULL;
                symTop--;
                ndTop--;

                free(lookahead);
                lookahead = nextToken(tb, src);
            } else {
                /* Mismatch: skip this token and report once per line */
                hadError = true;
                if (lastErrLine == lookahead->line) {
                    free(lookahead);
                    lookahead = nextToken(tb, src);
                    continue;
                }
                lastErrLine = lookahead->line;
                printf("Line %02d: Syntax Error : Token %s (lexeme \"%s\") "
                       "does not match expected token %s\n",
                       lookahead->line,
                       getTokenName(lookahead->type),
                       lookahead->lexeme,
                       getTokenName(top->sym.t));

                free(top);
                symStack[symTop] = NULL;
                symTop--;
                ndTop--;
            }

        } else {
            /* ---- Non-terminal on top of stack ---- */
            NON_TERMINAL nt = top->sym.nt;
            int ruleIdx = pt.cell[nt][lookahead->type];

            if (ruleIdx == -1) {
                /* Error cell — discard the lookahead and keep going */
                hadError = true;
                if (lastErrLine == lookahead->line) {
                    free(lookahead);
                    lookahead = nextToken(tb, src);
                    continue;
                }
                lastErrLine = lookahead->line;
                printf("Line %02d: Syntax Error : Unexpected token %s "
                       "(lexeme \"%s\") while expanding %s\n",
                       lookahead->line,
                       getTokenName(lookahead->type),
                       lookahead->lexeme,
                       getNonTerminal(nt));

                free(lookahead);
                lookahead = nextToken(tb, src);

            } else if (ruleIdx == -2) {
                /* Sync cell — pop the non-terminal and try to resynchronise */
                hadError = true;
                if (lastErrLine == lookahead->line) {
                    free(lookahead);
                    lookahead = nextToken(tb, src);
                    continue;
                }
                lastErrLine = lookahead->line;
                printf("Line %02d: Syntax Error : Unexpected token %s "
                       "(lexeme \"%s\") while expanding %s — popping\n",
                       lookahead->line,
                       getTokenName(lookahead->type),
                       lookahead->lexeme,
                       getNonTerminal(nt));

                free(top);
                symStack[symTop] = NULL;
                symTop--;
                ndTop--;

            } else {
                /* Valid rule — expand the non-terminal */
                ProductionRule rule = g.prods[nt][ruleIdx];
                ParseTreeNode *cur  = nodeStack[ndTop];
                ndTop--;

                free(top);
                symStack[symTop] = NULL;
                symTop--;

                if (g.has_eps[nt] && ruleIdx == g.prod_count[nt]) {
                    /* ε-rule: add a single epsilon leaf */
                    cur->child_count  = 1;
                    ParseTreeNode *epsNode = makeSymNode(
                        (GrammarSymbol){ .isTerminal = true,
                                         .sym.t      = EPSILLON },
                        cur);
                    cur->children[0] = epsNode;
                } else {
                    /* Normal rule: create a child for each RHS symbol */
                    cur->child_count = rule.rhs_len;

                    /* Push in reverse so leftmost child is processed first */
                    for (int k = rule.rhs_len - 1; k >= 0; k--) {
                        GrammarSymbol rhsSym;
                        rhsSym.isTerminal = rule.rhs[k].isTerminal;
                        rhsSym.sym        = rule.rhs[k].sym;

                        ParseTreeNode *child = makeSymNode(rhsSym, cur);
                        cur->children[k] = child;

                        nodeStack[++ndTop] = child;

                        GrammarSymbol *pushed = (GrammarSymbol *)malloc(sizeof(GrammarSymbol));
                        pushed->isTerminal = rule.rhs[k].isTerminal;
                        pushed->sym        = rule.rhs[k].sym;
                        symStack[++symTop] = pushed;
                    }
                }
            }
        }
    } /* end main loop */

    /* ----- Post-parse checks ----- */
    if (!(symTop == 0 && symStack[symTop] != NULL &&
          symStack[symTop]->isTerminal &&
          symStack[symTop]->sym.t == DOLLAR)) {
        hadError = true;
        printf("Syntax Error : Input consumed but symbol stack is not empty\n");
    } else if (lookahead->type != DOLLAR) {
        hadError = true;
        printf("Syntax Error : Symbol stack empty but input not fully consumed\n");
    }

    free(tb);

    if (!hadError)
        printf("COMPILATION SUCCESS!\n");
    else
        printf("COMPILATION FAILED\n");

    /* Clean up remaining stack entries */
    while (symTop >= 0) {
        if (symStack[symTop] != NULL)
            free(symStack[symTop]);
        symTop--;
    }

    return root;
}

/* ------------------------------------------------------------------
 * printParseTree
 *
 * In-order traversal of the parse tree; prints each node as one
 * fixed-width row in the output file.
 * ------------------------------------------------------------------ */
void printParseTree(ParseTreeNode *root, FILE *out) {
    if (root == NULL)
        return;

    /* Print column headers exactly once */
    static int headerPrinted = 0;
    if (!headerPrinted) {
        headerPrinted = 1;
        fprintf(out,
                "%-30s%-30s%-30s%-30s%-30s%-30s%-30s\n\n",
                "lexeme", "lineno", "token",
                "valueIfNumber", "parentNodeSymbol",
                "isLeafNode(yes/no)", "NodeSymbol");
    }

    /* Visit first child before printing this node (in-order) */
    if (root->child_count > 0 && root->children[0] != NULL)
        printParseTree(root->children[0], out);

    if (out == NULL)
        return;

    /* --- Lexeme column --- */
    fprintf(out, "%-30s", (root->data.lexeme != NULL) ? root->data.lexeme : "----");

    /* --- Line number --- */
    fprintf(out, "%-30d", root->data.line);

    /* --- Token / non-terminal name --- */
    if (root->data.sym.isTerminal)
        fprintf(out, "%-30s", getTokenName(root->data.sym.sym.t));
    else
        fprintf(out, "%-30s", getNonTerminal(root->data.sym.sym.nt));

    /* --- Numeric value (only for TK_NUM / TK_RNUM leaves) --- */
    if (root->data.sym.isTerminal &&
        (root->data.sym.sym.t == TK_NUM || root->data.sym.sym.t == TK_RNUM))
        fprintf(out, "%-30s", root->data.lexeme);
    else
        fprintf(out, "%-30s", "----");

    /* --- Parent symbol, leaf flag, node symbol --- */
    if (root->parent != NULL) {
        fprintf(out, "%-30s", getNonTerminal(root->parent->data.sym.sym.nt));
        fprintf(out, "%-30s", (root->child_count == 0) ? "YES" : "NO");
        fprintf(out, "%-30s", getTokenName(root->data.sym.sym.t));
        fprintf(out, "\n");
    } else {
        fprintf(out, "%-30s%-30s%-30s\n", "----", "----", "----");
    }

    /* Visit remaining children */
    for (int c = 1; c < root->child_count; c++)
        if (root->children[c] != NULL)
            printParseTree(root->children[c], out);
}
