#include "lexer.h"
#include "parser.h"
#include "parserDef.h"
#include "utils.h"
#include <time.h>

static const char *MENU_TEXT =
    "\nWhat would you like to do?\n"
    "  0) Exit\n"
    "  1) Remove Comments (print cleaned source)\n"
    "  2) Print Token Stream\n"
    "  3) Parse Source Code and Print Parse Tree\n"
    "  4) Parse Source Code and Report Time Taken\n"
    "==> ";

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <output_file>\n", argv[0]);
        return 1;
    }

    /* Build parser infrastructure once at startup */
    Grammar     G  = initializeGrammar();
    FirstFollow ff = computeFirstFollow(G);
    ParseTable  PT;
    buildParseTable(ff, &PT);

    int choice;
    for (;;) {
        printf("%s", MENU_TEXT);
        if (scanf("%d", &choice) != 1) break;

        switch (choice) {
        case 0:
            return 0;

        case 1: {
            printf("---- Cleaned Source (no comments) ----\n");
            removeComments(argv[1]);
            printf("--------------------------------------\n\n");
            break;
        }

        case 2: {
            FILE *srcFP = fopen(argv[1], "r");
            if (!srcFP) { perror(argv[1]); break; }
            printf("---- Token Stream ----\n");
            getStream(srcFP);
            fclose(srcFP);
            printf("----------------------\n\n");
            break;
        }

        case 3: {
            FILE *srcFP = fopen(argv[1], "r");
            FILE *outFP = fopen(argv[2], "w");
            if (!srcFP) { perror(argv[1]); break; }
            if (!outFP) { perror(argv[2]); fclose(srcFP); break; }

            printf("Parsing...\n");
            ParseTreeNode *root = parseSourceCode(PT, ff, G, srcFP);
            printParseTree(root, outFP);
            printf("Parse tree written to: %s\n\n", argv[2]);

            fclose(srcFP);
            fclose(outFP);
            break;
        }

        case 4: {
            FILE *srcFP = fopen(argv[1], "r");
            if (!srcFP) { perror(argv[1]); break; }

            printf("Parsing...\n");
            clock_t t_start = clock();
            ParseTreeNode *pt = parseSourceCode(PT, ff, G, srcFP);
            clock_t t_end   = clock();
            (void)pt;   /* result not printed in this mode */

            double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;
            printf("Parsing complete.\n");
            printf("Clock ticks : %ld\n",  (long)(t_end - t_start));
            printf("Time (sec)  : %.6f\n\n", elapsed);

            fclose(srcFP);
            break;
        }

        default:
            printf("Invalid choice. Please enter 0–4.\n");
            break;
        }
    }

    return 0;
}
