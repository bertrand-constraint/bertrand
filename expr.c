/***********************************************************************
 *
 * Routines to manage expressions
 *
 ***********************************************************************/

#include "def.h"
#include <ctype.h>

#define NODE_ALLOC 100		/* number of expr. tree nodes to allocate */
NODE *expr_mem = NULL;		/* next free expression tree node */ 

/***********************************************************************
 *
 * Allocate memory for expression tree nodes.  Such a node can
 * be used as a TERM_NODE, NAME_NODE, NUM_NODE, or STR_NODE.
 * A NODE is a generic type, and is used only for storage management.
 *
 * exit:	return pointer to a new expression-tree node
 *
 ***********************************************************************/
NODE *
node_new()
{
void *calloc();
NODE *temp;
register int i;

if (!expr_mem) {
#   ifdef DEBUG
    printf("allocating expression nodes\n");
    fflush(stdout);
#   endif
    expr_mem = (NODE *) calloc(NODE_ALLOC, sizeof (NODE));
    if (!expr_mem) error("out of memory");
    for (i = 0; i < NODE_ALLOC - 1; i++)
	expr_mem[i].next = &expr_mem[i+1];
    expr_mem[NODE_ALLOC-1].next = NULL;
    }

temp = expr_mem;
expr_mem = expr_mem->next;
return temp ;
}

/***********************************************************************
 *
 * Storage reclamation
 *
 ***********************************************************************/
void
node_free(n)
NODE *n;	/* expression node to be freed */
{
/* Put a node back on free list */
n->next = expr_mem;
expr_mem = n;
}

void expr_free(fn)
NODE *fn;
{
char *arity_name();		/* from ops.c */
void name_free();		/* from names.c */

if ((fn->op->arity == OP_STR) || (fn->op->arity == OP_NUM)) node_free(fn);
else if (fn->op->arity == OP_NAME) name_free((NAME_NODE *) fn);
/* if node is a nullary operator */
else if (fn->op->arity == NULLARY) {
    if (((TERM_NODE *)fn)->label) name_free(((TERM_NODE *) fn)->label);
    node_free(fn);
    }
else if ((fn->op->arity & BINARY) || (fn->op->arity & UNARY)) {
    if (((TERM_NODE *)fn)->label) name_free(((TERM_NODE *) fn)->label);
    if ((fn->op->arity & BINARY) || (fn->op->arity == POSTFIX))
	expr_free(((TERM_NODE *)fn)->left);
    if (fn->op->arity != POSTFIX) expr_free(((TERM_NODE *)fn)->right);
    node_free(fn);
    }
else {
    fprintf(stderr, "arity: %s\n", arity_name(fn->op->arity));
    error("Unknown arity while printing expression tree");
    }
}

/***********************************************************************
 *
 * Traverse and print the expression tree inorder.
 *
 * entry:	root of tree (or subtree)
 *
 ***********************************************************************/
void
expr_print(p)
NODE *p;
{
void name_print();	/* from names.c */
char *arity_name();	/* from ops.c */

/* static unsigned char *hit_newline = 0;
if (_iob[2]._ptr < hit_newline || 65 < _iob[2]._ptr - hit_newline) {
    hit_newline = _iob[2]._ptr;
    fprintf(stderr,"\n   ");
    } */
/* if node is a string */
if (p->op->arity == OP_STR) fprintf(stderr, "\"%s\"", ((STR_NODE *)p)->value);
/* if node is a number */
else if (p->op->arity == OP_NUM) fprintf(stderr, "%g", ((NUM_NODE *)p)->value);
/* if node is a name */
else if (p->op->arity == OP_NAME) name_print((NAME_NODE *) p);
/* if node is a nullary operator */
else if (p->op->arity == NULLARY) {
    if (((TERM_NODE *)p)->label) {
	name_print(((TERM_NODE *)p)->label);
	fprintf(stderr, ":");
	}
    if (isalpha(p->op->pname[0])) fprintf(stderr, " %s ", p->op->pname);
    else fprintf(stderr, "%s", p->op->pname);
    }
/* if node is a binary or unary operator */
else if ((p->op->arity & BINARY) || (p->op->arity & UNARY)) {
    /* if node is labeled */
    if (((TERM_NODE *)p)->label) {
	name_print(((TERM_NODE *)p)->label);
	fprintf(stderr, ":");
	}
    if (p->op->arity != OUTFIX1) fprintf(stderr, "(");
    /* print left argument */
    if ((p->op->arity & BINARY) || (p->op->arity == POSTFIX))
	expr_print(((TERM_NODE *)p)->left);
    /* print operator.  If alphabetic, put spaces around it */
    if (isalpha(p->op->pname[0])) fprintf(stderr, " %s ", p->op->pname);
    else fprintf(stderr, "%s", p->op->pname);
    /* print right argument */
    if (p->op->arity != POSTFIX) expr_print(((TERM_NODE *)p)->right);
    /* print matching outfix operator */
    if (p->op->arity == OUTFIX1) fprintf(stderr, "%s", p->op->other->pname);
    else fprintf(stderr, ")");
    }
else {
    fprintf(stderr, "arity: %s\n", arity_name(p->op->arity));
    error("Unknown arity while printing expression tree");
    }
}

/***********************************************************************
 *
 * Make a copy of an expression tree.
 *
 * entry:	root of tree.
 *
 * exit:	root of a copy of the tree.
 *
 ***********************************************************************/
NODE *
expr_copy(otree)
NODE *otree;		/* old expression to copy */
{
NAME_NODE *name_copy();	/* from names.c */
char *arity_name();	/* from ops.c */

if (!otree) error("null node in expr_copy!");
if (!otree->op) error("node with no operator in expr_copy!");

if (otree->op->arity & OP_TERM) {		/* this is a TERM_NODE */
    TERM_NODE *te = (TERM_NODE *) node_new();
    te->op = otree->op;	/* copy operator */
    if (((TERM_NODE *) otree)->label)
	te->label = name_copy(((TERM_NODE *) otree)->label);
    else te->label = (NAME_NODE *) NULL;
    if (((TERM_NODE *) otree)->right)
	te->right = expr_copy(((TERM_NODE *) otree)->right);
    else te->right = (NODE *) NULL;
    if (((TERM_NODE *) otree)->left)
	te->left = expr_copy(((TERM_NODE *) otree)->left);
    else te->left = (NODE *) NULL;
    return (NODE *) te;
    }
if (otree->op->arity & OP_NAME) {
/*    if (((NAME_NODE *) otree)->value)	/* replace variable by its value *//*
	return expr_copy(((NAME_NODE *) otree)->value);
    else */ return (NODE *) name_copy((NAME_NODE *) otree);
    }
if (otree->op->arity & OP_NUM) {
    NUM_NODE *ne = (NUM_NODE *) node_new();
    ne->op = otree->op;
    ne->value = ((NUM_NODE *) otree)->value;
    return (NODE *) ne;
    }
if (otree->op->arity & OP_STR) {
    STR_NODE *se = (STR_NODE *) node_new();
    se->op = otree->op;
    /* WARNING: following leaves two pointers to the same string */
    se->value = ((STR_NODE *) otree)->value;
    return (NODE *) se;
    }
/* if we get here, then there is an error.  Shouldn't ever happen */
fprintf(stderr, "operator: %s, arity: %s\n",
    otree->op->pname, arity_name(otree->op->arity));
error("invalid operator arity in expr_copy");
}

/***********************************************************************
 *
 * Walk expression tree replacing bound variables by their values.
 *
 * entry:	root of tree.
 *
 * exit:	root of updated tree.
 *
 ***********************************************************************/
NODE *
expr_update(tree)
NODE *tree;
{
void name_free();		/* from names.c */

if (!tree) error("null node in expr_update!");
if (!tree->op) error("node with no operator in expr_update!");

if (tree->op->arity & OP_NAME) {	/* a NAME_NODE */
    if (((NAME_NODE *)tree)->value) {	/* variable is bound */
	/* note recursion! */
	NODE *val = expr_update(((NAME_NODE *)tree)->value);
	((NAME_NODE *)tree)->value = val;
	val = expr_copy(val);
	name_free((NAME_NODE *)tree);
	return val;
	}
    else return tree;
    }
if (tree->op->arity & OP_TERM) {	/* a TERM_NODE */
    if (((TERM_NODE *)tree)->left)
	((TERM_NODE *)tree)->left = expr_update(((TERM_NODE *)tree)->left);
    if (((TERM_NODE *)tree)->right)
	((TERM_NODE *)tree)->right = expr_update(((TERM_NODE *)tree)->right);
    }
return tree;	/* anything else */
}
