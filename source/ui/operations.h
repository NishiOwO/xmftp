/* operation.h - Many general functions used throughout */

/**************************************************************************/

#include "xmftp.h"

/**************************************************************************/

/* Sets the transfer mode to ASCII
 * This function is currently unimplimented
 */
void set_ascii_type(Widget w, XtPointer client_data, XtPointer call_data);

/* Sets the transfer mode to binary
 * This function is currently unimplimented
 */
void set_binary_type(Widget w, XtPointer client_data, XtPointer call_data);

/* Exiting of program */
void die_all(Widget w, XtPointer client_data, XtPointer call_data);

/* User definable options
 * This function is currently unimplimented
 */
void options(Widget w, XtPointer client_data, XtPointer call_data);

/* Perform a retrieval of remote files */
void download(Widget w, XtPointer client_data, XtPointer call_data);

/* Perform a send of local files to remote host */
void upload(Widget w, XtPointer client_data, XtPointer call_data);

/* Dialog prompt to change to a specified dir */
void chdir_locally(Widget w, XtPointer client_data, XtPointer call_data);

/* Reread a directory listing and update display
 * dirname should be "." for current directory
 */
void refresh_local_dirlist(Main_CB *main_callback, char *dirname);

/* The double-click callback on the local directory section */
void traverse_locally(Widget w, XtPointer client_data, XtPointer call_data);

/* Add a string to the status output window
 * if string is NULL, mstring is used, otherwise mstring is ignored
 */
void add_string_to_status(Main_CB *main_cb, char *string, XmString mstring);

/* Destroy the directory list in the specified section freeing the memory */
void destroy_list(DirSection *section);

/* Bring up an error dialog with message */
void error_dialog(Widget parent, char *message);

/* Bring up an info dialog with message */
void info_dialog(Widget parent, char *message);

/* Warning dialog asking yes/no with message 
 * returns dialog
 */
Widget warning_y_n_dialog(Widget parent, char *message);

/* Retrieve the topshell of a widget */
Widget GetTopShell(Widget w);

/* Refresh the screen (for forced updates) */
void refresh_screen(Main_CB *main_cb);

/* Update the current remote working directory label from actual value */
void set_remote_dir_label(Main_CB *main_cb);

/* Update the current local working directory label from actual value */
void set_local_dir_label(Main_CB *main_cb);

/* Process event queue for widget w */
void CheckForInterrupt(Main_CB *main_cb, Widget w);

/* Changes the mouse pointer to an hourglass if status is 1, back to
 * normal if status is 0.
 */
void BusyCursor(Main_CB *main_cb, int status);

/* General purpose destroywidget of client_data */
void DestroyShell(Widget w, XtPointer client_data, XtPointer call_data);

void InterruptSigGoUI(void *to_maincb, int *siggo);
/**************************************************************************/

