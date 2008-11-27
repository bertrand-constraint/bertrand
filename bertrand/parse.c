/***********************************************************************
 *
 * Attributed Operator Precedence Parser
 *
 ***********************************************************************/

#include "def.h"

/* types of nodes on parse stack */
#define OPER_TYPE  401		/* a lone operator */
#define EXPR_TYPE  402		/* a complete expression */

/* which part of rule are we parsing? */
#define HEAD	   501		/* head of rule */
#define BODY	   502		/* body of rule */

static SNODE *pstack = NULL;		/* (top of) parse stack */

static NODE *boe;		/* beginning of expression */

static int next_token;		/* next token (for lookahead) */
static int token;		/* token identifier returned from scanner.c */
extern double token_val;	/* numeric value of token, from scanner.c */
extern char token_prval[];	/* print value of token, from scanner.c */
extern OP *token_op;		/* oper ptr from scanner.c */
static NAME_NODE *rule_names = NULL;	/* local names for this rule */

/***********************************************************************
 *
 * Print a parse stack.	(for debugging)
 *
 ***********************************************************************/
void
ps_print(st)
SNODE *st;
{
#ifndef __STDC__
void expr_print();		/* from expr.c */
#endif

while(st) {
    if (st->info == OPER_TYPE) fprintf(stderr, "\tOP: %s\n", st->node->op->pname);
    else {
	fprintf(stderr, "\t");
	expr_print(st->node);
	fprintf(stderr, "\n");
	}
    st = st->next;
    }
}

/***********************************************************************
 *
 * Reduce the parse stack by creating a subtree of an operator and its
 * operand(s), popping them off the parse tree and replacing them by
 * a pointer to the operator (root of subtree).
 * An expression is enclosed by BOE and EOE "outfix operators".
 * These will cause the entire expression to reduce properly.
 *
 * entry:	operator node to be reduced
 *
 * exit:	top of parse stack is updated
 *
 ***********************************************************************/
void
reduce(rop)
register SNODE *rop;	/* operator (on parse stack) to be reduced */
{
extern OP *undeclared_prim;	/* from primitive.c */
char *arity_name();		/* from ops.c */
void st_free();			/* from util.c */
extern int label_count;		/* from rules.c */

SNODE *q;			/* temp parse stack pointer */

#ifdef DEBUG
fprintf(stderr, "reducing operator: %s, parse stack:\n", rop->node->op->pname);
ps_print(pstack);
#endif

/* handle special parser reduce functions */
if (rop->node->op->eval < 0) {
#   ifdef DEBUG
    fprintf(stderr, "special reduce function: %d\n", rop->node->op->eval);
#   endif
    switch(- rop->node->op->eval) {
     case 1:	/* throw away operator, typically for "()" */
	if (!(rop->node->op->arity & UNARY)) {	/* only works for unary */
	    fprintf(stderr, "operator: %s, arity: %s\n",
		rop->node->op->pname, arity_name(rop->node->op->arity));
	    error("special reduce function 1 requires unary operator");
	    }
	if (rop == pstack || pstack->info != EXPR_TYPE) {
  	    fprintf(stderr, "unary operator: %s\n", rop->node->op->pname);
	    error("unary operator has no argument");
	    }
	pstack->next = pstack->next->next;	/* delete rop */
	st_free(rop);
	break;
     case 2:	/* label operator, typically for ":" */
	if (!(rop->node->op->arity & BINARY)) {
	    fprintf(stderr, "operator: %s, arity: %s\n",
		rop->node->op->pname, arity_name(rop->node->op->arity));
	    error("special label operator must be binary operator");
	    }
	if (rop == pstack || pstack->info != EXPR_TYPE) {
	    fprintf(stderr, "binary operator: %s\n", rop->node->op->pname);
	    error("binary operator has no right argument");
	    }
	if (!rop->next || rop->next->info != EXPR_TYPE) {
	    fprintf(stderr, "binary operator: %s\n", rop->node->op->pname);
	    error("binary operator has no left argument");
	    }
	if (!(rop->next->node->op->arity == OP_NAME)) {
	    fprintf(stderr, "operator: %s, arity: %s\n",
		rop->next->node->op->pname,
		arity_name(rop->next->node->op->arity));
	    error("special label operator requires name for left argument");
	    }
	if (rop->next->node->op != undeclared_prim) {
	    fprintf(stderr, "parameter: %s\n",
		((NAME_NODE *)rop->next->node)->pval);
	    error("parameter may not be used as a label name");
	    }
	if (!(pstack->node->op->arity & OP_TERM)) {
	    fprintf(stderr, "right argument is: %s, of type: %s\n",
		arity_name(pstack->node->op->arity),  pstack->node->op->pname);
	    error("special label operator requires term for right argument");
	    }
	if (((TERM_NODE *)(pstack->node))->label) {
	    /* eventually should allow multiple labels */
	    fprintf(stderr, "new label: %s, old label: %s, operator: %s\n",
		((NAME_NODE *)(rop->next->node))->pval,
		((TERM_NODE *)(pstack->node))->label->pval,
		pstack->node->op->pname);
	    error("multiple labels on single expression");
	    }
	label_count++;		/* number of label names in a rule */
	((TERM_NODE *)(pstack->node))->label = (NAME_NODE *) rop->next->node;
	pstack->next = pstack->next->next->next;	/* delete : and name */
	st_free(rop->next);
	st_free(rop);
	break;
     case 3:		/* negate a constant */
	if (!(rop->node->op->arity & UNARY)) {	/* only works for unary */
	    fprintf(stderr, "operator: %s, arity: %s\n",
		rop->node->op->pname, arity_name(rop->node->op->arity));
	    error("special negation operator must be unary operator");
	    }
	if (rop == pstack || pstack->info != EXPR_TYPE) {
  	    fprintf(stderr, "unary operator: %s\n", rop->node->op->pname);
	    error("unary operator has no argument");
	    }
	if (!(pstack->node->op->arity & OP_NUM)) {
	    fprintf(stderr, "right argument is: %s, of type: %s\n",
		arity_name(pstack->node->op->arity),  pstack->node->op->pname);
	    error("special negation operator requires constant for argument");
	    }
	((NUM_NODE *)(pstack->node))->value *= -1;
	pstack->next = pstack->next->next;	/* delete rop */
	st_free(rop);
	break;
     case 4:	/* don't evaluate (typically []) */
	break;	/* implemented at run time */
     case 5:	/* simplify expression completely */
	break;	/* implemented at run time */
     default:
	fprintf(stderr,"operator: %s, function: %d\n",
	    rop->node->op->pname, rop->node->op->eval);
	error("unknown parser reduce function");
	break;
	}	/* end switch */
#   ifdef DEBUG
    fprintf(stderr, "result expr is: ");
    expr_print(pstack->node);
    fprintf(stderr, "\n");
#   endif
    }	/* end of special parser reduce functions */

/* After the expression has been reduced, pop all of the terminal nodes on
   the stack and replace with a pointer to the top node in the subtree. */

/* binary infix operator */
else if (rop->node->op->arity & BINARY) {
    if (rop == pstack || pstack->info != EXPR_TYPE) {
	fprintf(stderr, "binary operator: %s\n", rop->node->op->pname);
	error("binary operator has no right argument");
	}
    if (!rop->next || rop->next->info != EXPR_TYPE) {
	fprintf(stderr, "binary operator: %s\n", rop->node->op->pname);
	error("binary operator has no left argument");
	}
    ((TERM_NODE *)(rop->node))->right = pstack->node;
    ((TERM_NODE *)(rop->node))->left = rop->next->node;
#   ifdef DEBUG
    fprintf(stderr, "reduce BINARY\n");
    fprintf(stderr, "right child is: %s\n",
	((TERM_NODE *)(rop->node))->right->op->pname);
    fprintf(stderr, "left child is: %s\n",
	((TERM_NODE *)(rop->node))->left->op->pname);
#   endif
    rop->info = EXPR_TYPE;
    st_free(pstack);	/* free right child parse stack node */
    pstack = rop;
    q = rop->next;
    rop->next = rop->next->next;	/* pop expr nodes, leave oper node */
    st_free(q);		/* free left child parse stack node */
    }

/* unary prefix operator */
else if (rop->node->op->arity == PREFIX) {
    if (rop == pstack || pstack->info != EXPR_TYPE) {
  	fprintf(stderr, "prefix operator: %s\n", rop->node->op->pname);
	error("prefix operator has no argument");
	}
    ((TERM_NODE *)(rop->node))->left = NULL;
    ((TERM_NODE *)(rop->node))->right = pstack->node;
#   ifdef DEBUG
    fprintf(stderr, "reduce PREFIX\n");
    fprintf(stderr, "right child is: %s\n",
	((TERM_NODE *)(rop->node))->right->op->pname);
#   endif    
    st_free(pstack);	/* free right child parse stack node */
    pstack = rop;		/* pop off top node */
    rop->info = EXPR_TYPE;
    }

/* unary postfix operator */
else if (rop->node->op->arity == POSTFIX) {
    if (!rop->next || rop->next->info != EXPR_TYPE) {
	fprintf(stderr, "postfix operator: %s\n", rop->node->op->pname);
	error("postfix operator has no left argument");
	}
    ((TERM_NODE *)(rop->node))->left = rop->next->node;
    ((TERM_NODE *)(rop->node))->right = NULL;
#   ifdef DEBUG
    fprintf(stderr, "reduce POSTFIX\n");
    fprintf(stderr, "left child is: %s\n",
	((TERM_NODE *)(rop->node))->left->op->pname);
#   endif    
    rop->info = EXPR_TYPE;
    q = rop->next;
    rop->next = rop->next->next;	/* remove next-to-top node */
    st_free(q);		/* free left child parse stack node */
    }

/* If we have found the outfix1 oper which matches an input outfix2 oper,
   do a special reduce. */

/* unary outfix (matchfix) operator */
else if (rop->node->op->arity == OUTFIX1) {
    if (rop == pstack || pstack->info != EXPR_TYPE) {
  	fprintf(stderr, "outfix operator: %s\n", rop->node->op->pname);
	error("outfix operator has no argument");
	}
    ((TERM_NODE *)(rop->node))->left = NULL;
    ((TERM_NODE *)(rop->node))->right = pstack->node;
#   ifdef DEBUG
    fprintf(stderr, "OUTFIX1\n");
    fprintf(stderr, "right child is: %s\n",
	((TERM_NODE *)(rop->node))->right->op->pname);
#   endif
    rop->info = EXPR_TYPE;
    st_free(pstack);		/* free right child parse stack node */
    pstack = rop;
    }
else {	/* unknown type of operator */
    fprintf(stderr, "can't reduce %s, arity: %s\n", rop->node->op,
	arity_name(rop->node->op->arity));
    error("Don't know how to reduce");
    }

#ifdef DEBUG
fprintf(stderr, "leaving reduce with parse stack:\n");
ps_print(pstack);
#endif

}

/***********************************************************************
 *
 * Shift an expression, or an operator onto the parse stack.
 *
 * entry:	pointer to node
 *		type of node
 *
 ***********************************************************************/
void
shift(p, type)
NODE *p;
int type;
{
#ifndef __STDC__
void expr_print();	/* from expr.c */
#endif
register SNODE *temp;
SNODE *st_get();	/* from util.c */

#ifdef DEBUG
if (type==OPER_TYPE) fprintf(stderr, "shifting operator %s onto pstack\n",
	p->op->pname);
else if (type==EXPR_TYPE) {
	fprintf(stderr, "shifting expression onto pstack: ");
	expr_print(p);
	fprintf(stderr, "\n");
	}
else fprintf(stderr, "shifting unknown %s onto pstack\n",
	p->op->pname);
#endif

temp = st_get();
temp->next = pstack;	/* push onto parse stack */
pstack = temp;
pstack->node = p;	/* assign pointer to expression tree node */
pstack->info = type;	/* assign type of node */
}

/***********************************************************************
 *
 * Expression parser: called by parser to parse head, body or type of
 * rule.  Read from file and call scanner to get tokens; build 
 * expression tree.
 *
 * entry:	flag indicating whether this is the head, body or type
 *		first token read 
 *
 * exit:	return pointer to top of expression tree
 *
 ***********************************************************************/
NODE *
exp_parse(part)
int part;		/* parsing HEAD or BODY of rule */
{
register NODE *cnode;	/* current expression node */
register SNODE *lop;	/* last operator type node on parse stack */
int done = FALSE;	/* true if complete expression has been parsed */
char prev_prval[MAXTOKEN];	/* saved previous token_prval */
int holdtoken = TRUE;	/* true if next token has been read */
NAME_NODE *cspace;	/* current name space */

void st_free();		/* from util.c */
char *char_copy();	/* from util.c */
NODE *node_new();	/* from expr.c */
NODE *name_put();	/* from names.c */
extern OP *pnum_prim;	/* positive constants, from primitive.c */
extern OP *znum_prim;	/* the constant zero, from primitive.c */
extern OP *nnum_prim;	/* negative constants, from primitive.c */
extern OP *str_prim;	/* string constants, from primitive.c */
extern OP *undeclared_prim;	/* from primitive.c */
extern OP *untyped_prim;	/* from primitive.c */
extern NAME_NODE *global_names;	/* from names.c */

#ifdef DEBUG
if (part==HEAD) fprintf(stderr, "parsing HEAD\n");
else if (part==BODY) fprintf(stderr, "parsing BODY\n");
else fprintf(stderr, "parsing unknown = %d\n", part);

if (pstack) fprintf(stderr, "nodes left on parse stack!\n");
#endif

pstack = NULL;		/* reinitialize */

/* Push a beginning-of-expression oper onto the parse stack. */
shift(boe, OPER_TYPE);	

while (!done) {
    if (holdtoken) {
#	ifdef DEBUG
    	fprintf(stderr, "holdtoken is true, using next_token = %s\n", token_prval);
#	endif 
	token = next_token;	/* prev token -> current token */
	holdtoken = FALSE;	/* reset */
	}
    else {
	token = scan();
#	ifdef DEBUG
    	fprintf(stderr, "called scanner, token = %s\n", token_prval);
	if (token == OPER) fprintf(stderr, "OPER precedence = %ld\n",
		token_op->precedence);
#	endif 
	}

    switch(token) {
     case EOF: error("EOF encountered before end of expression");
	break;
     case '{':	/* end of head expression */
	if (part == HEAD) done = TRUE;
	else error("Opening brace found in body of rule");
	break;
     case '}':	/* end of body expression */
	if (part == BODY) done = TRUE;
	else error("Closing brace found, but not in body of rule");
	break;
     case IDENT: 	/* identifier, put in name space */
	/* an identifier must follow an operator */
	if (pstack->info != OPER_TYPE) {
	    fprintf(stderr, "for tokens: ");
	    expr_print(pstack->node);
	    fprintf(stderr, " and %s\n", token_prval);
	    error("missing operator");
	    }
	strcpy(prev_prval, token_prval);	/* save token_prval */
	next_token = scan();		 	/* look ahead */
#	ifdef DEBUG
	fprintf(stderr, "lookahead next_token = %s\n", token_prval);
#	endif
	if (part == HEAD) {	/* must be a parameter */
	    if (next_token == '.') {
		fprintf(stderr, "name beginning with: %s\n", prev_prval);
		error("qualified names illegal in head of rule");
		}
	    if (next_token == TYPE) {
		cnode = name_put(prev_prval, rule_names, token_op);
		}
	    else {
		cnode = name_put(prev_prval, rule_names, untyped_prim);
		holdtoken = TRUE;
		}
	    if (((NAME_NODE *) cnode)->refs != 2) {
		fprintf(stderr, "parameter: %s\n", prev_prval);
		error("reuse of parameter name in head of rule");
		}
	    shift(cnode, EXPR_TYPE);	/* shift as expression */
	    break;
	    }

	/* if we get here, then we must be in the body of the rule */
	if (next_token == TYPE) {
	    fprintf(stderr, "ident: %s, type: %s\n", prev_prval, token_prval);
	    error("types not allowed in body of rule");
	    }

	/* insert name into local name space */
	cspace = rule_names;
	while (next_token == '.') {
	    cspace = (NAME_NODE *) name_put(prev_prval, cspace, undeclared_prim);
	    next_token = scan();	/* look ahead for ident */
	    if (next_token != IDENT) {	
		if (next_token == OPER)
		    fprintf(stderr, "token: %s is an operator\n", token_prval);
		else fprintf(stderr, "token: %s\n", token_prval);
		error("expected identifier following '.'");
		}
	    strcpy(prev_prval, token_prval);
	    next_token = scan();	/* look ahead for '.' */
	    }	/* end of qualified name */
	holdtoken = TRUE;
	cnode = name_put(prev_prval, cspace, undeclared_prim);
	shift(cnode, EXPR_TYPE);	/* shift as expression */
	break;

     case '.':	/* global variable */
	if (part == HEAD) {
	    fprintf(stderr, "token: %s\n", token_prval);
	    error("global names illegal in head of rule");
	    }
	/* an identifier must follow an operator */
	if (pstack->info != OPER_TYPE) {
	    fprintf(stderr, "for tokens: ");
	    expr_print(pstack->node);
	    fprintf(stderr, " and %s\n", token_prval);
	    error("missing operator");
	    }
	cspace = global_names;
	while (next_token == '.') {
	    cspace = (NAME_NODE *) name_put(prev_prval, cspace, undeclared_prim);
	    next_token = scan();	/* look ahead for ident */
	    if (next_token != IDENT) {
		if (next_token == OPER)
		    fprintf(stderr, "token: %s is an operator\n", token_prval);
		else fprintf(stderr, "token: %s\n", token_prval);
		error("expected identifier following '.'");
		}
	    strcpy(prev_prval, token_prval);
	    next_token = scan();	/* look ahead for '.' */
 	    }	/* end of global name */
	holdtoken = TRUE;
	cnode = name_put(prev_prval, cspace, undeclared_prim);
	shift(cnode, EXPR_TYPE);	/* shift as expression */
	break;

     case NUMBER: 
#	ifdef DEBUG
	fprintf(stderr, "type is number\n");
#	endif
	cnode = node_new();
	cnode->op = (token_val > 0.0) ? pnum_prim :
	    ((token_val < 0.0) ? nnum_prim : znum_prim );
	((NUM_NODE *) cnode)->value = token_val;
	if (pstack->info != OPER_TYPE) {
	    fprintf(stderr, "for tokens: ");
	    expr_print(pstack->node);
	    fprintf(stderr, " and %s\n", token_prval);
	    error("missing operator");
	    }
	shift(cnode, EXPR_TYPE);	/* shift onto parse stack */
	break;

     case STRING: 	
#	ifdef DEBUG
	fprintf(stderr, "type is string\n");
#	endif
	cnode = node_new();
	cnode->op = str_prim;	/* constant oper for strings */
	((STR_NODE *) cnode)->value = char_copy(token_prval);
	if (pstack->info != OPER_TYPE) {
	    fprintf(stderr, "for tokens: ");
	    expr_print(pstack->node);
	    fprintf(stderr, " and %s\n", token_prval);
	    error("missing operator");
	    }
	shift(cnode, EXPR_TYPE);	/* shift onto parse stack */
	break;

     case OPER:
#	ifdef DEBUG
	fprintf(stderr, "type of %s is oper\n", token_prval);
#	endif

	cnode = node_new();		/* to hold this operator */
	cnode->op = token_op;		/* oper ptr from scanner */
	((TERM_NODE *) cnode)->label = (NAME_NODE *) NULL;
	((TERM_NODE *) cnode)->left = (NODE *) NULL;
	((TERM_NODE *) cnode)->right = (NODE *) NULL;
	    
	if (cnode->op->arity == NULLARY) {	/* is expression */
	    shift(cnode, EXPR_TYPE);
	    break;
	    }

	if (cnode->op->arity == OUTFIX1 || cnode->op->arity == PREFIX) {
	    shift(cnode, OPER_TYPE);
	    break;
	    }

	/* Keep reducing stack if input operator has lower precedence than 
	   top operator in stack (or if it has equal precedence and is
	   left associative.  */

	for (;;) {		/* for ever */

	    /* the top or next node on the stack must be an operator */
	    lop = pstack;
	    if (lop && lop->info != OPER_TYPE) lop = lop->next;
	    if (!lop || lop->info != OPER_TYPE)
		error("syntax error: missing operator!");
	
	    /* if there is an infix operator on top of the parse stack,
		and the current operator is also infix,
		then one of them must be convertable to unary
		(an infix operator contains a link to a unary
		operator of the same name in the "other" field),
		or else there is an error. */

	    if (lop == pstack && (lop->node->op->arity & BINARY) && 
       		cnode->op->arity & BINARY) {

		/* first check if cnode could be prefix */
		if (cnode->op->other && cnode->op->other->arity == PREFIX) {
		    /* could lop also be postfix? */
		    if (lop->node->op->other &&
			lop->node->op->other->arity == POSTFIX) {
			/* They both can be converted to unary.
			   Pick the one with the higher precedence.
			   If equal precedence, shift. */

			if (lop->node->op->precedence > cnode->op->precedence) {
			    lop->node->op = lop->node->op->other;
#			    ifdef DEBUG
			    fprintf(stderr, "switching last binary node to unary\n");
			    fprintf(stderr, "calling reduce\n");
#			    endif
			    reduce(lop);
			    }
			else {
			    cnode->op = cnode->op->other; 	/* to unary */
#			    ifdef DEBUG
			    fprintf(stderr, "switching current binary node to unary\n");
			    fprintf(stderr, "breaking out and shifting\n");
#			    endif
			    break;
			    }
			}
		    else {	/* only cnode can be converted to unary */
			cnode->op = cnode->op->other; 	/* to unary */
#			ifdef DEBUG
			fprintf(stderr, "switching current binary node to unary\n");
			fprintf(stderr, "breaking out and shifting\n");
#			endif
			break;
			}
		    }
		/* cnode could not be unary, can lop? */
		else if (lop->node->op->other &&
		    lop->node->op->other->arity == POSTFIX) {
#		    ifdef DEBUG
		    fprintf(stderr, "switching last binary node to unary\n");
		    fprintf(stderr, "calling reduce\n");
#		    endif
		    reduce(lop);
		    }
		else {	/* neither can be converted to unary */
		    fprintf(stderr, "operator: %s and operator: %s\n",
			lop->node->op->pname, cnode->op->pname);
		    error("syntax error: missing operand");
		    }
		}	/* end of two infix operators on top of stack */

	    /* if the current operator is binary infix, and there is a
		prefix or outfix1 operator on top of the parse stack,
		see if the current operator can be changed to unary */

       	    if (cnode->op->arity & BINARY && lop == pstack && (lop->node->op->arity
		== PREFIX || lop->node->op->arity == OUTFIX1)) {
		if (cnode->op->other && cnode->op->other->arity == PREFIX) {
		    cnode->op = cnode->op->other; 	/* to unary */
#		    ifdef DEBUG
		    fprintf(stderr, "switching current binary node to unary\n");
		    fprintf(stderr, "breaking out and shifting\n");
#		    endif
		    break;
		    }
		else {
		    fprintf(stderr, "infix operator: %s\n", cnode->op->pname);
		    error("syntax error: missing left operand");
		    }
		}

	    /* If node is an outfix2 operator, reduce the stack
	       to the matching outfix1 operator.  */

	    if (cnode->op->arity == OUTFIX2) {
		if (lop->node->op->arity == OUTFIX1) {
		    if (cnode->op->other == lop->node->op) {
			reduce(lop);
			break;
			}
		    else {
			fprintf(stderr, "outfix operators: %s and %s\n",
			    lop->node->op->pname, cnode->op->pname);
			error("outfix operators do not match");
			}
		    }	/* if outfix1 */
		else reduce(lop);
		}

	    /* Otherwise, check for precedence. */

	    else if (cnode->op->precedence < lop->node->op->precedence ||
		(cnode->op->precedence == lop->node->op->precedence &&
		cnode->op->arity == LEFT)) {
#		ifdef DEBUG
		fprintf(stderr, "calling reduce from <= prec\n");
#		endif
		reduce(lop);
		}

	    else if (lop->node->op->arity != OUTFIX1 &&
		(cnode->op->precedence==lop->node->op->precedence) &&
		(cnode->op->arity == NONASSOC)) {
		fprintf(stderr, "input = %s\n", cnode->op->pname);
		fprintf(stderr, "pstack node = %s\n", lop->node->op->pname);
		error("2 opers to be reduced are nonassociative");
		}

	    else {
#		ifdef DEBUG
	 	fprintf(stderr, "breaking out because none of the if's apply\n");
#		endif
		break;
		}

	    }	/* end of forever */

	/* finally, shift operator onto parse stack */
	if (cnode->op->arity != OUTFIX2) shift(cnode, OPER_TYPE);     

	break;

     case TYPE:
	fprintf(stderr,"type: %s\n", token_prval);
	if (part == BODY) error("types not allowed in body of rule");
	else error("type with no parameter");

     default: 				/* reserved character */
	fprintf(stderr,"token: %s\n", token_prval);
	error("illegal token in rule");

	}		/* end switch */

#   ifdef DEBUG
    fprintf(stderr, "\n");
#   endif
    }	/* end of do until done (end of expression) */

#ifdef DEBUG
ps_print(pstack);
#endif

/* have encountered end of expression, reduce everything */
for (lop = pstack; lop; lop = lop->next) {
    if (lop->info == OPER_TYPE) {
	if (lop->node == boe) break;	/* found bottom of parse stack */
	else {
	    reduce(lop);	/* reduce this operator */
	    lop = pstack;	/* start from the top */
	    }
	}
    }
if (!lop) error("empty parse stack!");
if (lop->next) error("nodes left on parse stack!");
st_free(lop);	/* free boe parse stack node */
cnode = pstack->node;	/* save expression */
st_free(pstack);
pstack = (SNODE *) NULL;
return(cnode);
}

/***********************************************************************
 *
 * Parse rules into heads, bodies and types and send each to the
 * expression parser.  When a rule has been parsed, send its head,
 * body and type to a routine to build the rule.
 *
 * exit:	entire program parsed
 *
 ***********************************************************************/
void parse()
{
RULE *rule_build();		/* from rules.c */
NODE *node_new();		/* from expr.c */
OP *op_new();			/* from ops.c */
void st_mem_free();		/* from util.c */
extern int label_count;		/* from rules.c */
extern OP *undeclared_prim;	/* from primitive.c */

NODE *head;		/* pointer to root of head's expression tree */
NODE *body;		/* pointer to root of body's expression tree */
OP *rule_tag;		/* pointer to root of tag's expression tree */

pstack = NULL;		/* parse stack is empty */

/* initialize boe psuedo operator */
boe = node_new();
boe->op = op_new(3);
strcpy(boe->op->pname,"BOE");
boe->op->arity = OUTFIX1;
boe->op->other = (OP *) NULL;	/* no operator matches boe */
((TERM_NODE *) boe)->label = (NAME_NODE *) NULL;
((TERM_NODE *) boe)->left = (NODE *) NULL;
((TERM_NODE *) boe)->right = (NODE *) NULL;

for (next_token = scan(); EOF != next_token; ) {
    /* initialize namespace for local and parameter names */
    rule_names = (NAME_NODE *) node_new();
    rule_names->op = undeclared_prim;
    rule_names->next = (NAME_NODE *) NULL;
    rule_names->parent = (NAME_NODE *) NULL;
    rule_names->child = (NAME_NODE *) NULL;
    rule_names->pval = NULL;
    rule_names->refs = 1;
    rule_names->interest = 0;
    label_count = 0;		/* number of label names in rule */

    head = exp_parse(HEAD);		/* parse HEAD of rule */
    next_token = scan();
    if (next_token == EOF) error("EOF encountered before end of rule");
    body = exp_parse(BODY);		/* parse BODY of rule */

    next_token = scan();
    if (next_token == TYPE) {	/* optional tag */
	rule_tag = token_op;
	next_token = scan();
	}
    else {
	rule_tag = (OP *) NULL;
	}
    rule_build(head, body, rule_tag, rule_names);
    }	/* for all rules in the input */
st_mem_free();		/* free all parse stack memory */
}
