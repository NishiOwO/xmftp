/* view.h - View text file operations */

#include <stdio.h>

/*************************************************************************/

/* View an open file pointed to by fp. This FILE * is rewound, it is NOT
 * closed.
 */
void viewFP(Widget w, Main_CB *main_cb, FILE *fp);

/* View selected remote file */
void view_remote(Widget w, XtPointer client_data, XtPointer call_data);

/* View selected local file */
void view_local(Widget w, XtPointer client_data, XtPointer call_data);

/*************************************************************************/

