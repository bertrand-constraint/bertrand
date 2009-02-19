# Bertrand interpreter.

.PHONY : clean

# OPT = -O
OPT = -g

# Make sure the variable LIBDIR is set to where you want your
# include files to live (e.g., beep, bops and bag).

CFLAGS = $(OPT) -DLIBDIR='"libraries/"'

# NeWS cps graphics library.
# GRAPHLIB = -lm $(NEWSHOME)/lib/libcps.a
# GRAPHOBJ = graphics.o
GRAPHOBJ = graphicsnull.o

SRCS = expr.c names.c ops.c parse.c prep.c rules.c primitive.c\
	scanner.c main.c util.c match.c
OBJS = expr.o names.o ops.o parse.o prep.o rules.o primitive.o\
	scanner.o main.o util.o match.o

bert: $(OBJS) $(GRAPHOBJ)
	cc -lm $(OPT) -o bert $(OBJS) $(GRAPHOBJ) $(GRAPHLIB)

graphicsnull.o: graphicsnull.c
	cc $(CFLAGS) -c graphicsnull.c

graphics.o: graphics.c graphics.h
	cc $(CFLAGS) -c graphics.c

graphics.h: graphics.cps
	cps graphics.cps

clean:
	rm *.o || true
	rm bert || true
