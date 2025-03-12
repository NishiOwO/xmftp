/* local_remote_ops.h - General file operations to perform on/in local and
 *                      remote directories.
 *
 * These should all be setup via callbacks with Main_CB * as client_data.
 * They all popup dialogs to confirm and do the operations.
 */

/**************************************************************************/

/* Make a local directory */
void mkdir_local(Widget w, XtPointer client_data, XtPointer call_data);

/* Make a remote directory */
void mkdir_remote(Widget w, XtPointer client_data, XtPointer call_data);

/* Delete a remote file OR directory (processed recursively) */
void delete_remote(Widget w, XtPointer client_data, XtPointer call_data);

/* Delete a local file OR directory (processed recursively) */
void delete_local(Widget w, XtPointer client_data, XtPointer call_data);

/* Rename a remote file */
void rename_remote(Widget w, XtPointer client_data, XtPointer call_data);

/* Rename a local file */
void rename_local(Widget w, XtPointer client_data, XtPointer call_data);

/* Execute SITE command on server */
void site_remote(Widget w, XtPointer client_data, XtPointer call_data);

/**************************************************************************/
