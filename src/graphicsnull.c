/*********************************************************
 *
 * Null graphics routines.
 *
 * If you do not have the capabilities for graphics output
 * you can use these routines to supply null stubs.
 *
 *********************************************************/

#include "def.h"

/* this variable is set to true when the graphics device has been opened */
int graphics = FALSE;

/* open graphics device */
void
graphics_init()
{
printf("null graphics device is open\n");
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
if (!graphics) graphics_init();
printf("draw line from (%g,%g) to (%g,%g)\n", x1, y1, x2, y2);
}

/* paint a string, centered at a certain location */
void
draw_string(str,x,y)
char *str;
double x,y;
{
if (!graphics) graphics_init();
printf("draw string \"%s\" at (%g,%g)\n", str, x, y);
}

/* close the graphics device, only called if graphics == TRUE */
void
graphics_close()
{
printf("close graphics device\n");
graphics = FALSE;
}
