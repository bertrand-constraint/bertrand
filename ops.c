/********************************************************************
 *
 * Manage operator nodes.
 *
 * Entry points are "op_new" to allocate an operator node,
 * and "op_mem_free" to free all operator memory.
 * Other entry points are for debugging.
 *
 ********************************************************************/

#include "def.h"

/* linked lists that hold operators definitions */
OP *single_op = NULL;	/* list of single-character operators */
OP *double_op = NULL;	/* list of double-character operators */
OP *name_op = NULL;	/* list of alphanumeric operators */
OP *type_op = NULL;	/* list of types */

#define OP_BYTES 1024	/* number of operator bytes to allocate */
static char *op_mem = NULL;		/* operator memory */
static int free_byte = OP_BYTES;	/* next free byte */

/* VERY MACHINE DEPENDENT */
#define ALIGN 4		/* align op nodes on 4 byte boundaries */

/********************************************************************
 *
 * Allocate memory for operator nodes.
 * Size of node depends upon length of operator name, but will always
 * be a multiple of 4 bytes.  This is very machine dependent.
 * 68000 only requires even byte alignment.  Other machines
 * may require size to be a some other multiple of 2.
 *
 * entry:	length of operator name.
 *
 * exit:	address of operator node returned
 *
 ********************************************************************/
OP *
op_new(pl)
int pl;		/* length of print name (not counting null) */
{
char *malloc();
int asize;	/* number of bytes to allocate */
OP *op;

asize = sizeof(OP) + pl;	/* includes added space for null */
if ((asize % ALIGN) != 0) asize += ALIGN - (asize % ALIGN);
#ifdef DEBUG
printf("allocating operator node of %d bytes\n", asize);
fflush(stdout);
#endif

if (asize > OP_BYTES - free_byte) {
    char *old_op_mem = op_mem;
    op_mem = malloc(OP_BYTES); 
    if (!op_mem) error("out of memory");
    ((char **) op_mem)[0] = old_op_mem;	/* first word points to old memory */
    free_byte = sizeof(char *);		/* skip first word */
#   ifdef DEBUG
    printf("allocated operator node memory\n");
#   endif
    }
op = (OP *) (op_mem + free_byte);
free_byte += asize;
op->length = (unsigned char) pl;
op->eval = 0;
op->hash = (RULE *) NULL;
op->super = (OP *) NULL;
op->other = (OP *) NULL;
return op;
}

/********************************************************************
 *
 * Free all operator node memory.
 *
 * This will also free all rules associated with the operators.
 *
 ********************************************************************/
void
op_mem_free()
{
char *next_op_mem;
void rule_free();	/* from rules.c */
void free();
int asize;

register int fpos;
register RULE *rr;

while (op_mem) {
    next_op_mem = *((char **) op_mem);
    fpos = sizeof(char *);
    while (fpos<free_byte) {
	rr = ((OP *)(op_mem+fpos))->hash;
	while(rr) {
	    rule_free(rr);
	    rr = rr->next;
	    }
	asize = sizeof(OP) + ((OP *)(op_mem+fpos))->length;
	if ((asize % ALIGN) != 0) asize += ALIGN - (asize % ALIGN);
	fpos += asize;
	}
    free(op_mem);
    op_mem = next_op_mem;
    }
free_byte = OP_BYTES;	/* to indicate no memory allocated */
single_op = NULL;	/* no single-character operators */
double_op = NULL;	/* no double-character operators */
name_op = NULL;		/* no alphanumeric operators */
type_op = NULL;		/* no types */
}

/********************************************************************
 *
 * Depending on the type of operator, insert node into appropriate 
 * list in alphabetical order.  (Infix version of an operator precedes
 * its unary version.)
 * Make sure that this operator is not a duplicate of another
 * operator with the same arity.  If the operator has the same
 * name as an operator of a different arity (one binary, one unary),
 * link the two together using the "other" field.
 *
 * entry: 	Pointer to pointer to appropriate op node list.
 *		Pointer to operator node to be inserted.
 * 
 * exit:	Node has been linked to appropriate list; 
 *		 if it is the first node in the list, the head of
 *		 the list points to it; error routine is called
 *		 if it is a duplicate operator.
 *
 ********************************************************************/
void
op_put(oplist, op)
OP **oplist;			/* pointer to pointer to op node list */
OP *op;				/* pointer to oper node to be inserted */
{
int val;			/* lexical comparison of operator names */
register OP *curr_op = *oplist;	/* pointer to op node list to insert in */
register OP *prev_op = NULL;	/* pointer to previous op node in list */
char *arity_name();		/* printable form of arity (in this file) */

/* Insert into list curr_op */

if (curr_op == NULL) {		/* list is empty */
    op->next = NULL;
    *oplist = op;	 		/* set pointer to top of list */
    return;
    }

/* insert in alphabetical order */
while (curr_op) {

   /* compare operator names */
   val = strcmp(curr_op->pname, op->pname);

   /* If two operator names are identical, link them together.
      If their arities are also identical, then it is an error. 
      If current operator is infix, insert it now and return;
      otherwise, the operator is unary so wait until we have
      passed the infix operator and insert it then. */

   if (val == 0) {
	curr_op->other = op;
	op->other = curr_op;
	if (op->arity & BINARY) {
	    val = 1;	/* force > 0, so will insert now */
	    if (!(curr_op->arity & UNARY)) {
		fprintf(stderr, "existing operator: %s", curr_op->pname);
		fprintf(stderr, ", arity: %s\n", arity_name(curr_op->arity));
		fprintf(stderr, "input operator: %s", op->pname);
		fprintf(stderr, ", arity: %s\n", arity_name(op->arity));
		fprintf(stderr, "duplicate operator is binary\n");
		fprintf(stderr, "current operator must be unary\n");
		error("invalid duplicate operators");
		}
	    }
	else if (op->arity & UNARY) {
	    /* if op is UNARY, then will get it on next time through */
	    if (!(curr_op->arity & BINARY)) {
		fprintf(stderr, "existing operator: %s", curr_op->pname);
		fprintf(stderr, ", arity: %s\n", arity_name(curr_op->arity));
		fprintf(stderr, "input operator: %s", op->pname);
		fprintf(stderr, ", arity: %s\n", arity_name(op->arity));
		fprintf(stderr, "duplicate operator is unary\n");
		fprintf(stderr, "current operator must be binary\n");
		error("invalid duplicate operators");
		}
	    }
	else {
	    fprintf(stderr, "existing operator: %s", curr_op->pname);
	    fprintf(stderr, ", arity: %s\n", arity_name(curr_op->arity));
	    fprintf(stderr, "input operator: %s", op->pname);
	    fprintf(stderr, ", arity: %s\n", arity_name(op->arity));
	    fprintf(stderr, "operators with the same name may only be");
	    fprintf(stderr, " unary and binary (one of each).\n");
	    error("invalid duplicate operators");
	    }
	}	/* end of duplicate operators */

    if (val > 0) {		/* insert here, and return */
	op->next = curr_op;
	if (prev_op)		/* middle of list */
	   prev_op->next = op;
	else			/* top of list */
	   *oplist = op;
	return;
	}
    /* otherwise, look at next node in list */
    prev_op = curr_op;
    curr_op = curr_op->next;
    }				/* end of while loop */
    
/* hit end of list */
prev_op->next = op;
op->next = NULL;
}

/********************************************************************
 *
 * Routines to print debugging information.
 *
 ********************************************************************/

/********************************************************************
 *
 * Format an arity field for printing.
 *
 * entry:	Numeric arity value, see def.h
 *
 * exit:	String containing print value of arity.
 * 		Error condition if invalid arity (shouldn't happen).
 *
 ********************************************************************/
char *
arity_name(arity)
short arity;
{
static char buf[80];

switch(arity) {
 case NONASSOC:	return("binary nonassociative");
 case LEFT:	return("binary left associative");
 case RIGHT:	return("binary right associative");
 case PREFIX:	return("unary prefix");
 case POSTFIX:	return("unary postfix");
 case OUTFIX1:	return("unary outfix1");
 case OUTFIX2:	return("unary outfix2");
 case NULLARY: 	return("nullary");
 case UNARY:	return("unary");
 case BINARY:	return("binary");
 case OP_NAME:	return("name or type");
 case OP_NUM:	return("number");
 case OP_STR:	return("string");
 default:	sprintf(buf, "unknown arity/associativity: 0x%x", arity);
		return(buf);
    }
}	/* end of arity_name */

/********************************************************************
 *
 * Print a single linked list containing operators.
 *
 ********************************************************************/
void
op_list_print(op)
OP *op;
{
    while (op) {
	fprintf(stderr, "operator: %s, ", op->pname);
	fprintf(stderr, "arity: %s, ", arity_name(op->arity));
	if (op->arity != OUTFIX1 && op->arity != OUTFIX2 &&
	  op->arity != NULLARY)
	    fprintf(stderr, "precedence: %ld\n", op->precedence);
	else fprintf(stderr, "\n");
	if (op->other) {
	    fprintf(stderr, " linked to node whose operator is: %s ",
		op->other->pname);
	    fprintf(stderr, "and whose arity is: %s\n",
		arity_name(op->other->arity));
	    }
	op = op->next;
	}
    }

/********************************************************************
 *
 * Print all of the linked lists containing operators (including types).
 *
 ********************************************************************/
void
ops_print()
{
    fprintf(stderr, "\f");		/* form feed */

    /* Print single_op list */
    fprintf(stderr, "\nsingle special character operators:\n\n");
    op_list_print(single_op);

    /* Print double_op list */
    fprintf(stderr, "\ndouble special character operators:\n\n");
    op_list_print(double_op);

    /* Print name_op list */
    fprintf(stderr, "\nalphanumeric operators:\n\n");
    op_list_print(name_op);

    /* Print list of types */
    fprintf(stderr, "\ntypes (missing single quotes):\n\n");
    op_list_print(type_op);
    }		/* end of ops_print */
