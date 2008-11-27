/********************************************************************
 *
 * Interpret proprocessor statements.
 * Initialize operator tables for scanner and parser.				
 *
 * Each statement begins with a pound sign (#) in column one,
 * and runs to the end of the line.  Comments are allowed.
 *
 * Valid statements are:
 * #op		operator definition
 * #operator	   "         "
 * #type	type definition
 * #primtive	add supertype information
 * #include	cause scanner to start reading from another file
 * #load	like include, but file has been compiled (not implemented)
 * #line	change line number for error messages
 * #trace	set tracing level
 * #quiet	turn all tracing off
 *
 * Other statements, including other C preprocessor statements,
 * may be added in the future.
 *
 * Entry point is function "preprocess".
 *
 ********************************************************************/

#include "def.h"

extern char token_prval[];	/* preprocessor statement from scanner */
static int token_pos;		/* position in statement */

/* Character input class translation */
extern char *trans;	/* from scanner.c */

/********************************************************************
 *
 * token_get:	Return next token from string.
 *
 * entry:	Accesses the global variable token_prval
 *		(from scanner.c) which it modifies.  Also
 *		uses the static variable token_pos, which
 *		should have been initialized to zero before
 *		the first call to token_get.
 *
 * exit:	Returns a pointer to the next token in token_prval.
 *		This token is null terminated by inserting a
 *		null into token_prval.
 *		Returns null if there are no more tokens.
 *
 * A token is defined as either a whitespace delimited string of
 * characters, or an arbitrary string of characters inside of double
 * quotes.  No string escapes are currently recognized inside of
 * strings, so, for example, it is impossible to have a token contain
 * both a blank and a double quote.  A comment begins with two
 * periods, and runs to the end of the line.
 *
 ********************************************************************/
static char *
token_get()
{
register char *pos = token_prval + token_pos;
char *tpos = NULL;		/* starting position of token */

while (C_WS == trans[*pos]) pos++;	/* skip whitespace */
/* if comment, then no more tokens */
if (C_PER == trans[*pos] && C_PER == trans[pos[1]]) return NULL;
if (C_EOF == trans[*pos] || C_NL == trans[*pos]) return NULL;
else if (C_DQ == trans[*pos]) {	/* quoted token */
    tpos = ++pos;		/* skip quote */
    while (C_DQ != trans[*pos] && C_NL != trans[*pos]) pos++;
    if (C_NL == trans[*pos]) error("unterminated string");
    *pos++ = '\0';	/* null terminate token (overwrite quote) */
    }
else {			/* unquoted token */
    tpos = pos++;	/* address of token */
    while (C_WS != trans[*pos] && C_NL != trans[*pos]) pos++;
    *pos++ = '\0';	/* null terminate token */
    }
token_pos = pos - token_prval;
return tpos;
}

/********************************************************************
 *
 * Operator definitions.
 *
 * In an operator definition the following keywords are recognized:
 *
 * unary		unary (default prefix)
 * prefix		unary prefix
 * postfix		unary postfix
 * infix		binary infix (default nonassociative)
 * binary		binary infix (default nonassociative)
 * left			binary infix left associative
 * right		binary infix right associative
 * nonassoc		binary infix nonassociative
 * nonassociative	binary infix nonassociative
 * non			binary infix nonassociative
 * associative		no-op
 * outfix		unary outfix
 * matchfix		unary outfix
 * nullary		nullary
 * precedence		no-op
 * supertype		no-op
 * 
 * The default arity is nullary.
 * A number is interpreted as a precedence (default 0).
 * Operator names are any non-keyword name, or one or two special characters.
 * Outfix takes two operators.
 * An operator may optionally have a single supertype.
 * Nullary and outfix do not take a precedence.
 * A numeric field beginning with a pound sign (#) indicates a special
 * operator.  Some special operators are evaluated at parse time
 * (such as ":" for labels, or "()" for changing evaluation order).
 * Other special operators control evaluation at run time
 * (such as [ ], to prevent evaluation).
 *
 * Examples:
 * #op + binary left associative precedence 45 ... standard plus operator
 * #op - left 45	... identical definition for minus
 * #op true		... nullary and precedence=0 assumed
 * #op false nullary supertype 'boolean	 ... supertype
 * #operator ( ) #1	... outfix, parser reduce function number 1
 *
 ********************************************************************/

#define DEFAULT_PREC 0

/* lists of operator definitions, from ops.c */
extern OP *single_op, *double_op, *name_op, *type_op;

/********************************************************************
 * 
 * op_create:	Allocate operator and check for valid name.
 *
 * entry:	The operator name to be checked.
 *		Pass in arity of this operator
 *
 * exit:	Operator node, inserted into operator tables.
 *		Arity and pname fields filled in.
 *		Returns pointer to node.
 *		Error exit if invalid name, or too many symbols.
 *
 * A valid operator is either an alphanumeric name or a symbol.
 * Symbols can be one or two characters long.
 *
 ********************************************************************/
static OP *
op_create(opn, arity)
char *opn;	/* operator name */
short arity;
{
register int l;		/* length of name */
int class;
OP *op;
#ifndef __STDC__
OP *op_new();		/* from ops.c */
void op_put();		/* from ops.c */
#endif

class = trans[opn[0]];
if (C_ALPH == class) {		/* alphanumeric operator */
    for (l = 1; C_EOF != (class = trans[opn[l]]); l++) {
	if (C_ALPH != class && C_NUM != class) {
	    fprintf(stderr, "operator: %s\n", opn);
	    error("invalid alphanumeric operator name");
	    }
	}
    op = op_new(l);		/* allocate operator node */
    strcpy(op->pname, opn);	/* copy operator name */
    op->arity = arity;
    op_put(&name_op, op);	/* insert into linked list */
    }
else if (C_SPC == class) {
    class = trans[opn[1]];
    if (C_EOF == class) {	/* single special char operator */
	op = op_new(1);		/* allocate operator node */
	strcpy(op->pname, opn);	/* copy operator name */
	op->arity = arity;
	op_put(&single_op, op);	/* insert into linked list */
	}
    else if (C_SPC == class) {
	class = trans[opn[2]];
	if (C_EOF == class) {
	    op = op_new(2);		/* allocate operator node */
	    strcpy(op->pname, opn);	/* copy operator name */
	    op->arity = arity;
	    op_put(&double_op, op);	/* insert into linked list */
	    }
	else {
	    fprintf(stderr, "operator: %s\n", opn);
	    error("operator name contains more than two symbols");
	    }
	}
    else {
	fprintf(stderr, "operator: %s\n", opn);
	error("invalid symbol in operator name");
	}
    }
else {
    fprintf(stderr, "operator: %s\n", opn);
    error("invalid operator name, or misspelled keyword");
    }
return op;
}

/********************************************************************
 *
 * Convert precedence of operator to numberic value.
 * This would be a generic routine for converting strings to numbers,
 * except that it checks for a valid precedence.
 *
 * entry:	string containing precedence.
 *
 * exit:	numeric value of precedence.
 *
 ********************************************************************/
static short
prec_conv(s) 
char *s;			/* input string */
{
long prec = 0;			/* precedence */
int i = 0;			/* position in string */

while (C_NUM == trans[s[i]]) prec = 10 * prec + s[i++] - '0';

if (C_EOF != trans[s[i]]) {
    fprintf(stderr,"field: %s\n", s);
    error("invalid precedence field");
    }

if (prec > BIG_SHORT) {
    printf("Precedence = %d; Maximum allowable = %ld\n", prec, BIG_SHORT);
    error("Precedence of operator exceeds maximum allowable");
    }

return (short) prec;
}

/********************************************************************
 *
 * op_define:	Read and interpret an operator definition.
 *
 ********************************************************************/
static void
op_define()
{
register char *tok;	/* current token */
char *op_name = NULL;	/* defined operator name */
char *other_op_name = NULL;	/* outfix requires two operator names */
char *supertype = NULL;
char *prfun = NULL;	/* parser reduce function number */
short arity = -1;	/* number of arguments (see def.h) */
short kind = -1;	/* i.e., postfix vs. prefix */
short precedence = -1;
OP *op;
char *arity_name();	/* from ops.c */

for ( ; NULL != (tok = token_get()); ) {	/* do until end of statement */
    if (tok[0] == '#') {	/* special information for operator */
	if (prfun) error("multiple special reduce functions not allowed");
	prfun = tok;
	}
    else if (C_NUM == trans[tok[0]]) {	/* precedence */
	precedence = prec_conv(tok);
	}
    else if (C_SQ == trans[tok[0]]) {	/* supertype */
	if (supertype) {
	    fprintf(stderr, "first supertype: %s, second supertype: %s\n",
		supertype, tok);
	    error("multiple supertypes not allowed");
	    }
	supertype = tok;
	}
    else if (0==strcmp(tok, "left")) {
	if (arity == -1 || arity == BINARY) arity = LEFT;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "right")) {
	if (arity == -1 || arity == BINARY) arity = RIGHT;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "prefix")) {
	if (arity == -1 || arity == UNARY) arity = PREFIX;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "postfix")) {
	if (arity == -1 || arity == UNARY) arity = POSTFIX;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "nullary")) {
	if (arity == -1) arity = NULLARY;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "nonassoc") ||
	0==strcmp(tok, "nonassociative") ||
	0==strcmp(tok, "non")) {
	if (arity == -1 || arity == BINARY) arity = NONASSOC;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "outfix") || 0==strcmp(tok, "matchfix")) {
	if (arity == -1 || arity == UNARY) arity = OUTFIX1;
	else {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "unary")) {
	if (arity == -1) arity = UNARY;
	else if (!(arity & UNARY)) {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "infix") || 0==strcmp(tok, "binary")) {
	if (arity == -1) arity = BINARY;
	else if (!(arity & BINARY)) {
	    fprintf(stderr, "keyword %s applied to op of arity %s\n",
		tok, arity_name(arity));
	    error("bad operator specification");
	    }
	}
    else if (0==strcmp(tok, "associative")) ; 	/* no-op */
    else if (0==strcmp(tok, "precedence")) ;	/* no-op */
    else if (0==strcmp(tok, "supertype")) ;	/* no-op */
    else {		/* operator name */
	if (!op_name) op_name = tok;
	else if (!other_op_name) other_op_name = tok;
	else {
	    fprintf(stderr, "operators: %s, %s, %s\n",
		op_name, other_op_name, tok);
	    error("too many operator names, or invalid keyword");
	    }
	}
    }	/* end of for loop */
/* now sort everything out */
if (arity == -1) {	/* arity not specified */
    if (!other_op_name) arity = NULLARY;	/* default is nullary */
    else arity = OUTFIX1;	/* unless two operators */
    }
else if (arity == UNARY) arity = PREFIX;
else if (arity == BINARY) arity = NONASSOC;
/* validity checks */
if (precedence != -1 && arity == NULLARY) {
    fprintf(stderr, "operator: %s, precedence: %d\n", op_name, precedence);
    error("a nullary operator may not have a precedence");
    }
if (precedence != -1 && arity == OUTFIX1) {
    fprintf(stderr, "operators: %s %s, precedence: %d\n",
	op_name, other_op_name, precedence);
    error("an outfix operator may not have a precedence");
    }
/* check operator names */
if (op_name) op = op_create(op_name, arity);
else {
    error("no operator name specified in operator definition");
    }
if (arity == OUTFIX1) {
    if (other_op_name) {
	op->other = op_create(other_op_name, OUTFIX2);
	op->other->other = op;	/* link outfix2 to outfix1 */
	}
    else {
	fprintf(stderr, "first operator: %s\n", op_name);
	error("outfix operator requires two operator names");
	}
    }
else if (other_op_name) {	/* only single name should be specified */
    fprintf(stderr, "operator: %s, second operator: %s\n",
	op_name, other_op_name);
    error("multiple operator names defined, or invalid keyword");
    }
if (arity == NULLARY) op->precedence = BIG_SHORT;
else if (arity == OUTFIX1) op->precedence = 0;
else if (precedence == -1) op->precedence = DEFAULT_PREC;
else op->precedence = precedence;
if (supertype) {
    OP *sop;
    for (sop = type_op; sop; sop = sop->next) {
	if (0==strcmp(sop->pname, supertype+1)) break;
	}
    if (sop) op->super = sop;
    else {
	fprintf(stderr,"type: %s\n", supertype);
	error("supertype is invalid type");
	}
    }
if (prfun) {
    int snum;
    snum = atoi(prfun+1);
    if (snum == 0) error("invalid parser reduce function");
    else op->eval = -snum;
    }
#ifdef DEBUG
printf("operator %s defined, arity = %s, precedence = %d\n",
    op->pname, arity_name(op->arity), op->precedence);
if (op->arity == OUTFIX1)
    printf("other operator %s defined, arity = %s, precedence = %d\n",
    op->other->pname, arity_name(op->other->arity), op->other->precedence);
#endif
}

/********************************************************************
 *
 * Type definitions.
 *
 * A type definition is just one or two type names.
 * The second type name is a supertype.
 * The keyword "supertype" is optional.
 *
 * Examples:
 * #type 'line
 * #type 'point supertype 'line
 * #type 'even 'integer
 *
 ********************************************************************/
static void
type_define()
{
register OP *ty;
register char *tok;
int len;
OP *sop;
#ifndef __STDC__
OP *op_new();		/* from ops.c */
#endif

tok = token_get();
if (tok[0] != '\'') {
    fprintf(stderr,"type: %s\n", tok);
    error("type must begin with a single quote");
    }
if (tok[1] == '\0') error("cannot define null type");
len = strlen(tok);
ty = op_new(len-1);	/* skip single quote */
strcpy(ty->pname, tok+1);
ty->arity = OP_NAME;
ty->precedence = 0;
ty->other = (OP *) NULL;
ty->super = (OP *) NULL;
op_put(&type_op, ty);

tok = token_get();
if (tok) {	/* supertype */
    if (0==strcmp(tok, "supertype")) {
	tok = token_get();
	if (!tok) {
	    error("no supertype following supertype keyword");
	    }
	}
    if (tok[0] != '\'') {
	fprintf(stderr,"supertype: %s\n", tok);
	error("supertype must begin with a single quote");
	}
    for (sop = type_op; sop; sop = sop->next) {
	if (0==strcmp(sop->pname, tok+1)) break;
	}
    if (sop) ty->super = sop;
    else {
	fprintf(stderr,"type: %s\n", tok);
	error("supertype is invalid type");
	}
    }
tok = token_get();
if (tok) error("invalid type definition");
#ifdef DEBUG
printf("type '%s defined, arity = %s", ty->pname, arity_name(ty->arity));
if (ty->super) printf(", supertype = '%s\n", ty->super->pname);
else printf("\n");
#endif
}

/********************************************************************
 *
 * Primitive definitions.
 *
 * Similar to a type definition, except the type or operator already
 * exists (typically it is a primitive to the system).
 * A primitive definition is just a type or operator name,
 * followed by a supertype name (the keyword "supertype" is optional).
 * There is no point to a primitive definition without a supertype.
 * This directive can also be used to add supertype information to an
 * operator or type declared previously.
 *
 * Examples:
 * #primitive 'constant supertype 'number
 * #primitive 'positive 'constant
 *
 ********************************************************************/
static void
primitive_define()
{
register OP *prim;	/* the primitive */
register char *tok;	/* tokens from statement */
register OP *sop;	/* supertype */

tok = token_get();
if (tok[0] == '\'') {
    for (prim = type_op; prim; prim = prim->next) {
	if (0==strcmp(prim->pname, tok+1)) break;
	}
    if (!prim) {
	fprintf(stderr, "primitive type: %s\n", tok);
	error("primitive not found");
	}
    }
else if (C_ALPH == trans[tok[0]]) {
    for (prim = name_op; prim; prim = prim->next) {
	if (0==strcmp(prim->pname, tok)) break;
	}
    if (!prim) {
	fprintf(stderr, "alphanumeric primitive: %s\n", tok);
	error("primitive not found");
	}
    }
else if (tok[1] == '\0') {
    for (prim = single_op; prim; prim = prim->next) {
	if (0==strcmp(prim->pname, tok)) break;
	}
    if (!prim) {
	fprintf(stderr, "special character primitive: %s\n", tok);
	error("primitive not found");
	}
    }
else if (tok[2] == '\0') {
    for (prim = double_op; prim; prim = prim->next) {
	if (0==strcmp(prim->pname, tok)) break;
	}
    if (!prim) {
	fprintf(stderr, "double special character primitive: %s\n", tok);
	error("primitive not found");
	}
    }
else {
    fprintf(stderr, "unknown primitive: %s\n", tok);
    error("primitive not found");
    }
if (prim->super) {
    fprintf(stderr, "primitive: %s, supertype: %s\n", tok, prim->super->pname);
    error("primitive already has a supertype");
    }

/* look for supertype */
tok = token_get();
if (tok) {	/* supertype */
    if (0==strcmp(tok, "supertype")) {
	tok = token_get();
	if (!tok) {
	    error("no supertype following supertype keyword");
	    }
	}
    if (tok[0] != '\'') {
	fprintf(stderr,"supertype: %s\n", tok);
	error("supertype must begin with a single quote");
	}
    for (sop = type_op; sop; sop = sop->next) {
	if (0==strcmp(sop->pname, tok+1)) break;
	}
    if (sop) prim->super = sop;
    else {
	fprintf(stderr,"type: %s, supertype: %s\n", tok, prim->super->pname);
	error("supertype is invalid type");
	}
    }
else {
    fprintf(stderr, "primitive: %s\n", tok);
    error("no supertype specified for primitive");
    }
tok = token_get();
if (tok) error("invalid primitive definition");
#ifdef DEBUG
printf("primitive %s defined, arity = %s", ty->pname, arity_name(ty->arity));
if (ty->super) printf(", supertype = '%s\n", ty->super->pname);
else printf("\n");
#endif
}

/********************************************************************
 *
 * Include and load statements.
 *
 * An #include causes the scanner to textually include the specified
 * file.  Includes can be nested.
 * A #load is similar to an include, except that the file to be loaded
 * has already been precompiled.  NOT IMPLEMENTED YET.
 * Included files are searched for first in the current directory,
 * then in the directory called libdir, which defaults to the value
 * LIBDIR (#defined in def.h) or the UNIX environment variable BERTRAND,
 * if it is defined.
 * If a file name contains spaces, it should be enclosed in double quotes.
 * A file name inside of double quotes may not contain any double quotes.
 *
 * Examples (assuming UNIX file conventions):
 * #include thisfile
 * #include dir1/dir2/file
 * #include /root/dir/file
 * #include "a file"	.. not a typical UNIX file name
 *
 ********************************************************************/
void
file_push()
{
extern int filespushed;		/* all from scanner.c */
extern FILE *infiles[];
extern FILE *infile;
extern char *infilenames[];
extern char *infilename;
extern int inlinenos[];
extern int verboses[];
extern int lineno;
char *tok;
char *char_copy();		/* from util.c */
extern char *libdir;		/* from main.c */

tok = token_get();
if (!tok) error("no include file name specified");
infiles[filespushed] = infile;
infilenames[filespushed] = infilename;
inlinenos[filespushed] = lineno;
verboses[filespushed] = verbose;

const int max_filename_size = 256;
char loadable_in_libraries[max_filename_size];
sprintf(loadable_in_libraries, "libraries/%s", tok);
char loadable_in_libdir[max_filename_size];
sprintf(loadable_in_libdir, "%s/%s", libdir, tok);
const char *loadables[] = {
    tok,
    loadable_in_libraries,
    loadable_in_libdir,
};
const int loadables_size = 3;

int loaded = 0;
char *loadable;
int loadable_i;
for (loadable_i = 0; loadable_i < loadables_size; ++loadable_i) {
    // TODO If verbose, print message
    // printf("trying %s\n", loadables[loadable_i]);
    loadable = loadables[loadable_i];
    infile = fopen(loadable, "r");
    if (NULL == infile) {
        // TODO If verbose, print message
    } else {
        loaded = 1;
        break;
    }
}

if (0 == loaded) {
    fprintf(stderr, "include file: %s\n", tok);
    error("file not found");
}
infilename = char_copy(tok);
verbose = FALSE;
lineno = 1;
filespushed++;
#ifdef DEBUG
printf("now reading from file %s\n", infilename);
#endif
}


void
load_file()
{
error("#load not implemented");
}

/********************************************************************
 *
 * preprocess:  Interpret preprocessor statements.
 *
 * entry:	String containing preprocessor statement is global
 *		variable token_prval from scanner.
 *
 * exit:	Side effects, depending upon type of statement.
 *		Error exit if invalid statement, syntax error, etc.
 *
 ********************************************************************/
void
preprocess()
{
char *tok;
extern int lineno;	/* from scanner.c */

token_pos = 0;
tok = token_get();
if (!tok) return;	/* null statement, ignore */
if (0 == strcmp(tok, "op") || 0 == strcmp(tok, "operator")) op_define();
else if (0 == strcmp(tok, "type")) type_define();
else if (0 == strcmp(tok, "primitive")) primitive_define();
else if (0 == strcmp(tok, "include")) file_push();
else if (0 == strcmp(tok, "load")) load_file();
else if (0 == strcmp(tok, "line")) {
    tok = token_get();
    if (tok) {
	if (C_NUM == trans[tok[0]]) lineno = atoi(tok);
	}
    }
else if (0 == strcmp(tok, "trace")) {
    tok = token_get();
    if (tok && (C_NUM == trans[tok[0]])) verbose = atoi(tok);
    else verbose = 1;
    }
else if (0 == strcmp(tok, "quiet")) verbose = 0;
else {
    fprintf(stderr, "preprocessor statement keyword: #%s\n", tok);
    error("invalid preprocessor statement");
    }
}
