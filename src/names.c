/***********************************************************************
 *
 * Manage global hierarchical name space
 *
 **********************************************************************/

#include "def.h"

NAME_NODE *global_names;	/* root of global name space */

/***********************************************************************
 *
 * Put names into name space.
 * If name already exists, increment its reference count.
 * Name space is stored as a tree.
 *
 * entry:	name string
 *		parent name space, to insert into
 *		type of name (op field for node)
 *
 * exit:	return pointer to name node
 *
 **********************************************************************/
NODE *
name_put(name, space, type)
char *name;		/* identifier name */
NAME_NODE *space;	/* parent name space node */
OP *type;		/* type field for this name */
{
extern OP *undeclared_prim;	/* from primitive.c */
char *char_copy();		/* from util.c */
NODE *node_new();		/* from expr.c */

register NAME_NODE *curr;
register NAME_NODE *prev = NULL;
register NAME_NODE *nn;
register int val;

#ifdef DEBUG
printf("insert new name: %s'%s\n", name, type->pname);
#endif

if (!type) {	/* no type specified */
    fprintf(stderr, "name: %s\n", name);
    error("no type specified");
    }

if (!space) {	/* no space specified */
    fprintf(stderr, "name: %s\n", name);
    error("no name space specified");
    }
curr = space->child;

/* link into name space */
while(curr) {
    val = strcmp(curr->pval, name);
    if (val == 0) {		/* name already exists */
	curr->refs++;
	if (curr->op == undeclared_prim) curr->op = type;
	else if (type != undeclared_prim && curr->op != type) {
	    fprintf(stderr, "name: %s, types: %s & %s\n",
		name, curr->op->pname, type->pname);
	    error("name with two different types!");
	    }
	return (NODE *) curr;
	}
    if (val > 0) break;		/* insert here */
    /* otherwise, look at next node in list */
    prev = curr;
    curr = curr->next;
    }
nn = (NAME_NODE *) node_new();
nn->next = curr;
if (prev) prev->next = nn;
else space->child = nn;

nn->op = type;
nn->parent = space;
nn->child = (NAME_NODE *) NULL;
nn->pval = char_copy(name);
nn->refs = 2;		/* One reference to this name */
			/* plus parent reference */
nn->value = (NODE *) NULL;
nn->interest = 0;	/* Of no interest, yet */

return((NODE *) nn);
}

/***********************************************************************
 *
 * Return a fresh pointer to a name.
 * Doesn't actually make a copy of a name, but it does increment
 * the reference count field.
 *
 * entry:	pointer to name node to be copied.
 *
 * exit:	pointer to same node.
 *
 ******************************************************************/
NAME_NODE *
name_copy(on)
NAME_NODE *on;		/* old name */
{
on->refs++;
return on;
}

/***********************************************************************
 *
 * Remove a reference to a name.
 * Decrements the reference count field.
 * If zero, then actually deletes the name.
 *
 * entry:	pointer to name node to be freed.
 *
 ******************************************************************/
void
name_free(fn)
NAME_NODE *fn;		/* name to free */
{
void node_free();	/* from expr.c */
void expr_free();	/* from expr.c */
NAME_NODE *ch, *nch;	/* children */

if (0 == --(fn->refs)) {
    /* actually delete this node */
    for (ch = fn->child; ch; ch = nch) {	/* free children */
	nch = ch->next;
	name_free(ch);
	}
    if (fn->value) expr_free(fn->value);	/* free value */
    node_free((NODE *) fn);
    }
}

/***********************************************************************
 *
 * Insert one name space into another.
 * Used to instantiate a rule.
 *
 * entry: space to insert, and space to insert into
 *        if space to be inserted into is null,
 *        makes a copy of space to insert
 *
 ******************************************************************/
NAME_NODE *
name_space_insert(ins, space)
NAME_NODE *ins;		/* name space to be inserted */
NAME_NODE *space;	/* name space to insert into */
{
NODE *node_new();		/* from expr.c */
NODE *expr_copy();		/* from expr.c */
extern OP *undeclared_prim;	/* from primitive.c */
void name_print();		/* forward reference */
register NAME_NODE *in, *sn;
register NAME_NODE *pn = (NAME_NODE *) NULL;	/* previous */
int cmpval;			/* result of string comparison */

if (!ins) return space;
in = ins->child;
if (!space) {	/* create dummy parent */
    space = (NAME_NODE *) node_new();
    space->op = ins->op;
    space->next = (NAME_NODE *) NULL;
    space->parent = (NAME_NODE *) NULL;
    space->child = (NAME_NODE *) NULL;
    space->pval = ins->pval;
/*  space->value = ins->value; */
    space->value = (NODE *) NULL;
    space->refs = 1;
    space->interest = ins->interest;
    }
sn = space->child;

while (in) {	/* more nodes to insert */
    if (sn) cmpval = strcmp(sn->pval, in->pval);
    else cmpval = 1;
    if (cmpval < 0) {		/* skip node in space to insert into */
	pn = sn;
	sn = sn->next;
	}
    else if (cmpval > 0) {	/* insert here */
	if (in->op != undeclared_prim) {	/* parameter */
	    if (in->value->op->arity & OP_NAME)	/* set value fields */
		name_space_insert(in, in->value);
	    }
	else {		/* local variable */
	    register NAME_NODE *tn = name_space_insert(in, (NAME_NODE *) NULL);
	    tn->next = sn;
	    tn->parent = space;
	    in->value = (NODE *) tn;
	    if (pn) pn->next = tn;
	    else space->child = tn;
	    pn = tn;
	    }
	in = in->next;
	}
    else {			/* name exists in both spaces */
	if (in->op != undeclared_prim) {	/* parameter */
	    if (sn->value) {
		fprintf(stderr, "parameter: ");
		name_print(in);
		fprintf(stderr, ", bound variable: ");
		name_print(sn);
		fprintf(stderr, "\n");
		error("parameter has already been bound a value!");
		}
	    sn->value = expr_copy(in->value);
	    if (in->value->op->arity & OP_NAME)	/* set value fields */
		name_space_insert(in, in->value);
	    }
	else {
	    sn->child = name_space_insert(in, sn)->child;
	    in->value = (NODE *) sn;
	    pn = sn;
	    }
	in = in->next;
	sn = sn->next;
	}
    }
return space;
}

/***********************************************************************
 *
 * Print out names.
 * qname_print is a recursive utility function.
 * name_print is the main entry point.
 * name_space_print dumps the entire name space tree.
 *
 ******************************************************************/
void
qname_print(qn)
NAME_NODE *qn;		/* a single qualified name */
{
if (qn->parent) {
    qname_print(qn->parent);
    if (qn->parent->pval) fprintf(stderr, ".");
    }
if (qn->pval) fprintf(stderr, "%s", qn->pval);
}

void
name_print(fn)
NAME_NODE *fn;	/* a full name */
{
qname_print(fn);
if (fn->op->pname[0]) fprintf(stderr, "'%s", fn->op->pname);
}

name_space_print(ns)
NAME_NODE *ns;
{
void expr_print();	/* from expr.c */
register NAME_NODE *nn = ns;
while(nn) {
    if (nn->pval) fprintf(stderr, " %s", nn->pval);
    if (nn->op->pname[0]) fprintf(stderr, "'%s", nn->op->pname);
    if (nn->value) {
	fprintf(stderr, "=");
	expr_print(nn->value);
	}
    if (nn->child) {
	fprintf(stderr, "(");
	name_space_print(nn->child);
	fprintf(stderr, ")");
	}
    nn = nn->next;
    }
}

/***********************************************************************
 *
 * Compare two names "lexically".
 * Return 0 if they are identically the name name.
 *
 ******************************************************************/
int
name_compare(n1, n2)
NAME_NODE *n1, *n2;
{
if (n1 == n2) return 0;		/* same name */
/* TO DO: this should do something more intelligent */
if (n1 > n2) return 1;
else return -1;
}
