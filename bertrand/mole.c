/*****************************************************************
 *
 * These routines are not finished yet, and have not been included
 * in the current Bertrand interpreter.  As they currently stand,
 * they make invalid assumptions about the form of a linear expression.
 * When a bound variable in a linear expression gets replaced by
 * its value, these assumptions will be violated.  There might be
 * other bugs as well (and ole_solve has not even been written yet!).
 *
 * Wm Leler
 *
 *****************************************************************/

/*****************************************************************
 *
 * Manipulate Ordered Linear Expressions
 *
 * Some of these routines could be implemented in Bertand directly
 * but were done instead as primitives for speed.
 * See the comments for each routine.
 *
 * Linear expressions are always of the form:
 *	(c1**v1) ++ (c2**v2) ++ ... ++ (cn**vn) ++ k
 * where the v's are variables and the c's and the k are constants.
 * (See note above for a problem concerning what happens when a
 * variable v becomes bound and is replaced by its value.)
 *
 *****************************************************************/

#include "def.h"

NODE *expr_copy();	/* from expr.c */
NODE *node_new();	/* from expr.c */
NODE *expr_update();	/* from expr.c */

/*****************************************************************
 *
 * Multiply a linear expression by a constant.
 *
 * The argument must be an expression of the form:
 *	k'constant * lx'linear
 *
 * Equivalent Bertrand code for this:
 *	k'constant * ((c**v) ++ rest)	{ ((k*c)**v) ++ (k * rest) }
 *
 *****************************************************************/
NODE *
ole_multiply(tn)
TERM_NODE *tn;
{
double k = ((NUM_NODE *)(tn->left))->value;
NODE *answer = expr_copy(tn->right);
register TERM_NODE *t = (TERM_NODE *) answer;

while(t) {	/* ever */
    if (t->op->arity == OP_NUM) {	/* the constant at the end */
	((NUM_NODE *)t)->value *= k;
	return answer;
	}
    ((NUM_NODE *)((TERM_NODE *)(t->left))->left)->value *= k;
    t = (TERM_NODE *)(t->right);
    }

expr_print(tn->right);
error("invalid linear expression to multiply");
}

/*****************************************************************
 *
 * Add two linear expressions.
 *
 * The argument must be an expression of the form:
 *	lx1'linear + lx2'linear
 *
 * Equivalent Bertrand code:
 *	... add a constant to a linear expression
 *	k'constant + ((c**v) ++ rest)	{ (c**v) ++ (k + rest) }
 *	((c**v) ++ rest) + k'constant	{ (c**v) ++ (k + rest) }
 *	... add two linear expressions
 *	((c1**v1'numvar)++r1) + ((c2**v2'numvar)++r2) {
 *	    lx_merge ( (1+(v1 lexc v2)), ((c1**v1)++r1) , ((c2**v2)++r2) }
 *	lx_merge( 0, (t1++r1) , lx2 ) { t1 ++ (r1 + lx2) }	.. less
 *	lx_merge( 2, lx1 , (t2++r2) ) { t2 ++ (lx1 + r2) }	.. greater
 *	lx_merge( 1, ((c1**v1)++r1) , ((c2**v2)++r2) ) {	.. equal
 *	    lx_merge ( (c1 = -c2), ((c1**v1)++r1) , ((c2**v2)++r2) }
 *	lx_merge(  true, (t1++r1) , (t2++r2) ) { r1 + r2 }
 *	lx_merge( false, ((c1**v1)++r1) , ((c2**v2)++r2) ) {
 *	    ((c1+c2)**v1) ++ (r1 + r2) }
 *
 *****************************************************************/
NODE *
ole_add(exp)
TERM_NODE *exp;
{
register TERM_NODE *le = (TERM_NODE *)(exp->left);
register TERM_NODE *re = (TERM_NODE *)(exp->right);
NODE *answer;
register TERM_NODE *pa = NULL;	/* position in answer */
double con;			/* merged constant */
register TERM_NODE *newt;	/* new term */

for(;;) {	/* ever */
    /* next term in answer is the constant (the final term) */
    if ((le->op->arity == OP_NUM) && (re->op->arity == OP_NUM)) {
	newt = (TERM_NODE *) expr_copy((NODE *)le);
	((NUM_NODE *)newt)->value =
	    ((NUM_NODE *)le)->value + ((NUM_NODE *)re)->value;
	if (pa) pa->right = (NODE *) newt;
	else {	/* answer is just a constant (other terms cancelled!) */
	    answer = (NODE *) newt;
	    }
	return answer;
	}	/* end of constant term */
    /* merge terms from le and re */
    else if ((le->op->arity != OP_NUM) && (re->op->arity != OP_NUM) &&
	(((TERM_NODE *)(le->left))->right==((TERM_NODE *)(re->left))->right)) {
	con = ((NUM_NODE *)(((TERM_NODE *)(le->left))->left))->value
	    + ((NUM_NODE *)(((TERM_NODE *)(re->left))->left))->value;
	if (con != 0.0) {
	    newt = (TERM_NODE *) node_new();
	    if (pa) pa->right = (NODE *) newt;
	    else answer = (NODE *) newt;
	    pa = newt;
	    pa->label = (NAME_NODE *) NULL;
	    pa->op = le->op;
	    pa->left = expr_copy(le->left);
	    ((NUM_NODE *)(((TERM_NODE *)(pa->left))->left))->value = con;
	    }
	le = (TERM_NODE *)(le->right);
	re = (TERM_NODE *)(re->right);
	}
    else {	/* next term in answer comes from le or re */
	newt = (TERM_NODE *) node_new();
	if (pa) pa->right = (NODE *) newt;
	else answer = (NODE *) newt;
	pa = newt;
	pa->label = (NAME_NODE *) NULL;
	if ((re->op->arity == OP_NUM) || (0 < strcmp(
	((NAME_NODE *)(((TERM_NODE *)(le->left))->right))->pval,
	((NAME_NODE *)(((TERM_NODE *)(re->left))->right))->pval) )) {	/* le */
	    pa->op = le->op;	/* get ++ operator */
	    pa->left = expr_copy(le->left);
	    le = (TERM_NODE *)(le->right);
	    }	/* end of term from le */
	else {	/* from re */
	    pa->op = re->op;	/* get ++ operator */
	    pa->left = expr_copy(re->left);
	    re = (TERM_NODE *)(re->right);
	    }	/* end of term from re */
	}
    }	/* end of forever */
}

/*****************************************************************
 *
 * Solve a linear equation.
 *
 * The argument must be an expression of the form:
 *	0 = lx'linear ; ex
 *
 * This routine walks the expression, and finds the variable (v) in
 * the linear expression lx that occurs furthest to the right in the
 * expression ex.  This (hopefully) finds the most "interesting"
 * variable in lx.  The linear expression lx is then solved for the
 * variable v, and the result bound as the value of v.  Finally,
 * the expression ex is returned.
 * Boundary conditions:
 * If there is only a single variable in lx, it is solved for.
 * If no variable in lx occurs in ex, then the variable with the
 * largest coefficient (in absolute value) is solved for.
 * If multiple variables have equally great coefficients, the one
 * with the name which occurs first alphabetically is solved for.

 * Solving would be more difficult (but not impossible) to implement
 * directly in Bertrand.  Here is some code to get you started.
 *
 *   0 = ((c**v'numvar) ++ rest'number) ; ex {
 *	(v is ((-1/c) * rest)) ; ex }
 *   (c'constant ** k'constant) ++ rest { (c*k) + rest }
 *   (c'constant ** (a ++ b)) ++ rest { (c*(a++b)) + rest }
 *
 *****************************************************************/
NODE *
ole_solve(exp)
TERM_NODE *exp;
{
NODE *ex = expr_update(expr_copy(exp->right));
NODE *lx = expr_update(((TERM_NODE *)(exp->left))->right);
return ex;
}
