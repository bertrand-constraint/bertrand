/*************************************************************
 *
 * Primitives
 *
 * Types and their supertypes:
 *
 *      positive -> nonzero -> constant
 *
 *      literal
 *
 *      false
 *      true
 *
 * Primitive constants (not accessible to the programmer):
 *
 *	str_prim -> literal
 *
 *	pnum_prim -> positive		(positive constants)
 *	znum_prim -> constant		(the constant zero)
 *	nnum_prim -> nonzero		(negative constants)
 *
 *	undeclared_prim		(variable not declared, prints as "'?")
 *	untyped_prim		(variable declared via a rule with no type)
 *
 *
 * Types for variables:
 *
 * A local variable is initially of type undeclared_prim.
 * If it labels a redex for a typed rule, the variable becomes
 * of that type.  for an untyped rule, it becomes of type untyped_prim.
 * If the local variable is not a label, or what it labels is never
 * reduced, then that variable stays undeclared (which the user
 * can see as type '?, and treat as an error).
 *
 * A parameter variable with a guard is of the type of the guard.
 * If no guard, then it is of type untyped_prim.
 * Parameters are NEVER of type undeclared_prim.
 *
 *
 * TO ADD YOUR OWN PRIMITIVES:
 *
 * Add the definition for your primitive operator at the end of the
 * routine primitive_init(), and then add the code for your primitive
 * at the end of the big switch statement in primitive_execute().
 * Your primitive operator should be nullary, even if it takes
 * arguments, and you should then define a regular rule that invokes
 * your primitive operator, and ensures that the correct arguments are
 * supplied.  Your primitive will be invoked immediately when this
 * rule is matched.  Your primitive should leave a numeric value in
 * answer->value, and the correct operator will be deduced, or else it
 * should set answer->op to be the correct operator.  If you have
 * problems doing this, look at how the other primitives are done,
 * or, failing all else, write to me for help.
 *
 *************************************************************/

#include "def.h"
#include <math.h>

/* from graphics.c */
extern int graphics;
void graphics_init();
void draw_line();
void draw_string();

OP *pnum_prim;		/* a particular positive numeric constant */
OP *nnum_prim;		/* a particular negative numeric constant */
OP *znum_prim;		/* the numeric constant zero */
OP *str_prim;		/* a particular string constant */
OP *undeclared_prim;	/* not declared yet ('?) */
OP *untyped_prim;	/* declared, but untyped */

OP *positive_type;	/* numeric constants greater than zero */
OP *nonzero_type;	/* numeric constants not equal to zero */
OP *constant_type;	/* any numeric constant */
OP *literal_type;	/* string constants */
/* true and false would not need to be primitives operators, except */
/* that the relational primitives need to return them. */
OP *true_op;		/* boolean true operator */
OP *false_op;		/* boolean false operator */

/*************************************************************
 *
 * Set up all the primitive operators and types.
 *
 *************************************************************/
/* linked lists to put operators and types -- from ops.c */
extern OP *single_op;	/* list of single-character operators */
extern OP *double_op;	/* list of double-character operators */
extern OP *name_op;	/* list of alphanumeric operators */
extern OP *type_op;	/* list of types */

/* used for a type or operator with no supertype */
#define NOSUPER (OP *) NULL

void
primitive_init()
{
OP *primitive();		/* from util.c */

/* primitive types and operators */
constant_type = primitive("constant", OP_NAME, NOSUPER, &type_op, 0);
nonzero_type = primitive("nonzero", OP_NAME, constant_type, &type_op, 0);
positive_type = primitive("positive", OP_NAME, nonzero_type, &type_op, 0);
literal_type = primitive("literal", OP_NAME, NOSUPER, &type_op, 0);
true_op = primitive("true", NULLARY, NOSUPER, &name_op, 0);
false_op = primitive("false", NULLARY, NOSUPER, &name_op, 0);

pnum_prim = primitive("positive constants", OP_NUM, positive_type, NULL, 0);
znum_prim = primitive("zero", OP_NUM, constant_type, NULL, 0);
nnum_prim = primitive("negative constants", OP_NUM, nonzero_type, NULL, 0);
str_prim = primitive("string constant", OP_STR, literal_type, NULL, 0);
undeclared_prim = primitive("?", OP_NAME, NOSUPER, NULL, 0);
untyped_prim = primitive("", OP_NAME, NOSUPER, NULL, 0);

/* machine primitives, to execute. */
/* All primitives are NULLARY, despite the fact that they take arguments. */
/* Note typical usage in bops. */
primitive("bind_primitive", NULLARY, NOSUPER, &name_op, 1);

primitive("addition_primitive", NULLARY, NOSUPER, &name_op, 16);
primitive("subtraction_primitive", NULLARY, NOSUPER, &name_op, 17);
primitive("multiplication_primitive", NULLARY, NOSUPER, &name_op, 18);
primitive("division_primitive", NULLARY, NOSUPER, &name_op, 19);
primitive("equality_primitive", NULLARY, NOSUPER, &name_op, 20);
primitive("lessthan_primitive", NULLARY, NOSUPER, &name_op, 21);
primitive("lessorequal_primitive", NULLARY, NOSUPER, &name_op, 22);
primitive("power_primitive", NULLARY, NOSUPER, &name_op, 23);
primitive("sin_primitive", NULLARY, NOSUPER, &name_op, 24);
primitive("cos_primitive", NULLARY, NOSUPER, &name_op, 25);
primitive("tan_primitive", NULLARY, NOSUPER, &name_op, 26);
primitive("atan_primitive", NULLARY, NOSUPER, &name_op, 27);
primitive("round_primitive", NULLARY, NOSUPER, &name_op, 28);
primitive("floor_primitive", NULLARY, NOSUPER, &name_op, 29);
primitive("lexcompare_primitive", NULLARY, NOSUPER, &name_op, 30);
primitive("trace_primitive", NULLARY, NOSUPER, &name_op, 31);

primitive("line_primitive", NULLARY, NOSUPER, &name_op, 40);
primitive("string_primitive", NULLARY, NOSUPER, &name_op, 41);

/* USER DEFINED PRIMITIVES GO HERE */
/* You might want to number your primitives starting with 64 */

/* End of user defined primitives */
}

/*************************************************************
 *
 *  Routines to execute machine primitives
 *
 *  These primitives make assumptions about their arguments,
 *  which are enforced in "bops".
 *  Changes to bops are at the user's peril!
 *
 * After the primitive executes, the expression ex will be freed.
 * Therefore, any parts of it that are used in the answer must be
 * copied.
 *
 *************************************************************/

NODE *
primitive_execute(which, ex)
short which;
NODE *ex;
{
NODE *node_new();	/* from expr.c */
char *arity_name();	/* from ops.c */
NODE *expr_copy();	/* from expr.c */
int name_compare();	/* from names.c */

/* Should be set if a variable gets bound. */
/* Causes all bound variables to be replaced by their value */
extern int bondage;	/* from match.c */

register TERM_NODE *tn = (TERM_NODE *) ex;
register NODE *answer = node_new();
double value;		/* numeric result */
OP *bresult;		/* boolean result */

answer->op = (OP *) NULL;
((TERM_NODE *)answer)->label = (NAME_NODE *) NULL;
((TERM_NODE *)answer)->right = (NODE *) NULL;
((TERM_NODE *)answer)->left = (NODE *) NULL;

switch(which) {
 case 1:		/* bind */
    answer->op = true_op;
    if (tn->left->op->arity != OP_NAME) {
	fprintf(stderr, "operator: %s, arity %s\n", tn->left->op->pname,
	    arity_name(tn->left->op->arity));
	error("attempt to bind a value to something other than a name");
	}
    if (((NAME_NODE *)(tn->left))->value) {
	fprintf(stderr, "variable: %s\n", ((NAME_NODE *)(tn->left))->pval);
	error("attempt to bind a value to an already bound variable");
	}
	if (name_in_expr(tn->right, (NAME_NODE *)tn->left)) {
    fprintf(stderr, "variable: %s; expr: ", ((NAME_NODE *)(tn->left))->pval);
    expr_print(tn->right);
	error("\nbound expression contains variable to which it is being bound");
	}
    ((NAME_NODE *)(tn->left))->value = expr_copy(tn->right);
    bondage = TRUE;	/* need to replace bound variable */
    break;
 case 16:		/* addition */
    ((NUM_NODE *)answer)->value = ((NUM_NODE *)(tn->left))->value +
	((NUM_NODE *)(tn->right))->value;
    break;
 case 17:		/* subtraction */
    ((NUM_NODE *)answer)->value = ((NUM_NODE *)(tn->left))->value -
	((NUM_NODE *)(tn->right))->value;
    break;
 case 18:		/* multiplication */
    ((NUM_NODE *)answer)->value = ((NUM_NODE *)tn->left)->value *
	((NUM_NODE *)tn->right)->value;
    break;
 case 19:		/* division */
    ((NUM_NODE *)answer)->value = ((NUM_NODE *)tn->left)->value /
	((NUM_NODE *)tn->right)->value;
    break;
 case 20:		/* numeric equality */
    answer->op = (((NUM_NODE *)tn->left)->value ==
	((NUM_NODE *)tn->right)->value) ? true_op : false_op ;
    break;
 case 21:		/* numeric less than */
    answer->op = (((NUM_NODE *)tn->left)->value <
	((NUM_NODE *)tn->right)->value) ? true_op : false_op ;
    break;
 case 22:		/* numeric less or equal */
    answer->op = (((NUM_NODE *)tn->left)->value <=
	((NUM_NODE *)tn->right)->value) ? true_op : false_op ;
    break;
 case 23:		/* raise to power */
    ((NUM_NODE *)answer)->value = pow(((NUM_NODE *)tn->left)->value,
	((NUM_NODE *)tn->right)->value);
    break;
 case 24:		/* sine */
    ((NUM_NODE *)answer)->value = sin(((NUM_NODE *)tn->right)->value);
    break;
 case 25:		/* cosine */
    ((NUM_NODE *)answer)->value = cos(((NUM_NODE *)tn->right)->value);
    break;
 case 26:		/* tangent */
    ((NUM_NODE *)answer)->value = tan(((NUM_NODE *)tn->right)->value);
    break;
 case 27:		/* arc tangent */
    ((NUM_NODE *)answer)->value = atan(((NUM_NODE *)tn->right)->value);
    break;
 case 28:		/* round to integer */
    ((NUM_NODE *)answer)->value = rint(((NUM_NODE *)tn->right)->value);
    break;
 case 29:		/* floor */
    ((NUM_NODE *)answer)->value = floor(((NUM_NODE *)tn->right)->value);
    break;
 case 30:		/* lexical comparison of variable names */
    ((NUM_NODE *)answer)->value = (double) name_compare(
	(NAME_NODE *)(tn->left), (NAME_NODE *)(tn->right));
    break;
 case 31:		/* trace */
    ((NUM_NODE *)answer)->value = verbose;
    verbose = (((NUM_NODE *)tn->right)->value);
    break;
 case 40:		/* draw a line */
    answer->op = true_op;
    draw_line(
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->left)->left)->value,
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->left)->right)->value,
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->right)->left)->value,
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->right)->right)->value);
    break;
 case 41:		/* draw a string centered at a location */
    answer->op = true_op;
    draw_string(
     ((STR_NODE *)((TERM_NODE *)tn->left)->left)->value,
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->right)->left)->value,
     ((NUM_NODE *)((TERM_NODE *)((TERM_NODE *)tn->left)->right)->right)->value);
    break;

/* USER DEFINED PRIMITIVES GO HERE */

/* End of user defined primitives */

 default:
    fprintf(stderr, "operator: %s (#%d)\n", ex->op->pname, ex->op->eval);
    error("invalid builtin function");
    }

/* at this point, the operator of a boolean answer is true or false */
/* the operator of a numeric answer depends on the sign of the answer */
/* If a answer->op has not been assigned, assume that the answer is */
/* a number, and set answer->op depending upon its sign. */
if (NULL == answer->op) {	/* if null, then must be numeric */
    if (((NUM_NODE *)answer)->value == 0.0) answer->op = znum_prim;
    else answer->op = (((NUM_NODE *)answer)->value > 0.0) ?
	pnum_prim : nnum_prim ;
    }

return answer;
}
