Package:	language/bertrand

Description:	Bertrand

Version:	'88 Beta

Notes:

  Constraint lannguages represent a new programming paradigm with applications
  in such areas as the simulation of physical systems, computer-aided design,
  VLSI, graphics, and typesetting.  Constraint languages are declarative; a
  programmer specifies a desired goal, not a specific algorithm to accomplish
  that goal.  As a result, constraint programs are easy to build and modify,
  and their nonprocedural nature makes them amenable for execution on parallel
  processors.

  This book is aimed at researchers investigating declarative programming
  languages and rewrite rule systems, and engineers interested in building
  useful systems using constraint-satisfaction techniques.  It provides an
  introduction to the subject of constraint satisfaction, a survey of existing
  systems, and introduces a new technique that makes constraint-satisfaction
  systems easier to create and extend.  A general-purpose specification
  language called Bertrand is defined that allows users to describe a
  constraint-satisfaction system using rules.  This language uses a new
  inference mechanism called augmented term rewriting to execute the user's
  specification.  Bertrand supports a rule-based programming methodology, and
  also includes a form of abstract data type.  Using rules, a user can
  describe new objects and new constraint-satisfaction mechanisms.  This book
  shows how existing constraint-satisfaction systems can be implemented using
  Bertrand, and gives examples of how to use Bertrand to solve algebraic word
  and computer-engineering problems, and problems in graphics involving
  computer-aided design, illustration, and mapping.  It also gives a precise
  operational semantics for augmented term rewriting, and presents techniques
  for efficient execution, including interpretation using fast pattern
  matching, and compilation.

	Preface, Constraint Programming Languages
          Their Specification and Generation

Language(s):    C

Requirements:   None

Origin:

  Wm. Leler, wm@leler.com

See Also:	?

Restrictions:	?

References:

  Constraint Programming Languages
    Their Specification and Generation
  Wm. Leler
  Addison-Wesley, 1988, 0-201-06243-7

#-----------------------------------------------------------------------

Source for Bertrand '88 Beta distribution.

The documentation has been converted from troff to html. See
Reference.html and Guide.html.

The directory "libraries" contains the bertrand libraries, and
the directory "examples" contains a small number of examples.
Other examples can be typed in from the book.  I'd appreciate
copies of interesting programs that you write in Bertrand.

The file graphics.h was generated automatically from graphics.cps
by the program cps (which comes with the NeWS window system from
Sun Microsystems).  If you do not have NeWS, you will need to
rewrite graphics.c and ignore graphics.h and graphics.cps.  These
files are pretty simple, actually.

There is also graphicsnull.c, that simulates a graphics device
but just prints out everything rather than drawing it.

Have fun!
