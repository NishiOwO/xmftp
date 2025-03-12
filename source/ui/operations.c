/* operations.c - Many general user interface functions */

/*************************************************************************/

#include <X11/cursorfont.h>
#include <Xm/List.h>
#include <Xm/FileSB.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <Xm/DialogS.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "operations.h"
#include "../program/misc.h"
#include "../program/ftp.h"
#include "download.h"
#include "fill_filelist.h"
#include "transfer.h"
#include "upload.h"
#include "view.h"
#include "connection.h"

/*************************************************************************/

/* Ok and cancel button callbacks for changing local directory */
void process_dir_selection(Widget w, XtPointer client_data, 
			   XtPointer call_data);
void cancel_dir_selection(Widget w, XtPointer client_data, 
			   XtPointer call_data);

/* double-click callback on local directory section */
void traverse_locally(Widget w, XtPointer client_data, XtPointer call_data);

/*************************************************************************/

void set_ascii_type(Widget w, XtPointer client_data, XtPointer call_data)
{}
/*************************************************************************/

void set_binary_type(Widget w, XtPointer client_data, XtPointer call_data)
{}
/*************************************************************************/

void set_remote_dir_label(Main_CB *main_cb)
{
  ConnectionState *c_state = &(main_cb->current_state.connection_state);
  
  /* The CWD label may be out of sync, lets get info and re-update it */
  get_remote_directory(&(c_state->site_info));
  XtVaSetValues(main_cb->remote_section.cwd_field, XmNvalue,
		c_state->site_info.directory,
		XmNcursorPosition, strlen(c_state->site_info.directory),
		NULL);
}
/*************************************************************************/

void set_local_dir_label(Main_CB *main_cb)
{
  char *cwd;

  cwd = get_local_dir();
  XtVaSetValues(main_cb->local_section.cwd_field, XmNvalue,
		cwd,
		XmNcursorPosition, strlen(cwd),
		NULL);
  
  free(cwd);
}
/*************************************************************************/

void die_all(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  /* Other cleanup operations would be added if necessary */
  do_kill_connection(main_cb);
  exit(0);
}
/*************************************************************************/

void options(Widget w, XtPointer client_data, XtPointer call_data)
{}
/*************************************************************************/

void download(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XferInfo *xfer_info;
  int number;

  /* Get selected files (also recursing into subdirs) */
  BusyCursor(main_cb, 1);
  xfer_info = get_selected_files_to_XferInfo(main_cb, &number, 1);
  BusyCursor(main_cb, 0);

  if(!xfer_info) {
    error_dialog(w, "No files were selected!");
    return;
  }
  
  /* for RTM mode */
  while(!perform_download(main_cb, "./", xfer_info, number));

  destroy_XferInfo(&xfer_info, number);
}
/*************************************************************************/

void upload(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XferInfo *xfer_info;
  int number;

  if(!main_cb->current_state.connection_state.connected) {
    error_dialog(w, 
		 "You must first connect to a site before selecting upload.");
    return;
  }

  /* Get selected files (also recursing into subdirs) */
  BusyCursor(main_cb, 1);
  xfer_info = get_selected_files_to_XferInfo(main_cb, &number, 0);
  BusyCursor(main_cb, 0);

  if(!xfer_info) {
    error_dialog(w, "No files were selected!");
    return;
  }

  perform_upload(main_cb, "./", xfer_info, number);

  destroy_XferInfo(&xfer_info, number);

}
/*************************************************************************/

void chdir_locally(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget dialog;

  dialog = XmCreateFileSelectionDialog(w, "filesb", NULL, 0);
  XtVaSetValues(dialog, XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  XtSetSensitive(XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST), False);
  XtSetSensitive(XmFileSelectionBoxGetChild(dialog, XmDIALOG_LIST_LABEL), 
		 False);
  XtAddCallback(dialog, XmNokCallback, process_dir_selection, client_data);
  XtAddCallback(dialog, XmNcancelCallback, cancel_dir_selection, w);
  
  XtManageChild(dialog);
}
/*************************************************************************/

void cancel_dir_selection(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{  
  XtDestroyWidget(w);
}
/*************************************************************************/

void process_dir_selection(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XmFileSelectionBoxCallbackStruct *cbs =   
    (XmFileSelectionBoxCallbackStruct *) call_data;
  char *dirname;
  
  XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &dirname);

  if(!change_dir(dirname))
    error_dialog(w, "Cannot change to specified directory!");
  else {
    XtVaSetValues(main_cb->local_section.cwd_field, XmNvalue, dirname, 
		  XmNcursorPosition, strlen(dirname),
		  NULL);
    XtDestroyWidget(w);
    refresh_local_dirlist(main_cb, dirname);
  }
  free(dirname);
}
/*************************************************************************/

void error_dialog(Widget parent, char *message)
{
  Widget topshell = GetTopShell(parent);
  Widget error;
  XmString error_msg, cancel_label;

  error = XmCreateErrorDialog(topshell, "error", NULL, 0);
  error_msg = XmStringCreateLocalized(message);
  cancel_label = XmStringCreateLocalized("OK");
  XtVaSetValues(error, XmNmessageString, error_msg,
		XmNcancelLabelString, cancel_label,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  XtUnmanageChild(XmMessageBoxGetChild(error, XmDIALOG_OK_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(error, XmDIALOG_HELP_BUTTON));
  
  XmStringFree(error_msg);
  XmStringFree(cancel_label);
  XtManageChild(error);
}  
/*************************************************************************/

Widget warning_y_n_dialog(Widget parent, char *message)
{
  Widget dialog;
  XmString prompt, ok_str, cancel_str;
  
  dialog = XmCreateWarningDialog(parent, "Warning", NULL, 0);
  prompt = XmStringCreateLocalized(message);
  ok_str = XmStringCreateLocalized("Yes");
  cancel_str = XmStringCreateLocalized("No");

  XtVaSetValues(dialog, XmNmessageString, prompt,
		XmNokLabelString, ok_str,
		XmNcancelLabelString, cancel_str,
		XmNdefaultButtonType, XmDIALOG_CANCEL_BUTTON,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);

  XmStringFree(prompt);
  XmStringFree(ok_str);
  XmStringFree(cancel_str);
  return dialog;
}
/*************************************************************************/

void info_dialog(Widget parent, char *message)
{
  Widget topshell = GetTopShell(parent);
  Widget error;
  XmString error_msg, cancel_label;

  error = XmCreateInformationDialog(topshell, "error", NULL, 0);
  error_msg = XmStringCreateLocalized(message);
  cancel_label = XmStringCreateLocalized("OK");
  XtVaSetValues(error, XmNmessageString, error_msg,
		XmNcancelLabelString, cancel_label,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  XtUnmanageChild(XmMessageBoxGetChild(error, XmDIALOG_OK_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(error, XmDIALOG_HELP_BUTTON));
  
  XmStringFree(error_msg);
  XmStringFree(cancel_label);
  XtManageChild(error);
}  
/*************************************************************************/

void add_string_to_status(Main_CB *main_cb, char *string, XmString mstring)
{
  XmString to_status;
  char *ch_ptr;

  if(string) {
    /* Get rid of junk that ftpd gives us */
    while((ch_ptr = strpbrk(string, "\r\n")))
      *ch_ptr = '\0';
    to_status = XmStringCreateLocalized(string);
    XmListAddItem(main_cb->status_output, to_status, 0);
    XmStringFree(to_status);
  }
  else 
    XmListAddItem(main_cb->status_output, mstring, 0);
  
  XmListSetBottomPos(main_cb->status_output, 0);
}
/*************************************************************************/

void traverse_locally(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname, *total_dir_name;
  int *position_list, position_count;
  FileList file;

  if(XmListGetSelectedPos(w, &position_list, &position_count) == False)
    return;
  file = main_cb->local_section.files[(*position_list-1)];
  free(position_list);

  dirname = file.filename;
  
  if(!file.link) {
    /* File is NOT a link */
    if(!file.directory) {
      /* Looks like a regular file, view it */
      view_local(w, main_cb, NULL);
      return;
    }

    /* Must be a directory */
    if(!change_dir(dirname)) {
      error_dialog(w, "Cannot change to specified directory!");
      return;
    }
  }
  else {
    /* it's a link, what to do? */
    if(local_file_exists(file.filename, &(file.size)) == 1) {
      /* It looks like a regular file, let's view it */
      view_local(w, main_cb, NULL);
      return;
    }
    /* It must be a directory, but still may not be able to cd (permission) */
    if(!change_dir(dirname)) {
      error_dialog(w, "Cannot change to specified directory!");
      return;
    }
  }

  total_dir_name = get_local_dir();    
  XtVaSetValues(main_cb->local_section.cwd_field, XmNvalue, total_dir_name, 
		XmNcursorPosition, strlen(total_dir_name),
		NULL);
  refresh_local_dirlist(main_cb, total_dir_name);
  free(total_dir_name);
}
/*************************************************************************/
  
void destroy_list(DirSection *section)
{
  XmListDeleteAllItems(section->dir_list);
  if(section->files) {
    destroy_filelist(&(section->files), section->num_files);
    section->num_files = 0;
  }
}
/*************************************************************************/

void refresh_local_dirlist(Main_CB *main_callback, char *dirname)
{
  unsigned int num_files;
  FileList *files;

  if(main_callback->local_section.files)
    destroy_list(&(main_callback->local_section));

  files = generate_local_filelist(dirname, &num_files, 
				  main_callback->current_state.dir_state);
  
  if(!files) {
    puts("files is null!");
    exit(1);
  }

  main_callback->local_section.files = files;
  main_callback->local_section.num_files = num_files;

  fill_slist(files, main_callback->local_section.dir_list, num_files, 
	     (main_callback->current_state.options.flags) & OptionLargeFont,
	     (main_callback->current_state.options.flags) & 
	     OptionDontCropFNames);
}
/*************************************************************************/
  
Widget GetTopShell(Widget w)
{
  while(w && !XtIsWMShell(w))
    w = XtParent(w);
  return w;
}
/*************************************************************************/

void DestroyShell(Widget w, XtPointer client_data, XtPointer call_data)
{
  Widget shell = (Widget) client_data;

  XtDestroyWidget(shell);
}
/*************************************************************************/

void refresh_screen(Main_CB *main_cb)
{
  XFlush(main_cb->dpy);
  XmUpdateDisplay(main_cb->toplevel);
}
/*************************************************************************/

void CheckForInterrupt(Main_CB *main_cb, Widget w)
{
  /* Window win = XtWindow(w); */
  XEvent event;

  XFlush(main_cb->dpy);
  XmUpdateDisplay(main_cb->toplevel);
  
  while(XCheckMaskEvent(main_cb->dpy,
			ButtonPressMask | ButtonReleaseMask |
			ButtonMotionMask | PointerMotionMask |
			KeyPressMask, &event)) {
    XtDispatchEvent(&event);

  }
}
/*************************************************************************/

void InterruptSigGoUI(void *to_maincb, int *siggo)
{
  XEvent event;
  Main_CB *main_cb = to_maincb;

  XFlush(main_cb->dpy);
  XmUpdateDisplay(main_cb->toplevel);
  
  while(XCheckMaskEvent(main_cb->dpy,
			ButtonPressMask | ButtonReleaseMask |
			ButtonMotionMask |
			KeyPressMask, &event)) {
    XtDispatchEvent(&event);
    *siggo = 1;
  }
}

void BusyCursor(Main_CB *main_cb, int status)
{
  Cursor cursor;
  XSetWindowAttributes attrs;

  if(status) {
    cursor = XCreateFontCursor(main_cb->dpy, XC_watch);
    attrs.cursor = cursor;
  }
  else
    attrs.cursor = None;

  XChangeWindowAttributes(main_cb->dpy, XtWindow(main_cb->toplevel),
			  CWCursor, &attrs);

  XFlush(main_cb->dpy);
}
/*************************************************************************/
