/************************************************************
 *
 * Routines to manage rules
 *
 ************************************************************/

#include "def.h"
int label_count;		/* number of label names in a rule */

/*****************************************************************
 *
 * Compare two pattern expressions to determine which is "more specific".
 * In general, a pattern is more specific if it matches a subset of
 * the expressions matched by another pattern.  We require, however,
 * that two expressions are equally specific ONLY if they are the same
 * expression, so we need to set up a total ordering out of this partial
 * ordering.
 *
 * returns:
 *	1 if A is more specific than B
 *	-1 if B is more specific than A
 *	0 if they are equally specific
 *
 *
 *  An untyped parameter (which matches anything) is less specific
 *  than anything else.
 *  An operator or type is more specific than a supertype.
 *  A term is more specific than a parameter.
 *  compare the precedence of operator A to the precedence of operator B
 *  compare the address of operator A to the address of operator B
 *
 * If the two operators the same, then compare their arguments.
 *
 *****************************************************************/
static int
more_specific(A, B)
NODE *A, *B;
{
extern OP *untyped_prim;	/* from primitive.c */
extern OP *undeclared_prim;	/* from primitive.c */
extern OP *positive_type, *nonzero_type;	/* from primitive.c */
register OP *opa = A->op;
register OP *opb = B->op;
register OP *sop;

if (opa != opb) {
    if (opb == untyped_prim) return 1;
    if (opa == untyped_prim) return -1;
    for (sop = opa->super; sop; sop = sop->super) if (sop == opb) return 1;
    for (sop = opb->super; sop; sop = sop->super) if (sop == opa) return -1;
    if (opb->arity == OP_NAME && opa->arity != OP_NAME) return 1;
    if (opa->arity == OP_NAME && opb->arity != OP_NAME) return -1;
    if (opa->precedence > opb->precedence) return 1;
    if (opa->precedence < opb->precedence) return -1;
    if (opa > opb) return 1;
    if (opa < opb) return -1;
    }
/* same operator */
if (opa->arity == NULLARY) return 0;
if ((opa->arity & BINARY) || (opa->arity & UNARY)) {
    if ((opa->arity & BINARY) || (opa->arity == POSTFIX))
	return more_specific(((TERM_NODE *)A)->left, ((TERM_NODE *)B)->left);
    if (opa->arity != POSTFIX)
	return more_specific(((TERM_NODE *)A)->right, ((TERM_NODE *)B)->right);
    }
return 0;
}

/************************************************************
 *
 * Build a rule, and insert it as the hash value of the
 * root operator of the head expression.
 * Return a pointer to the rule, in case someone wants to
 * print it, or something.
 *
 * TO DO: shouldn't use malloc direcly.
 *
 ************************************************************/
RULE *
rule_build(head, body, tag, names)
NODE *head, *body;
OP *tag;
NAME_NODE *names;		/* local name space */
{
char *malloc();
void rule_print();		/* forward reference */
register RULE *rr;
RULE *cr, *pr = NULL;		/* used to insert rule into list */
int cmp;
static int rule_verbose = -1;

if (!(head->op->arity & OP_TERM)) {
    error("head of rule must be an expression");
    }
rr = (RULE *) malloc(sizeof(RULE));
rr->head = head;
rr->body = body;
rr->tag = tag;
rr->space = names;
rr->size = label_count;		/* number of label names */

if (rule_verbose == -1) rule_verbose = verbose;
if (rule_verbose) {
    rr->verbose = 1;
    rule_print(rr);
    }
else rr->verbose = 0;
rule_verbose = verbose;

cr = head->op->hash;
if (cr == NULL) {	/* no rules for this op */
    rr->next = NULL;
    head->op->hash = rr;
    return rr;
    }
while (cr) {
    if (1 == more_specific(rr->head, cr->head)) {	/* insert here */
	rr->next = cr;
	if (pr) pr->next = rr;
	else head->op->hash = rr;
	return rr;
	}
    pr = cr;
    cr = cr->next;
    }
pr->next = rr;
rr->next = NULL;
return rr;
}

/************************************************************
 *
 * Free a rule.
 *
 ************************************************************/
void rule_free(rr)
RULE *rr;
{
void expr_free();	/* from expr.c */

expr_free(rr->head);
expr_free(rr->body);
name_free(rr->space);
free((char *)rr);
}

/************************************************************
 *
 * Print out a rule.
 *
 ************************************************************/
void rule_print(rp)
RULE *rp;
{
void expr_print();		/* from expr.c */
void name_space_print();	/* from names.c */

expr_print(rp->head);
fprintf(stderr, " { ");
expr_print(rp->body);
fprintf(stderr, " }");
if (rp->tag) fprintf(stderr, " '%s", rp->tag->pname);
fprintf(stderr, "\t");
name_space_print(rp->space);
fprintf(stderr, "\n");
}
