/* remote_refresh.h - operations specific to remote section dir listing */

/**************************************************************************/

#include "xmftp.h"

/**************************************************************************/

/* Refresh the remote directory listing */
void refresh_remote_dirlist(Main_CB *main_cb);

/* double-click callback for remote section */
void traverse_remotely(Widget w, XtPointer client_data, XtPointer call_data);

/* Dialog to change remote dir specifically */
void chdir_remotely(Widget w, XtPointer client_data, XtPointer call_data);

void sort_remote_dirlist(Main_CB *main_cb);

/**************************************************************************/
