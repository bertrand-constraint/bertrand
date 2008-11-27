#include <stdlib.h>
#include "def.h"

/*************************************************************
 *
 *  Define a primitive operator or type.
 *  Insert in operator tables (if not null).
 *  Return pointer to operator node.
 *
 *************************************************************/

OP *
primitive(name, arity, super, oplist, eval)
char *name;             /* name of primitive */
short arity;            /* kind of operator */
OP *super;              /* supertype */
OP **oplist;            /* list to put it in */
short eval;             /* special eval function */
{
void op_put();          /* from ops.c */
OP *op_new();           /* from ops.c */

OP *top = op_new(strlen(name));
strcpy(top->pname, name);
top->arity = arity;
top->super = super;
top->eval = eval;
if (oplist) op_put(oplist, top);
return top;
}

/***********************************************************************
 *
 * Initilize variables.
 *
 *    Create operators which describe names.
 *    Return initial subject expression.
 *
 ***********************************************************************/
NODE *init()
{
    /* op_new takes integer argument, which is length of name */
    OP *op_new();			/* from ops.c */
    NODE *node_new();			/* from expr.c */
    extern OP *name_op;			/* from ops.c */
    extern OP *undeclared_prim;		/* from primitive.c */
    void primitive_init();		/* from primitive.c */
    void op_mem_free();			/* from ops.c */
    extern NAME_NODE *global_names;	/* from names.c */
    extern int lineno, charno;		/* from scan.c */
    extern int verbose;			/* from main.c */

    register TERM_NODE *insex;	/* initial subject expression */
    OP *main_op;		/* operator for initial subject expression */
    static char noname[] = "";		/* static so it won't go away */

    op_mem_free();		/* make sure operator memory is empty */
    primitive_init();		/* initialize all machine primitives */

    lineno = 1;
    charno = 0;
    verbose = FALSE;

    /* initial subject expression operator */
    main_op = primitive("main", NULLARY, (OP *) NULL, &name_op, 0);

    /* initialize global name space */
    global_names = (NAME_NODE *) node_new();
    global_names->op = undeclared_prim;
    global_names->next = (NAME_NODE *) NULL;
    global_names->parent = (NAME_NODE *) NULL;
    global_names->child = (NAME_NODE *) NULL;
    global_names->pval = noname;	/* no name. */
    global_names->refs = 1;	/* will never delete */
    global_names->interest = 0;

    /* create and return initial subject expression */
    insex = (TERM_NODE *) node_new();
    insex->op = main_op;
    insex->label = global_names;
    global_names->refs++;	/* since we have a pointer to it */
    insex->left = (NODE *) NULL;
    insex->right = (NODE *) NULL;
    return (NODE *) insex;
}

/***********************************************************************
 *
 * Routines to manage nodes for stacks.
 * Used for the parse and match stacks.
 *
 ***********************************************************************/

#define NUM_ST	   50		/* number of parse stack nodes to allocate */
static SNODE *st_mem = NULL;		/* free parse stack nodes */
static SNODE *all_st_mem = NULL;	/* pointer to all stack memory */
 
/***********************************************************************
 *
 * Get a stack node.  Allocate memory for it if necessary.
 *
 * exit:	pointer to a fresh stack node
 *
 ***********************************************************************/
SNODE *
st_get()
{
#ifndef __STDC__
char *malloc();
#endif
SNODE *temp;		/* node to be allocated to stack */ 

if (!st_mem) {
    register int i;
#   ifdef DEBUG
    printf("allocating stack nodes\n");
#   endif
    st_mem = (SNODE *) malloc((sizeof (SNODE *)) + (NUM_ST * sizeof (SNODE)));
    if (!st_mem) error("out of memory");

    /* Save pointer to the beginning of this chunk of memory and link
       it to other allocated chunks (to be freed at end of program). */
    *((SNODE **) st_mem) = all_st_mem;;
    all_st_mem = st_mem; 
    /* real memory starts after first word */
    st_mem = (SNODE *) (((char *)st_mem) + sizeof(SNODE *));

    for (i = 0; i < NUM_ST-1 ; i++) 	/* link list */
	st_mem[i].next = &st_mem[i+1];
    st_mem[NUM_ST-1].next = NULL;
    }

temp = st_mem;
st_mem = st_mem->next;
return temp;
}

/***********************************************************************
 *
 * Free a stack node.
 *
 * exit:	free list counter incremented
 *		node pushed back on free list
 *		return new top of free list
 *
 ***********************************************************************/
void
st_free(p)
SNODE *p;
{
p->next = st_mem;
st_mem = p;
}


/***********************************************************************
 *
 * Free all stack nodes.
 *
 ***********************************************************************/
void
st_mem_free()
{
register SNODE *t;
void free();

while(all_st_mem) {
    t = *((SNODE **) all_st_mem);
    free(all_st_mem);
    all_st_mem = t;
    }
st_mem = (SNODE *) NULL;
}

/*********************************************************************
 *
 * Routines to manage character string memory.
 * For the time being, these are very simple.
 * TO DO: make these more efficient.
 *
 *********************************************************************/

/*********************************************************************
 *
 * Allocate a new character string and copy the contents of
 * the argument string into it.
 *
 * returns:	pointer to new string
 *
 *********************************************************************/
char *
char_copy(s)
char *s;
{
char *t;	/* new character string */
int ss;		/* size of argument string s */
#ifndef __STDC__
char *calloc();
#endif

ss = strlen(s) + 1;	/* one extra for null terminator */
t = (char *) calloc(ss, sizeof(char));
if (!t) error("out of character string memory");
strcpy(t, s);
return(t);
}



/***********************************************************************
 *
 * Free a character string.
 * String must have been allocated by char_copy (or calloc).
 *
 ***********************************************************************/
void
char_free(s)
char *s;
{
    void free();

    free(s);
}

/***********************************************************************
 *
 * Error routine
 *
 * exit:	abort program
 *
 ***********************************************************************/
void
error(s)
char *s;
{
extern int lineno, charno;	/* from scanner.c */
extern char *infilename;
fflush(stdout);
if (lineno) {
    if (charno == 0) fprintf(stderr, "file %s, line %d: ", infilename, lineno-1);
    else fprintf(stderr, "file %s, line %d, before position %d: ",
	infilename, lineno, charno);
    }
fprintf(stderr, "%s\n", s);
fflush(stderr);
exit(1);
}

/* flush stderr output buffer.  Used only during debugging */
void fl() { fprintf(stderr, "\n"); fflush(stderr); }
