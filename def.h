/* Definitions for Bertrand interpreter */

/* COPYRIGHT (c) 1988  by Wm Leler, all rights reserved */
/* The Bertrand Language, and this interpreter are proprietary to
   Wm Leler, and may not be used or copied without permission. */
static char notice[] = "Copyright (c) 1988 Wm Leler";

#include <stdio.h>
#include <string.h>

/* Location to look for #included files in */
#ifndef LIBDIR
#define	LIBDIR	"/usr/lib/local/bertrand/"
#endif

#define TRUE		1
#define FALSE		0
#ifndef NULL
#define NULL		0
#endif

#define BIG_LONG	2147483646	/* 2^31 - 1 */
#define BIG_SHORT	32767		/* 2^15 - 1 */

#define MAXTOKEN 1023           /* maximum length of an input token */
                                /* including strings! */
#define MAXFILES 16             /* max input files (max depth of #includes) */

extern void error();	/* print error message routine, from util.c */
extern int verbose;	/* print debugging info, from main.c */

/* token types, returned from scan()				*/
#define OPER	301	/* user defined operator		*/
#define	NUMBER	302	/* constant number			*/
#define	IDENT	303	/* identifier that is not an operator	*/
#define	STRING	304	/* character string			*/
#define	TYPE	305	/* type, beginning with single quote	*/
/* The three reserved characters {.} return as themselves.	*/

/* Character input classes, used by scanner.c and prep.c	*/
#define C_EOF	0	/* eof, null			*/
#define C_CTRL	1	/* (invalid) control character	*/
#define C_NL	2	/* lf, cr, ff (lineno++)	*/
#define C_WS	3	/* blank, tab (whitespace)	*/
#define C_SPC	4	/* special character		*/
#define C_NUM	5	/* numeric 0-9			*/
#define C_ALPH	6	/* alphabetic a-Z, underscore _	*/
#define C_PER	7	/* period . (lotsa stuff)	*/
#define C_DQ	8	/* double quote " (string)	*/
#define C_BQ	9	/* back quote ` (string escape)	*/
#define C_SQ	10	/* single quote ' (type)	*/
#define C_BRC	11	/* braces { } (rule body)	*/
#define C_LB	12	/* pound sign # (preprocessor)	*/
/*  Special characters are all symbols such as + * & , etc.	*/
/*  They can be used in one or two character operators.		*/
/*  Periods are used in numbers, to connect compound names,	*/
/*  and to begin comments.					*/

typedef struct rule {
	struct rule *next;	/* next rule in hash table */
	struct node *head;	/* head expression */
	struct node *body;	/* body expression */
	struct op *tag;		/* a type */
	struct namenode *space;	/* local name space */
	short size;		/* number of label names in rule */
	short verbose;		/* trace */
	} RULE, *RULE_PTR;

typedef struct snode {		/* stack nodes */
	struct snode *next;	/* next node in stack */
	short info;
	struct node *node;	/* expression tree */
	} SNODE;

/* Definitions of different kinds of operators */

/* operators (including types) */
typedef	struct op {
	struct op *next;		/* next node in op table */
	short arity;			/* and associativity */
	short precedence;
	short eval;			/* parser reduce function or */
					/* builtin operator */
	struct rule *hash;		/* rules this operator is root of */
	struct op *super;		/* supertype */
	struct op *other;		/* friend operator */
	unsigned char length;		/* length of print name */
	char pname[1];			/* first char of print name */
	/* other characters follow ... */
	} OP, *OP_PTR;

/* arity encodes arity, associativity, and other information */
/* other is matching outfix operator, or other operator with same name */

/* If any of the top 4 bits are on, then this operator is for a TERM_NODE */
/* and the top 4 bits encode arity (number of arguments): */
#define OP_TERM		0xf000	/* this node is a TERM_NODE */
#define NULLARY 	0x1000	/* no arguments */
#define UNARY		0x2000	/* one argument */
#define BINARY		0x4000	/* two arguments */
#define ARB		0x8000	/* number of arguments unknown */

/* The next four bits encode operators for other kinds of nodes */
#define OP_NAME		0x0800	/* for a NAME_NODE */
#define OP_NUM		0x0400	/* for a NUM_NODE */
#define OP_STR		0x0200	/* for a STR_NODE */

/* The remaining 8 bits encode different things depending upon the arity. */
/* If arity is BINARY: */
#define RIGHT		0x4001	/* binary right associative */
#define LEFT		0x4002	/* binary left associative */
#define NONASSOC 	0x4004	/* binary non-associative */

/* If arity is UNARY: */
#define PREFIX		0x2001	/* unary prefix */
#define POSTFIX		0x2002	/* unary postfix */
#define OUTFIX1		0x2004	/* starting outfix (matchfix) */
#define OUTFIX2		0x2008	/* finishing outfix (matchfix) */

/* Nodes that live in expressions (shouldn't stow thrones) */

/* expression term node */
typedef struct termnode {
	struct op *op;		/* operator description */
	struct namenode	*label;	/* optional label for node */
	struct node *right;	/* right child (optional) */
	struct node *left;	/* left child (optional) */
	} TERM_NODE, *TERM_NODE_PTR;

/* name node */
typedef struct namenode {
	struct op *op;		 /* type of name */
	struct namenode *next;	 /* next child of parent */
	struct namenode *parent; /* back pointer to parent */
	struct namenode *child;	 /* children of this name */
	char *pval;		 /* print value of name */
	struct node *value;	 /* also used during instantiation */
	short refs;		 /* reference count for garbage collection */
	short interest;		 /* how interesting is this variable? */
	} NAME_NODE, *NAME_NODE_PTR;

/* numeric constant node */
typedef struct numbernode {
	struct op *op;		/* number operator */
	double	value;		/* value of number */
	} NUM_NODE, *NUM_NODE_PTR;

/* string node */
typedef struct stringnode {
	struct op *op;		/* string operator */
	char	*value;		/* actual string */
	} STR_NODE, *STR_NODE_PTR;

/* this union is used only to determine the maximum size of a node */
union maxnode {
	struct termnode t;
	struct namenode n;
	struct numbernode c;
	struct stringnode s;
	};

/* Generic expression tree node. Used only for storage management */
/* Will actually be a TERM_NODE, NAME_NODE, NUM_NODE or STR_NODE */
typedef struct node {
	struct op *op;		/* operator description */
	struct node *next;	/* free list pointer */
	/* pad to max size of any node */
	char filler[sizeof(union maxnode) -
	    (sizeof(struct op *) + sizeof(struct node *))];
	} NODE, *NODE_PTR;
