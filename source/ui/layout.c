/* layout.c - Main operations to setup layout of application on screen */

/************************************************************************/

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/MainW.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <malloc.h>
#include <stdio.h>
#include "layout.h"
#include "operations.h"
#include "toolbar.h"
#include "xmftp.h"
#include "pixmap.h"
#include "../program/unix/implementation.h"
#include "remote_refresh.h"
#include "../program/ftp.h"

/************************************************************************/

/* Used to add/setup the status section to parent_form. toolbar may be
 * NULL of there is no toolbar being used (see below)
 */
Widget add_status_section(Widget parent_form, Main_CB *main_cb, 
			  Widget toolbar);

/* Create one directory listing section (ScrolledList) in parent_form
 * (takes up full form)
 */
DirSection add_dir_section(Widget parent_form);

/* Callbacks for when user modifies CWD fields */
void process_implicit_local_cd(Widget w, XtPointer client_data, 
			       XtPointer call_data);
void process_implicit_remote_cd(Widget w, XtPointer client_data, 
			       XtPointer call_data);
/************************************************************************/

void create_layout(Widget main_window, Main_CB *main_callback)
{
  Widget status, main_form, local_section_form, remote_section_form,
    status_bottom, toolbar;
  DirSection local_section, remote_section;

  main_form = XtVaCreateWidget("form", xmFormWidgetClass, main_window, 
			       NULL);

  /* User can also define NOTOOLBAR to explicitly disable the
   * toolbar.
   */

#ifndef NOTOOLBAR
  toolbar = XtVaCreateWidget("toolbar", xmFormWidgetClass, main_form, 
			     XmNtopAttachment, XmATTACH_FORM,
			     XmNleftAttachment, XmATTACH_FORM,
			     XmNrightAttachment, XmATTACH_FORM,
			     NULL);
  
  add_toolbar_section(toolbar, main_callback);
  XtManageChild(toolbar);
#else
  toolbar = NULL;
#endif
  
  /* The main status output is a scrolled list widget */
  status = add_status_section(main_form, main_callback, toolbar);

  XtManageChild(status);

  /* create the two dirsections */
  local_section_form = XtVaCreateWidget("lform", xmFormWidgetClass,
					main_form,
					XmNleftAttachment, XmATTACH_FORM,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, status,
					XmNrightAttachment, XmATTACH_POSITION,
					XmNrightPosition, 50,
					XmNbottomAttachment, XmATTACH_FORM,
					NULL);

  remote_section_form = XtVaCreateWidget("rform", xmFormWidgetClass,
					 main_form,
					 XmNleftAttachment, XmATTACH_POSITION,
					 XmNleftPosition, 50,
					 XmNtopAttachment, XmATTACH_WIDGET,
					 XmNtopWidget, status,
					 XmNrightAttachment, XmATTACH_FORM,
					 XmNbottomAttachment, XmATTACH_FORM,
					 NULL);

  local_section = add_dir_section(local_section_form);
  remote_section = add_dir_section(remote_section_form);

  /* These callbacks are for entering directories in the CWD field */
  XtAddCallback(local_section.cwd_field, XmNactivateCallback,
		process_implicit_local_cd, main_callback);
  XtAddCallback(local_section.cwd_field, XmNlosingFocusCallback,
		process_implicit_local_cd, main_callback);
  XtAddCallback(remote_section.cwd_field, XmNactivateCallback,
		process_implicit_remote_cd, main_callback);
  XtAddCallback(remote_section.cwd_field, XmNlosingFocusCallback,
		process_implicit_remote_cd, main_callback);

  XtManageChild(local_section_form);
  XtManageChild(remote_section_form);

  /* A label at bottom for some version info */
  status_bottom = XtVaCreateManagedWidget(VERSION_INFO, xmLabelWidgetClass,
					  main_window, NULL);

  XtManageChild(main_form);

  XmMainWindowSetAreas(main_window, NULL, status_bottom, NULL, NULL, 
		       main_form);
				
  /* Send back info in main structure */
  main_callback->local_section = local_section;
  main_callback->remote_section = remote_section;
  main_callback->status_output = status;
  main_callback->status_bottom_output = status_bottom;
}
/************************************************************************/

DirSection add_dir_section(Widget parent_form)
{
  Arg args[10];
  Widget cwd, dirlist, cd;
  DirSection dir_section;
  int n = 0;

  /* Create a current working directory non-editable textfield */
  cwd = XtVaCreateManagedWidget("cwd", xmTextFieldWidgetClass, parent_form,
				/*
				XmNeditable, False,
				XmNcursorPositionVisible, False,
				XmNhighlightOnEnter, False,
				XmNtraversalOn, False,
				*/
				XmNtopAttachment, XmATTACH_FORM,
			        XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_POSITION,
				XmNrightPosition, 90,
				NULL);

  /* Create a button to change directories explicitly */
  cd = XtVaCreateManagedWidget("CD...", xmPushButtonWidgetClass, parent_form,
			       XmNleftAttachment, XmATTACH_POSITION,
			       XmNleftPosition, 90,
			       XmNrightAttachment, XmATTACH_FORM,
			       XmNtopAttachment, XmATTACH_FORM,
			       XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			       XmNbottomWidget, cwd,
			       NULL);

  /* Finally the actual directory listing */
  XtSetArg(args[n], XmNvisibleItemCount, 25); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, cwd); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNselectionPolicy, XmEXTENDED_SELECT); n++;
  XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
  dirlist = XmCreateScrolledList(parent_form, "dirlist", args, n);
  XtManageChild(dirlist);
  
  /* Need information to setup callbacks */
  dir_section.cwd_field = cwd;
  dir_section.dir_list = dirlist;
  dir_section.cd = cd;

  return dir_section;
}
/************************************************************************/

Widget add_status_section(Widget parent_form, Main_CB *main_cb, Widget toolbar)
{
  Arg args[10];
  Pixmap pix;
  int n = 0;
  Widget label;

  /* If using toolbar, attach this section to the bottom of it and also
   * load another icon for beauty as a logo.
   */
  if(toolbar) {
    pix = load_xmftp_pixmap(main_cb, LOGO, parent_form);

    label = XtVaCreateManagedWidget("label", xmLabelWidgetClass, parent_form,
				    XmNlabelType, XmPIXMAP,
				    XmNlabelPixmap, pix,
				    XmNrightAttachment, XmATTACH_FORM,
				    XmNtopAttachment, XmATTACH_WIDGET,
				    XmNtopWidget, toolbar,
				    NULL);

    XtVaSetValues(main_cb->toplevel, XmNiconPixmap, pix, NULL);

    XtVaCreateManagedWidget("xmftp", xmLabelWidgetClass, parent_form,
			    XmNrightAttachment, XmATTACH_FORM,
			    XmNleftAttachment, toolbar ? 
			    XmATTACH_OPPOSITE_WIDGET : XmATTACH_NONE,
			    XmNleftWidget, label,
			    XmNtopAttachment, toolbar ? 
			    XmATTACH_WIDGET : XmATTACH_FORM,
			    XmNtopWidget, label,
			    XmNalignment, XmALIGNMENT_CENTER,
			    NULL);
  }
  
  XtSetArg(args[n], XmNhighlightOnEnter, False); n++;
  XtSetArg(args[n], XmNtraversalOn, False); n++;
  XtSetArg(args[n], XmNvisibleItemCount, 4); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, toolbar ? XmATTACH_WIDGET :
	   XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopWidget, toolbar); n++;
  XtSetArg(args[n], XmNrightAttachment, toolbar ? XmATTACH_WIDGET :
	   XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightWidget, label); n++;
  return (XmCreateScrolledList(parent_form, "status", args, n));
}
/************************************************************************/

void process_implicit_local_cd(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname, *real_dir;
  XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) call_data;
  
  dirname = XmTextFieldGetString(w);
  real_dir = get_local_dir();
  
  if(!strcmp(dirname, real_dir)) {
    if(cbs->reason == XmCR_ACTIVATE) {
      /* hit enter refresh dir */
      refresh_local_dirlist(main_cb, real_dir);
    }
    free(real_dir);
    return;
  }
      
  if(!change_dir(dirname))
    error_dialog(w, "Cannot change to specified directory!");
  else {
    /* different from before */
    free(real_dir);
    real_dir = get_local_dir();
    refresh_local_dirlist(main_cb, real_dir);
  }

  XtVaSetValues(w, XmNvalue, real_dir, XmNcursorPosition, strlen(real_dir),
		NULL);
  free(real_dir);

}  
  
void process_implicit_remote_cd(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname, *real_dir;
  SiteInfo *site = &(main_cb->current_state.connection_state.site_info);
  XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) call_data;

  if(!main_cb->current_state.connection_state.connected)
    return;

  dirname = XmTextFieldGetString(w);

  real_dir = main_cb->current_state.connection_state.site_info.directory;

  if(!strcmp(dirname, real_dir)) {
    if(cbs->reason == XmCR_ACTIVATE) {
      /* user just hit enter in field to refresh */
      refresh_remote_dirlist(main_cb);
    }
    return;
  }
  
  if(!change_remote_dir(dirname))
    error_dialog(w, "Cannot change to specified remote directory!");
  else {
      /* different from before */
      /* Store current dir info in application state */
      get_remote_directory(site);
      refresh_remote_dirlist(main_cb);
  }

  XtVaSetValues(w, XmNvalue, site->directory, XmNcursorPosition, 
		strlen(site->directory), NULL);
}  







