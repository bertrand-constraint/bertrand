/*********************************************************
 *
 * Graphics routines for the NeWS window system.
 *
 * The routines snews_setup, snews_line, and snews_string are defined
 * in graphics.cps, which gets converted into graphics.h by cps.
 * The routines ps_open_PostScript, ps_flush_PostScript,
 * ps_close_PostScript, and PostScriptInput are defined by NeWS.
 *
 *********************************************************/

#include "def.h"
#include "graphics.h"

/* this variable is set to true when the graphics device has been opened */
int graphics = FALSE;

/* open graphics device */
void
graphics_init()
{
ps_open_PostScript();
snews_setup();
ps_flush_PostScript();
graphics = TRUE;
}

/* Assume numbers in Bertrand are in inches */
/* If other units are desired, simply use a different constant */
#define INCHES 100	/* resolution of the screen, in pixels per inch */

/* draw a line between two points */
void
draw_line(x1,y1,x2,y2)
double x1, y1, x2, y2;
{
int x1i = (int) (INCHES * x1);
int y1i = (int) (INCHES * y1);
int x2i = (int) (INCHES * x2);
int y2i = (int) (INCHES * y2);

if (!graphics) graphics_init();
snews_line(x1i, y1i, x2i, y2i);
ps_flush_PostScript();
}

/* paint a string, centered at a certain location */
void
draw_string(str,x,y)
char *str;
double x,y;
{
int xi = (int) (INCHES * x);
int yi = (int) (INCHES * y);
if (!graphics) graphics_init();
snews_string(str, xi, yi);
ps_flush_PostScript();
}

/* close the graphics device, only called if graphics == TRUE */
void
graphics_close()
{
while ('X' != getc(PostScriptInput))		;
while ('\n' != getc(PostScriptInput))		;
ps_close_PostScript();
graphics = FALSE;
}
