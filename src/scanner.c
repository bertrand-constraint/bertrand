/******************************************************************
 *
 * scanner for a constraint satisfaction system
 * EOF must be -1 for character translation to work
 * Entry point is scan()
 *
 ******************************************************************/

#include "def.h"

/* global values returned by the scanner */
double token_val;		/* value of numeric token */
char token_prval[MAXTOKEN + 1];	/* print value of token */
OP *token_op;			/* operator node for operator token */

/* globals for printing informative error messages */
int lineno = 1;			/* current line number */
int charno = 0;			/* position in current line */

/* input file pointers */
FILE *infile;			/* default to standard input */
char *infilename;		/* input file name */

/* used for #include file stacks */
int filespushed = 0;		/* depth of #includes */
FILE *infiles[MAXFILES];	/* #include file pointers */
char *infilenames[MAXFILES];	/* #include file names */
int inlinenos[MAXFILES];	/* line numbers */
int verboses[MAXFILES];		/* verbose flags */

/* lists of user defined operators, from ops.c */
extern OP *single_op;		/* list of single-character operators */
extern OP *double_op;		/* list of two character operators */
extern OP *name_op;		/* list of alphanumeric operators */
extern OP *type_op;		/* list of types */

/* handle preprocessor statements */
extern void preprocess();	/* from prep.c */
extern void char_free();	/* from util.c */

/*  Character input class translations:	*/
/*  These definitions are in def.h */
/* C_EOF	0	eof, null			*/
/* C_CTRL	1	(invalid) control character	*/
/* C_NL		2	lf, cr, ff (lineno++)		*/
/* C_WS		3	blank, tab (whitespace)		*/
/* C_SPC	4	special character		*/
/* C_NUM	5	numeric 0-9			*/
/* C_ALPH	6	alphabetic a-Z, underscore _	*/
/* C_PER	7	period .			*/
/* C_DQ		8	double quote " (string)		*/
/* C_BQ		9	back quote ` (string escape)	*/
/* C_SQ		10	single quote ' (type)		*/
/* C_BRC	11	braces { } (rule body)		*/
/* C_LB		12	pound sign # (preprocessor)	*/
/*  special characters are all symbols such as + * & , etc. */
static char transtab[] =
/*    -1        0       1       2       3       4       5       6       7   */
    {C_EOF, C_EOF, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL,
/* 0x08 */ C_CTRL,   C_WS,   C_NL, C_CTRL,   C_NL,   C_NL, C_CTRL, C_CTRL,
/* 0x10 */ C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL,
/* 0x18 */ C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL, C_CTRL,
/* 0x20 */   C_WS,  C_SPC,   C_DQ,   C_LB,  C_SPC,  C_SPC,  C_SPC,   C_SQ,
/* 0x28 */  C_SPC,  C_SPC,  C_SPC,  C_SPC,  C_SPC,  C_SPC,  C_PER,  C_SPC,
/* 0x30 */  C_NUM,  C_NUM,  C_NUM,  C_NUM,  C_NUM,  C_NUM,  C_NUM,  C_NUM,
/* 0x38 */  C_NUM,  C_NUM,  C_SPC,  C_SPC,  C_SPC,  C_SPC,  C_SPC,  C_SPC,
/* 0x40 */  C_SPC, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x48 */ C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x50 */ C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x58 */ C_ALPH, C_ALPH, C_ALPH,  C_SPC,  C_SPC,  C_SPC,  C_SPC, C_ALPH,
/* 0x60 */   C_BQ, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x68 */ C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x70 */ C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH, C_ALPH,
/* 0x78 */ C_ALPH, C_ALPH, C_ALPH,  C_BRC,  C_SPC,  C_BRC,  C_SPC, C_CTRL};
/* trans[c] is class of input character c */
char *trans = transtab + 1;	/* trans[-1] == transtab[0] */

/* Finite state machine states: 	*/
#define ST  0	/* start		*/
#define SP  1	/* special		*/
#define S2  2	/* second special	*/
#define SL  3	/* leading period	*/
#define SN  4	/* number		*/
#define SF  5	/* fraction of number	*/
#define SI  6	/* identifier		*/
#define SS  7	/* string		*/
#define SE  8	/* string escape	*/
#define SC  9	/* comment		*/
#define SX 10	/* preprocessor syntaX	*/

#define TERM_STATE 50	/* start of terminal states */
/* temporary terminal states */
#define XR 51	/* Reserved char { } .	*/
/* scanner input errors */
#define EC 55	/* Character error	*/
#define EN 56	/* Number error		*/
#define ES 57	/* String error		*/
/* terminal states */
#define TI 61	/* Identifier		*/
#define TN 62	/* Number		*/
#define TO 63	/* single char Operator	*/
#define T2 64	/* 2 char operator	*/
#define TS 65	/* String		*/
static char stab[11][13] = {	/* state table	*/
/*0   1   2   3   4   5   6   7   8   9  10  11  12
EOF CTL  `n  SP   *   0   a   .   "   `  '   {}  #
         `f  `t   +   9   Z              		*/
{ST, EC, ST, ST, SP, SN, SI, SL, SS, SI, SI, XR, SX},  /* STart */
{TO, TO, TO, TO, S2, TO, TO, TO, TO, TO, TO, TO, TO},  /* SPecial */
{T2, T2, T2, T2, T2, T2, T2, T2, T2, T2, T2, T2, T2},  /* S 2 special */
{XR, XR, XR, XR, XR, SF, XR, SC, XR, XR, XR, XR, XR},  /* S Leading decimal */
{TN, TN, TN, TN, TN, SN, EN, SF, TN, TN, TN, TN, TN},  /* S Number */
{TN, TN, TN, TN, TN, SF, EN, EN, TN, TN, TN, TN, TN},  /* S Fraction */
{TI, TI, TI, TI, TI, SI, SI, TI, TI, TI, TI, TI, TI},  /* S Ident */
{ES, SS, ES, SS, SS, SS, SS, SS, TS, SE, SS, SS, SS},  /* S String */
{ES, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS},  /* S Escape */
{ST, SC, ST, SC, SC, SC, SC, SC, SC, SC, SC, SC, SC},  /* S Comment */
{ST, ST, ST, SX, SX, SX, SX, SX, SX, SX, SX, SX, SX}}; /* SyntaX */

/* action table */
#define AU 0	/* unget */
#define AE 1	/* eat character */
#define AA 2	/* add to symbol */
#define AN 3	/* add numeric */
#define AF 4	/* add decimal fraction */
#define AX 5	/* add escape */
#define AS 6	/* check special */
#define AR 7	/* Restart */
#define AP 8	/* preprocessor */
#define AC 9	/* check end of file */
static char atab[11][13] = {	/* action table */
/*0   1   2   3   4   5   6   7   8   9  10  11  12
EOF CTL  `n  SP   *   0   a   .   "   `  '   {}  #
         `f  `t   +   9   Z				*/
{AC, AA, AE, AE, AA, AN, AA, AA, AE, AA, AA, AA, AE},  /* STart */
{AU, AU, AE, AE, AS, AU, AU, AU, AU, AU, AU, AU, AU},  /* SPecial */
{AU, AU, AE, AE, AU, AU, AU, AU, AU, AU, AU, AU, AU},  /* S 2 special */
{AU, AU, AE, AE, AU, AF, AU, AE, AU, AU, AU, AU, AU},  /* S Leading decimal */
{AU, AU, AE, AE, AU, AN, AA, AA, AU, AU, AU, AU, AU},  /* S Number */
{AU, AU, AE, AE, AU, AF, AA, AA, AU, AU, AU, AU, AU},  /* S Fraction */
{AU, AU, AE, AE, AU, AA, AA, AU, AU, AU, AU, AU, AU},  /* S Ident */
{AU, AA, AU, AA, AA, AA, AA, AA, AE, AE, AA, AA, AA},  /* S String */
{AU, AX, AE, AX, AX, AX, AX, AX, AA, AA, AX, AX, AX},  /* S Escape */
{AR, AE, AR, AE, AE, AE, AE, AE, AE, AE, AE, AE, AE},  /* S Comment */
{AP, AP, AP, AA, AA, AA, AA, AA, AA, AA, AA, AA, AA}}; /* SyntaX */

/*****************************************************************
 * Finite state automaton scanner.  Reads infile.
 *
 * exit:	returns token type (defined in def.h)
 *		sets global values:
 *		token_prval - print name of token
 *		token_val - if token is of type NUMBER
 *		token_op - if token is of type OPER
 *
 *****************************************************************/
int
scan()
{

/* static variables, initial values used only first time scanner called */
static int class = C_NL;	/* input char class, initially newline */
static int c = '\n';		/* last character read */

/* these variables are reinitialized every time scanner is called */
register int state = ST;	/* current state, initially start */
register int tlength = 0;	/* length of token, initially zero */
register double fvalue = 0.0;	/* floating point value */
register double place = 0.1;	/* place after decimal point */

#ifdef DEBUG
printf("enter scanner: state %d, class %d, char '%c'\n", state, class, c);
fflush(stdout);
#endif

/* states >= TERM_STATE mean that a token has been recognized */
while (state < TERM_STATE) {
#ifdef DEBUG
printf("scanner: action %d,", atab[state][class]);
fflush(stdout);
#endif

   switch(atab[state][class]) {	/* perform action */
	case AA: L1: token_prval[tlength++] = c;	/* add */
		 if (tlength>MAXTOKEN) tlength = MAXTOKEN;
		 /* fall through */
	case AE: c = getc(infile);			/* eat */
		 if (C_NL==trans[c]) { lineno++; charno = 0; }
		 else charno++;
		 /* fall through */
	case AU: break;			/* do nothing for unget */
	case AN: fvalue = (fvalue * 10) + (c-'0');	/* numeric */
		 goto L1;
	case AF: fvalue += (c-'0') * place;		/* decimal fraction */
		 place /= 10;
		 goto L1;
	case AX: switch(c) {				/* escape */
		    case 'n': c = '\n'; break;	/* newline */
		    case 't': c = '\t'; break;	/* tab */
		    case 'b': c = '\b'; break;	/* backspace */
		    case 'r': c = '\r'; break;	/* return */
		    case 'f': c = '\f'; break;	/* formfeed */
		    default:
			fprintf(stderr, "character '%c':\n", c);
			error("illegal string escape");
		    } /* end switch */
		 goto L1;	/* add character */
	case AS:		/* check for special operator */
	    /* see if *token_prval and c form a double operator */
	    for (token_op = double_op; token_op; token_op = token_op->next) {
	    	if (*token_prval == token_op->pname[0] && 
		  c == token_op->pname[1]) 
		    goto L1;	/* found, do add action */
	        }
	    /* if not double op, *token_prval must be single char oper */
	    token_prval[1] = '\0';
#	    ifdef DEBUG
	    printf("\tchange state to %d\n", TO);
	    fflush(stdout);
#	    endif
	    goto L2;		/* exit to terminal state TO */
	case AP:		/* interpret preprocessor statement */
	    token_prval[tlength++] = '\n';
	    token_prval[tlength] = '\0';
	    preprocess();
	    /* fall through */
	case AR:		/* restart, throw away token */
	    tlength = 0;
	    break;		/* unget */
	case AC:		/* end of file */
	    if (filespushed) {
		fclose(infile);
		filespushed--;
		char_free(infilename);	/* free character string */
		infile = infiles[filespushed];
		infilename = infilenames[filespushed];
		lineno = inlinenos[filespushed];
		verbose = verboses[filespushed];
		c = '\n';
		charno = 0;
#		ifdef DEBUG
		printf("back to file %s, line %d\n", infilename, lineno);
#		endif
		}
	    else {
		class = C_NL;
		c = '\n';
		return EOF;
		}

     } /* end switch */

   state = stab[state][class];	/* calculate new state */
   class = trans[c];
#ifdef DEBUG
printf(" new state %d,", state);
if (c != EOF) printf(" next char '%c',", c);
else printf(" next char 'EOF',");
printf(" next class %d\n", class);
fflush(stdout);
#endif
  }	/* if you leave this loop, you have a token */

token_prval[tlength] = '\0';	/* null terminate token print name */

#ifdef DEBUG
printf("terminal state %d, token is '%s'\n", state, token_prval);
fflush(stdout);
#endif

if (tlength > MAXTOKEN) switch(state) {
	case TS: error("string too long");
	case TI: error("name too long");
	case TN: error("number too big");
	default: error("object too long");
	}

switch(state) {
 case TI:	/* identifier (could be a name operator) */
	if (*token_prval == '\'') {	/* a type */
	    for (token_op = type_op; token_op; token_op = token_op->next) {
		if (strcmp(token_prval+1, token_op->pname) == 0) return TYPE;
		}
	    fprintf(stderr, "type name %s not declared\n", token_prval);
	    error("invalid type");
	    }
	for (token_op = name_op; token_op; token_op = token_op->next) {
	    if (strcmp(token_prval, token_op->pname) == 0) return OPER;
	    }
	/* otherwise */ return IDENT;

 case TO:	/* single character operator */
	L2:	/* came from AS */
	for (token_op = single_op; token_op; token_op = token_op->next) {
	   if (*token_prval == token_op->pname[0]) return OPER;
	   }
	/* error, special char that is not an operator */
	fprintf(stderr, "character is: '%c'\n", *token_prval);
	error("invalid character");
 case T2:	/* two character operator */
	/* token_op was set in AS above */
	return OPER;
 case TN:	/* number */
	token_val = fvalue;
	return NUMBER;
 case TS:	/* string */
	return STRING;
 case XR: 	/* reserved character . { } */
	return (int) *token_prval;
 case EC:
	error("illegal character");
 case EN:
	error("mis-formed number (or possibly name)");
 case ES:
	error("unterminated string");
	}
}
