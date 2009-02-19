#include "def.h"
#include <string.h>

#define MAXP 32		/* maximum number of parameters in a rule */
NODE *param_val[MAXP];	/* array of parameter values */
int learn;		/* did I learn anything? */
int bondage;		/* did a variable get bound? */
static SNODE *stack;	/* stack for walking tree */

#define WR  1		/* walk right next */
#define POP 2		/* pop stack next */
#define HAS_ARG (UNARY | BINARY)	/* is a term that has an argument */

typedef struct sub_stack {
    struct sub_stack *next;
    NODE *exp;
    } SUB_STACK;
static SUB_STACK *sub_top = NULL;

/******************************************************************
 *
 * Push old subject expression on stack, get new subject expression.
 *
 ******************************************************************/
void
subject_push(old)
NODE *old;		/* old subject expression */
{
void *malloc();
register SUB_STACK *node;

node = (SUB_STACK *) malloc(sizeof(SUB_STACK));
node->next = sub_top;
node->exp = old;
}

/******************************************************************
 *
 * Pop old subject expression off stack.
 *
 ******************************************************************/
NODE *
subject_pop()
{
void free();
register SUB_STACK *node;
register NODE *exp;

if (!sub_top) return NULL;
node = sub_top;
sub_top = node->next;
exp = node->exp;
free(node);
return exp;
}

/******************************************************************
 *
 * Find a rule that matches an expression.
 *
 * entry:	an expression
 *
 * exit:	returns the rule that matched
 *		return NULL if no rule matches this expression.
 *		Does not try to match against subexpressions.
 *
 ******************************************************************/
static RULE *
match(exp)
NODE *exp;	/* the expression to match */
{
int match_sub(register NODE *, register NODE *);	/* forward reference */
register RULE *rtt;	/* rule to try */

/* this assumes that the root of all rule heads are terms */
if (!(exp->op->arity & OP_TERM)) return (RULE *) NULL; 

for(rtt = exp->op->hash; rtt; rtt = rtt->next) {
    if (match_sub(rtt->head, exp)) return rtt;
    }
return (RULE *) NULL;	/* no rule matched */
}

/******************************************************************
 *
 * See if a parameter with a guard matches its argument
 *
 ******************************************************************/

static int
match_types(guard, exp)
OP *guard;		/* type to match */
NODE *exp;		/* expression to match against */
{
register OP *mt = exp->op;
for (; mt; mt = mt->super) if (guard == mt) return TRUE;
return FALSE;
}

/******************************************************************
 *
 * Match a single rule against an expression.
 * TO DO:	Works recursively, SHOULD USE THE STACK.
 *
 ******************************************************************/
int
match_sub(head, exp)
register NODE *head;		/* pattern to match against */
register NODE *exp;		/* subexpression to match */
{
char *arity_name();		/* from ops.c */
extern OP *untyped_prim;	/* from primitive.c */

if (head->op->arity == OP_STR) {
    return (exp->op->arity == OP_STR && 0 == strcmp(
	((STR_NODE *) head)->value, ((STR_NODE *) exp)->value));
    }
if (head->op->arity == OP_NUM) {
    return (exp->op->arity == OP_NUM &&
	((NUM_NODE *) head)->value == ((NUM_NODE *) exp)->value);
    }
if (head->op->arity == OP_NAME) {	/* parameter */
    if (head->op == untyped_prim || match_types(head->op, exp)) {
	/* bind value to parameter */
	((NAME_NODE *) head)->value = exp;
	return TRUE;
	}
    else return FALSE;
    }
if (head->op->arity == NULLARY) {
    return ((exp->op->arity == NULLARY) && (head->op == exp->op));
    }
if (head->op->arity & UNARY) {
    if ((head->op->arity != exp->op->arity) || (head->op != exp->op))
	return FALSE;
    if (head->op->arity == POSTFIX) return match_sub(
	((TERM_NODE *) head)->left, ((TERM_NODE *) exp)->left);
    else return		/* PREFIX and OUTFIX */
	match_sub(((TERM_NODE *) head)->right,((TERM_NODE *) exp)->right);
    }
if (head->op->arity & BINARY) {
/* Had to change the following because of a bug in the Sun C compiler */
/*  return ((exp->op->arity & BINARY) &&
	match_sub(((TERM_NODE *) head)->left, ((TERM_NODE *) exp)->left) &&
	match_sub(((TERM_NODE *) head)->right,((TERM_NODE *) exp)->right)); */
    if ((!(exp->op->arity & BINARY)) || (head->op != exp->op)) return FALSE;
    if (!match_sub(((TERM_NODE *) head)->left, ((TERM_NODE *) exp)->left))
	return FALSE;
    if (!match_sub(((TERM_NODE *) head)->right,((TERM_NODE *) exp)->right))
	return FALSE;
    return TRUE;
    }
fprintf(stderr, "arity: %s\n", arity_name(exp->op->arity));
error("Unknown arity during pattern match!");
return FALSE;	/* will never execute */
}

/*************************************************************
 *
 * Walk the tree, looking for subexpressions that match a rule
 *
 * exit:	possibly transformed expression
 *		sets global variable "learn" if transformed.
 *
 *************************************************************/
NODE *
walk(subject)
NODE *subject;		/* subject expression */
{
SNODE *st_get();		/* from util.c */
NODE *instantiate();		/* forward reference */
NODE *primitive_execute();	/* from primitives.c */
void expr_free();		/* from expr.c */
NAME_NODE *name_space_insert();	/* from names.c */
void name_free();		/* from names.c */
NODE *expr_copy();		/* from expr.c */
NODE *expr_update();		/* from expr.c */
extern OP *untyped_prim;	/* from primitive.c */

register NODE *cn = subject;	/* current node */
register SNODE *stn;		/* a stack node */
NAME_NODE *ts;			/* temp name space pointer */
RULE *mrule;			/* the rule that matched */
NODE *ib;			/* instantiated body */

learn = FALSE;			/* haven't learned anything yet */
stack = (SNODE *) NULL;		/* initially empty */

for (;;) {	/* for ever */
    if (cn->op->arity == OP_NAME && ((NAME_NODE *)cn)->value) {
	fprintf(stderr, "variable: ");
	name_print((NAME_NODE *)cn);
	fprintf(stderr, "\n");
	error("Found loose bound variable in subject expression!");
	}
    else if (mrule = match(cn)) {	/* found a match */
	learn = TRUE;
	if ((mrule->verbose + verbose)>1) {
	    fprintf(stderr, "\nMATCH: ");
	    rule_print(mrule);
	    fprintf(stderr, "  REWRITE: ");
	    // expr_print(subject);
	    expr_print(cn);
	    fprintf(stderr, " ==> ");
	    }
	/* if rule has a tag, and redex is labeled, then type the label */
	if ((cn->op->arity & OP_TERM) && (((TERM_NODE *) cn)->label)) {
	    ((TERM_NODE *) cn)->label->op = (mrule->tag) ?
		(mrule->tag) : untyped_prim;
	    ts = name_space_insert(mrule->space, ((TERM_NODE *) cn)->label);
	    }
	else {		/* create new (disjoint) name space */
	    ts = name_space_insert(mrule->space, (NAME_NODE *) NULL);
	    name_free(ts);	/* root of space is dummy node */
	    }
	if (mrule->body->op->eval > 0) {
	    ib = primitive_execute(mrule->body->op->eval, cn);	/* primitive */
	    }
	else ib = instantiate(mrule->body);	/* regular rule */
	expr_free(cn);
	ib = expr_update(ib);	/* remove any bound variables */
	if (stack) {
	    if ((stack->info == WR) || (stack->node->op->arity == POSTFIX))
		((TERM_NODE *) stack->node)->left = ib;
	    else ((TERM_NODE *) stack->node)->right = ib;
	    }
	else subject = ib;
	if (bondage) {	/* a variable was bound */
	    subject = expr_update(subject);
	    bondage = FALSE;
	    }
	if ((mrule->verbose + verbose)>1) {
	    expr_print(ib);
	    fprintf(stderr, "\n  SUBJECT: ");
	    expr_print(subject);
	    fprintf(stderr, "\n");
	    }
	return subject;
	}
    else {	/* walk children */
	/* do not walk children if eval function = -4 (usually []) */
	if (cn->op->arity & HAS_ARG && cn->op->eval != -4) {
	    stn = st_get();
	    stn->next = stack;	/* push on stack */
	    stack = stn;
	    stn->node = cn;
	    if (cn->op->arity & BINARY) {
	 	stn->info = WR;		/* next action is walk right */
		cn = ((TERM_NODE *) cn)->left;
		}
	    else {	/* unary */
		stn->info = POP;	/* next action is pop */
		if (cn->op->arity == POSTFIX) cn = ((TERM_NODE *) cn)->left;
		else cn = ((TERM_NODE *) cn)->right;
		}
	    }
	else {		/* terminal node, walk back up stack */
	    stn = NULL;
	    do {
		if (stn) st_free(stn);
		stn = stack;
		if (!stn) return subject;
		cn = stn->node;
		stack = stn->next;
		} while (stn->info == POP);
	    stack = stn;	/* push back, walk right */
	    cn = ((TERM_NODE *) cn)->right;
	    stn->info = POP;	/* next move will be a pop */
	    }
	}	/* end of no match at this node */
    }		/* end of forever */
}

/*************************************************************
 *
 * Instantiate the body of a rule
 * Make a copy of the expression, insert parameters,
 *  put other names into name space.
 *
 * exit:	new expression to be inserted into subject expression
 *
 *************************************************************/
NODE *
instantiate(body)
NODE *body;		/* body of rule */
{
NAME_NODE *name_copy();		/* from names.c */
NODE *expr_copy();		/* from expr.c */
NODE *node_new();		/* from expr.c */
char *arity_name();		/* from ops.c */

if (!body) error("cannot instantiate null rule body");
if (!body->op) error("missing operator in instantiate");

if (body->op->arity & OP_TERM) {	/* TERM_NODE */
    TERM_NODE *te = (TERM_NODE *) node_new();
    te->op = body->op;
    if (((TERM_NODE *) body)->label)
	te->label = name_copy(((TERM_NODE *) body)->label->value);
    else te->label = (NAME_NODE *) NULL;
    if (((TERM_NODE *) body)->right)
	te->right = instantiate(((TERM_NODE *) body)->right);
    else te->right = (NODE *) NULL;
    if (((TERM_NODE *) body)->left)
	te->left = instantiate(((TERM_NODE *) body)->left);
    else te->left = (NODE *) NULL;
    return (NODE *) te;
    }
if (body->op->arity == OP_NUM) {
    NUM_NODE *ne = (NUM_NODE *) node_new();
    ne->op = body->op;
    ne->value = ((NUM_NODE *) body)->value;
    return (NODE *) ne;
    }
if (body->op->arity == OP_STR) {
    STR_NODE *se = (STR_NODE *) node_new();
    se->op = body->op;
    /* WARNING: following leaves two pointers to the same string */
    se->value = ((STR_NODE *) body)->value;
    return (NODE *) se;
    }
if (body->op->arity == OP_NAME) {	/* parameter or local name */
    return expr_copy(((NAME_NODE *) body)->value);
    }
/* if we get here, then there is an error */
fprintf(stderr, "operator: %s, arity: %s\n",
    body->op->pname, arity_name(body->op->arity));
error("invalid operator arity in instantiate");
}
