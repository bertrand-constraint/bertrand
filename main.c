#include "def.h"

int verbose;
char *libdir;	/* where #included files are found */

static char* copyright = "copyright (c) 1988 Wm Leler";

/*********************************************************************
 *
 * Execution shell for Bertrand interpreter.
 * 
 * command line arguments:	names of bertrand programs to be executed 
 *
 *********************************************************************/
int
main(argc, argv)
int argc;
char *argv[];
{
void parse();			/* from parse.c */
extern FILE *infile;		/* from scanner.c */
extern char *infilename;	/* from scanner.c */
extern int lineno;		/* from scanner.c */
NODE *init();			/* from util.c */
extern int learn;		/* from match.c */
NODE *walk();			/* from match.c */
void expr_print();		/* from expr.c */
void name_space_print();	/* from names.c */
extern NAME_NODE *global_names;	/* from names.c */
void graphics_close();		/* from graphics.c */
extern int graphics;		/* from graphics.c */
void st_mem_free();		/* from util.c */
char *getenv();			/* UNIX system routine */
void exit(int);			/* UNIX system routine */

int argno = 1;			/* command line argument */
NODE *subject;			/* subject expression */

/* check for BERTRAND environment variable */
if (!(libdir = getenv("BERTRAND"))) libdir = LIBDIR;

do {
    subject = init();		/* init constant operators */
    if (argc==1) {
	infilename = "stdin";
	infile = stdin;
	parse();
	}
    else {
	infilename = argv[argno];
	infile = fopen(infilename, "r");
	if (NULL==infile) {
	    fprintf(stderr, "can't open program file %s\n", infilename);
	    exit(1);
	    }
	parse();	/* call parser */
	fclose(infile);
	}

    lineno = 0;	/* to supress error message line numbers */
    if (verbose) fprintf(stderr, "\n");

    do {	/* apply rules to subject expression */
	subject = walk(subject);
	} while (learn);

    if (verbose && global_names->child) {
	fprintf(stderr, "\nglobal name space is: ");
	name_space_print(global_names);
	}
    if (verbose) fprintf(stderr, "\nfinal expression is: ");
    expr_print(subject);
    fprintf(stderr, "\n");

    st_mem_free();	/* free stack memory */

    if (graphics) {
	fprintf(stderr, "Zap output window to continue...\n");
	graphics_close();
	}

    } while(++argno<argc);
}
