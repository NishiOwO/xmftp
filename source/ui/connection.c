/* connection.c - FTP connection specific operations */

/****************************************************************************/

#include <Xm/Xm.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/List.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "xmftp.h"
#include "operations.h"
#include "../program/misc.h"
#include "../program/ftp.h"
#include "../program/site_db/site_db.h"
#include "../program/url.h"
#include "transfer.h"
#include "download.h"
#include "remote_refresh.h"
#include "connection.h"
#include "view.h"
#include "password.h"

/****************************************************************************/

/* for quick connect callback */
typedef struct {
  Widget dialog;
  Widget host_field;
  Widget login_field;
  PassField *password_field;
  Main_CB *main_cb;
} Qconnect_CB;

/*************************************************************************/

/* Quick Connect cancel button callback
 * destroys password field as well as dialog
 */
void cancel_connect(Widget w, XtPointer client_data, XtPointer call_data);

/* URL processing called from within Quick Connect and command line
 * makes the connection and transfers file if 'file' is non-null
 * set port to -1 for default port
 * nothing else may be NULL !
 */
void common_url(Main_CB *main_cb, char *site, char *login, char *password,
		char *directory, int port, char *file);

/* Anonymous & login type changer callback for radio box containing
 * two radio buttons in this order:
 * 1. Anonymous
 * 2. Login & Password
 */
void set_login_type(Widget w, XtPointer client_data, XtPointer call_data);

/* Fill in the appropriate information for anonymous login
 * modifies the login_field to "anonymous" and the password_field to
 * user@local.host.com in the quick connect dialog
 */
void setup_anonymous(Qconnect_CB *qcb);

/* Clear out the information for login & password type connection */
void setup_login_pass(Qconnect_CB *qcb);

/* Modify the application current state to reflect the new connection */
void mod_connection_info(Main_CB *main_cb, char *hostname, SystemType s_type,
			 char *login, char *password, int port);

/* Start a connection, OK callback */
void perform_connect(Widget w, XtPointer client_data, XtPointer call_data);

/****************************************************************************/

void quick_connect(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  Widget pane, rowcol, button_form, ok_button, cancel_button, radio_box, 
    frame, rowcol2;
  XmString anon, login;
  Qconnect_CB *qcb;

  qcb = (Qconnect_CB *) malloc(sizeof(Qconnect_CB));

  /* create the dialog */
  qcb->dialog = XmCreateFormDialog(main_cb->main_window, 
				   "Quick Connect", NULL, 0);
  XtVaSetValues(qcb->dialog,
		XmNdeleteResponse, XmDESTROY,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  
  pane = XtVaCreateWidget("pane", xmPanedWindowWidgetClass, qcb->dialog,
			  XmNsashWidth, 1,
			  XmNsashHeight, 1,
			  NULL);

  rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, pane, 
			    XmNorientation, XmVERTICAL,
			    XmNentryAlignment, XmALIGNMENT_CENTER,
			    NULL);

  /* the host field */
  XtVaCreateManagedWidget("Hostname or URL", xmLabelWidgetClass, rowcol, NULL);
  qcb->host_field = XtVaCreateManagedWidget("h_field", xmTextFieldWidgetClass,
					    rowcol, 
					    NULL);
  
  /* The login type radio box (anon or passwd) */
  frame = XtVaCreateWidget("frame", xmFrameWidgetClass, rowcol, NULL);
  rowcol2 = XtVaCreateWidget("rowcol2", xmRowColumnWidgetClass, frame, 
			     XmNorientation, XmVERTICAL,
			     XmNentryAlignment, XmALIGNMENT_CENTER,
			     NULL);
  XtVaCreateManagedWidget("Login Type", xmLabelWidgetClass, rowcol2, NULL);
  
  anon = XmStringCreateLocalized("Anonymous");
  login = XmStringCreateLocalized("Login & Password");

  radio_box = XmVaCreateSimpleRadioBox(rowcol2, "radio_box", 0, set_login_type,
				       XmVaRADIOBUTTON, anon, NULL, NULL, NULL,
				       XmVaRADIOBUTTON, login, NULL, NULL,
				       NULL, NULL);
  XtVaSetValues(radio_box, XmNuserData, qcb, NULL);

  XmStringFree(anon);
  XmStringFree(login);
  XtManageChild(radio_box);
  XtManageChild(rowcol2);
  XtManageChild(frame);

  /* The login field */
  XtVaCreateManagedWidget("Login", xmLabelWidgetClass, rowcol, NULL);
  qcb->login_field = XtVaCreateManagedWidget("l_field", xmTextFieldWidgetClass,
					     rowcol, 
					     NULL);
  XtSetSensitive(qcb->login_field, False);

  /* the passwd field */
  XtVaCreateManagedWidget("Password", xmLabelWidgetClass, rowcol, NULL);

  qcb->password_field = PField_Create(rowcol, 1);

  qcb->main_cb = main_cb;

  XtManageChild(rowcol);

  /* The buttons */
  button_form = XtVaCreateWidget("b_form", xmFormWidgetClass, pane,
				 XmNfractionBase, 5,
				 NULL);

  ok_button = XtVaCreateManagedWidget("OK", xmPushButtonWidgetClass,
				      button_form,
				      XmNtopAttachment, XmATTACH_FORM,
				      XmNbottomAttachment, XmATTACH_FORM,
				      XmNleftAttachment, XmATTACH_POSITION,
				      XmNleftPosition, 1,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 2,
				      NULL);

  XtAddCallback(ok_button, XmNactivateCallback, perform_connect,
		qcb);

  cancel_button = XtVaCreateManagedWidget("Cancel", xmPushButtonWidgetClass,
					  button_form,
					  XmNtopAttachment, XmATTACH_FORM,
					  XmNbottomAttachment, XmATTACH_FORM,
					  XmNleftAttachment, XmATTACH_POSITION,
					  XmNleftPosition, 3,
					  XmNrightAttachment, 
					  XmATTACH_POSITION,
					  XmNrightPosition, 4,
					  NULL);

  XtAddCallback(cancel_button, XmNactivateCallback, cancel_connect, qcb);
  setup_anonymous(qcb);  /* default login type */

  XtManageChild(button_form);
  XtManageChild(pane);
  XtManageChild(qcb->dialog);
}
/*************************************************************************/

void cancel_connect(Widget w, XtPointer client_data, XtPointer call_data)
{
  Qconnect_CB *qcb = (Qconnect_CB *) client_data;
  
  PField_Destroy(qcb->password_field);
  DestroyShell(NULL, qcb->dialog, NULL);
  free(qcb);
}

void perform_connect(Widget w, XtPointer client_data, XtPointer call_data)
{
  Qconnect_CB *qcb = (Qconnect_CB *) client_data;
  Main_CB *main_cb = qcb->main_cb;
  char *hostname, *login, *password;
  char *site, *directory, *file;
  int port;
  char dont_freepass = 0, dont_freelogin = 0;

  /* kill any current connection */
  do_kill_connection(main_cb);

  hostname = XmTextFieldGetString(qcb->host_field);
  
  if(!(*hostname))
    return;

  /* kill the quick connect dialog */
  XtPopdown(XtParent(qcb->dialog));
  DestroyShell(NULL, qcb->dialog, NULL);

  /* check for url */
  if(process_url(hostname, &login, &password, &site, &port, &directory, 
		 &file)) {
    /* it's a url */

    if(!login) {
      /* user did not specify login in url, use what's in the login field */
      login = XmTextFieldGetString(qcb->login_field);
      dont_freelogin = 1;
    }
    if(!password) {
      /* user did not specify password in url, use whats in the field */
      password = PField_GetString(qcb->password_field);
      dont_freepass = 1;
    }
    
    /* establish connection and maybe transfer file */
    common_url(main_cb, site, login, password, directory, port, file);

    if(!dont_freelogin)
      free(login);
    if(!dont_freepass)
      free(password);
  }
  else {
    /* its not a URL */
    login = XmTextFieldGetString(qcb->login_field);
    password = PField_GetString(qcb->password_field);

    /* perform regular login */
    do_login(main_cb, hostname, login, password, NULL, -1);
  }
  PField_Destroy(qcb->password_field);
  free(qcb);
}
/*************************************************************************/

void mod_connection_info(Main_CB *main_cb, char *hostname, SystemType s_type,
			 char *login, char *password, int port)
{
  ConnectionState *c_state = &(main_cb->current_state.connection_state);
  char cport[10];

  /* connection state will host info about the site/connection */
  c_state->connected = 1;
  c_state->site_info.system_type = s_type;
  strcpy(c_state->site_info.hostname, hostname);
  strcpy(c_state->site_info.login, login);
  strcpy(c_state->site_info.password, password);
  if(port >= 0) {
    sprintf(cport, "%d", port);
    /* port should be at most 5 chars long, structure depends on this */
    if(strlen(cport) > 5)
      c_state->site_info.port[0] = '\0';
    else
      strcpy(c_state->site_info.port, cport);
  }
  else
    /* default port */
    *(c_state->site_info.port) = '\0';
  
  get_remote_directory(&(c_state->site_info));

  XtVaSetValues(main_cb->remote_section.cwd_field, XmNvalue,
		c_state->site_info.directory,
		XmNcursorPosition, strlen(c_state->site_info.directory),
		NULL);

  refresh_remote_dirlist(main_cb);

}
/*************************************************************************/

void reconnect(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  SiteInfo s_info = main_cb->current_state.connection_state.site_info;
  char *last_resp;

  if(IsConnected(main_cb)) {
    kill_connection();
    add_string_to_status(main_cb, "Reconnecting...", NULL);
    BusyCursor(main_cb, 1);
    if(!make_connection(s_info.hostname, &last_resp, 
			(*(s_info.port)) ? atoi(s_info.port) : -1, NULL) ||
       !login_server(s_info.login, s_info.password, &last_resp, NULL) ||
       !change_remote_dir(s_info.directory)) {
      BusyCursor(main_cb, 0);
      if(call_data) {
	/* no prompt when called indirectly */
	error_dialog(w, "An error occured while reconnecting!");
	do_kill_connection(main_cb);
      }
      else {
	add_string_to_status(main_cb, "An error occured while reconnecting!", 
			     NULL);
	/* Notify indirect caller failure of reconnect */
	XtVaSetValues(w, XmNuserData, NULL, NULL);
      }
    }
    else {
      BusyCursor(main_cb, 0);
      add_string_to_status(main_cb, "Reconnect successful.", 
			   NULL);
      /* Notify indirect caller success of reconnect */
      XtVaSetValues(w, XmNuserData, last_resp, NULL);
    }
  }
  else 
    error_dialog(w, "You were never connected!");
  
}
/*************************************************************************/

void disconnect(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  do_kill_connection(main_cb);

}
/*************************************************************************/

void do_kill_connection(Main_CB *main_cb)
{
  char *dir;
  char *file;
  SiteInfo site = main_cb->current_state.connection_state.site_info;
  int len;

  main_cb->current_state.connection_state.connected = 0;
  XmListDeleteAllItems(main_cb->remote_section.dir_list);
  if(IsConnectedFromSiteMgr(main_cb)) {
    /* do it */
    dir = XmTextFieldGetString(main_cb->remote_section.cwd_field);
    strcpy(site.directory, dir);

    if(main_cb->current_state.connection_state.connect_from_site_mgr == 2) {
      /* dont save password */
      len = strlen(site.password);
      memset(site.password, '\0', len);
    }

    file = get_sitelist_filename();
    SiteInfoList_read_list(file);
    SiteInfoList_delete_site(NULL, site.hostname);
    SiteInfoList_add_site(&site);
    SiteInfoList_save_sites(file);
    SiteInfoList_destroy_list();
    main_cb->current_state.connection_state.connect_from_site_mgr = 0;
  }
  
  XmTextFieldSetString(main_cb->remote_section.cwd_field, "");
  kill_connection();
}
/*************************************************************************/


int do_login(Main_CB *main_cb, char *hostname, char *login, char *password, 
	     char *directory, int port)
{
  SystemType s_type;
  int retval;
  char templine[LINE_MAX];
  char *last_resp;
  FILE *fp;

  fp = tmpfile();
  if(port == -1)
    sprintf(templine, "Making connection to %s...", hostname);
  else
    sprintf(templine, "Making connection to %s on port %d...",
	    hostname, port);

  add_string_to_status(main_cb, templine, NULL);
  refresh_screen(main_cb);

  BusyCursor(main_cb, 1);
  retval = make_connection(hostname, &last_resp, port, fp);
  BusyCursor(main_cb, 0);

  if(!retval) {
    error_dialog(main_cb->main_window, "Connection could not be made!");
    fclose(fp);
    return 0;
  }

  add_string_to_status(main_cb, last_resp, NULL);

  refresh_screen(main_cb);

  BusyCursor(main_cb, 1);
  retval = login_server(login, password, &last_resp, fp);
  add_string_to_status(main_cb, last_resp, NULL);
  BusyCursor(main_cb, 0);

  if(main_cb->current_state.options.flags & OptionShowConnMsg)
    viewFP(main_cb->main_window, main_cb, fp);
  fclose(fp);

  refresh_screen(main_cb);

  if(!retval) {
    add_string_to_status(main_cb, "Could not login!", NULL);
    do_kill_connection(main_cb);
    error_dialog(main_cb->main_window, "Could not login!");
    return 0;
  }  

  if(!get_system_type(&s_type, &last_resp)) {
    add_string_to_status(main_cb, "Unknown error (lost connection?)", NULL);
    do_kill_connection(main_cb);
    error_dialog(main_cb->main_window, "The connection has been lost!");
  }

  if(s_type == UNKNOWN) {
    add_string_to_status(main_cb, "Unsupported system type", NULL);
    do_kill_connection(main_cb);
    info_dialog(main_cb->main_window, 
           "The remote system type is unsupported in this version of xmftp");
    return 0;
  }
  
  if(directory) {
    if(!change_remote_dir(directory)) {
    info_dialog(main_cb->main_window, 
           "Could not change directory into last directory");
    }
  }

  refresh_screen(main_cb);

  mod_connection_info(main_cb, hostname, s_type, login, password, port);
  return 1;
}
/*************************************************************************/

void set_login_type(Widget w, XtPointer client_data, XtPointer call_data)
{
  int which = (int) client_data;
  XmToggleButtonCallbackStruct *state = (XmToggleButtonCallbackStruct *)
    call_data;
  Qconnect_CB *qcb;

  XtVaGetValues(XtParent(w), XmNuserData, &qcb, NULL);

  switch(which) {
  case 0:
    /* anonymous */
    if(state->set)
      setup_anonymous(qcb);
    break;
  case 1:
    if(state->set)
      setup_login_pass(qcb);
    break;
  }
}
/*************************************************************************/

void setup_anonymous(Qconnect_CB *qcb)
{
  char *password;

  XtSetSensitive(qcb->login_field, False);

  XtVaSetValues(qcb->login_field, XmNvalue, "anonymous", NULL);
  
  password = get_email();
  PField_SetMode(qcb->password_field, 0);  
  PField_SetString(qcb->password_field, password);
}
/*************************************************************************/
  
void setup_login_pass(Qconnect_CB *qcb)
{
  XtSetSensitive(qcb->login_field, True);

  XtVaSetValues(qcb->login_field, XmNvalue, "", NULL);
  PField_SetString(qcb->password_field, "");

  PField_SetMode(qcb->password_field, 1);
}
/*************************************************************************/

void command_line_url(Main_CB *main_cb, char *url)
{
  char *login, *password, *site, *directory, *file;
  char *logdefault = "anonymous";
  int port;
  char dont_freelogin = 0;
  char dont_freepass = 0;

  if(process_url(url, &login, &password, &site, &port, &directory,
		 &file)) {
    if(!login) {
      login = logdefault;
      dont_freelogin = 1;
    }

    if(!password) {
      password = get_email();
      dont_freepass = 1;
    }

    common_url(main_cb, site, login, password, directory, port, file);

    if(!dont_freelogin)
      free(login);
    if(!dont_freepass)
      free(password);
  }
  else {
    fprintf(stderr, "\nWrong format for URL. URL format is:\n\n"
	    "ftp://[<user>:<password>@]<host>:<port>/<cwd1>/<cwd2>/"
	    ".../<cwdN>/<name>\n\n");
    exit(2);
  }
}

void common_url(Main_CB *main_cb, char *site, char *login, char *password,
		char *directory, int port, char *file)
{
  FileList *files;
  int i, number_files;
  char found = 0;
  XferInfo *xfer_info;

  if(!do_login(main_cb, site, login, password, (directory && *directory) ? 
	       directory : NULL, port)) {
    free(site); free(directory);
    if(file)
      free(file);
    return;
  }
  
  if(file && !change_remote_dir(file)) {
    /* download file */

    files = main_cb->remote_section.files;
    
    /* find where it is in filelist - linear search ack! */
    for(i = 0; i < main_cb->remote_section.num_files; i++) {
      if(!strcmp(files[i].filename, file)) {
	found = 1;
	break;
      }
    }
    
    if(!found) {
      printf("file not found\n");
      return;
    }
    XmListSelectPos(main_cb->remote_section.dir_list, i + 1, False);
    free(file);
    
    xfer_info = get_selected_files_to_XferInfo(main_cb, &number_files, 1);
    if(!xfer_info)
      return;
    perform_download(main_cb, NULL, xfer_info, number_files);
    
    destroy_XferInfo(&xfer_info, number_files);
    refresh_local_dirlist(main_cb, ".");
  }
  else
    refresh_remote_dirlist(main_cb);
  free(site); free(directory);
}


