/* site_mgr.c - Site manager specific operations */

/***************************************************************************/

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/ToggleB.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/SelectioB.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "xmftp.h"
#include "../program/site_db/site_db.h"
#include "../program/misc.h"
#include "site_mgr.h"
#include "operations.h"
#include "connection.h"
#include "password.h"

/***************************************************************************/

#ifndef FILENAME_MAX
#define FILENAME_MAX LINE_MAX
#endif

typedef struct {
  Widget dialog, site, login, password, dir, comment, sitelist;
  Widget b_edit, b_delete, b_connect, b_add, b_addcurr; /* buttons */
  Main_CB *main_cb;
} SiteDispInfo;

typedef struct {
  Widget hostfield, loginfield, commfield, nickfield,
    portfield, portlabel, stdport, nstdport;
  PassField *passfield;
  Widget dialog;
  SiteDispInfo *disp_info;
} SiteAddInfo;

/***************************************************************************/
/* Prompt for a password if password field blank and trying to connect
 * User does not have to store passwd in sitelist file
 */
void prompt_password(Main_CB *main_cb, SiteInfo *site);

/* Callbacks for the above prompt */
void ok_password(Widget w, XtPointer client_data, XtPointer call_data);
void cancel_password(Widget w, XtPointer client_data, XtPointer call_data);

/* Perform a connection */
int perform_login(Main_CB *main_cb, SiteInfo *site);

/* Login field Text widget changed callback (modifies password field) */
void login_field_changed(Widget w, XtPointer client_data, 
			 XtPointer call_data);
void set_port(Widget w, XtPointer client_data, XtPointer call_data);

/* Clear the fields of dialog */
void clear_info(SiteDispInfo *disp_info);

/* Popup the dialog common to Add and Delete operations 
 * operations is 0 for Add, 1 for Edit
 * b_add and b_cancel are modified to point to the corresponding buttons
 * add_info is modified to contain the site information
*/
Widget do_common_add_mod_dialog(Widget main_window, int operation,
				Widget *b_add, Widget *b_cancel, 
				SiteAddInfo *add_info);

/* Cancel add/edit */
void cancelled_add(Widget w, XtPointer client_data, XtPointer call_data);

/* Cancel site manager */
void cancelled(Widget w, XtPointer client_data, XtPointer call_data);

/* Delete site callback */
void delete_site(Widget w, XtPointer client_data, XtPointer call_data);

/* Actual adding of site */
void do_addition(Widget w, XtPointer client_data, XtPointer call_data);

/* Setup the site information window/frame */
void addto_site_info(Widget w, SiteDispInfo *disp_info);

/* Load the user's sites into memory */
void load_sites(SiteDispInfo *disp_info);


/* Setup callbacks on site manager dialog */
void setup_cbs(SiteDispInfo *disp_info);

/* Update display (when site is deleted/modified) */
void modify_display(Widget w, XtPointer client_data, XtPointer call_data);

/* Add button callback, add a new site */
void add_new_site(Widget w, XtPointer client_data, XtPointer call_data);

/* Edit button callback, edit a site */
void edit_site(Widget w, XtPointer client_data, XtPointer call_data);

/* Put buttons into the site manager (layout) */
void add_buttons(Widget w, SiteDispInfo *disp_info);

/* Connect to the selected site */
void connect_from_site_mgr(Widget w, XtPointer client_data, XtPointer 
			   call_data);

void add_current_site(Widget w, XtPointer client_data, XtPointer call_data);

/***************************************************************************/

void site_manager(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  Widget dialog, frame, s_info;
  Arg args[15];
  int n = 0;
  SiteDispInfo *disp_info = NULL;

  disp_info = (SiteDispInfo *) malloc(sizeof(SiteDispInfo));
  disp_info->main_cb = main_cb;

  dialog = XmCreateFormDialog(main_cb->main_window, "Site Manager",
			      NULL, 0);
  disp_info->dialog = dialog;

  XtVaSetValues(dialog,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNwidth, 400,
		XmNheight, 400,
		XmNresizePolicy, XmRESIZE_NONE,
		XmNautoUnmanage, False,
		NULL);
  

  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 30); n++;
  disp_info->sitelist = XmCreateScrolledList(dialog, "slist", args,
					     n);
  XtManageChild(disp_info->sitelist);

  frame = XtVaCreateWidget("frame1", xmFrameWidgetClass, dialog,
			   XmNtopAttachment, XmATTACH_FORM,
			   XmNleftAttachment, XmATTACH_POSITION,
			   XmNleftPosition, 32,
			   XmNrightAttachment, XmATTACH_FORM,
			   XmNrightOffset, 2,
			   XmNtopPosition, XmATTACH_FORM,
			   XmNbottomAttachment, XmATTACH_POSITION,
			   XmNbottomPosition, 50,
			   NULL);

  XtVaCreateManagedWidget("Site Information", xmLabelWidgetClass, frame,
			  XmNchildType, XmFRAME_TITLE_CHILD,
			  XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
			  NULL);
  s_info = XtVaCreateWidget("form", xmFormWidgetClass, frame, NULL);
  addto_site_info(s_info, disp_info);

  XtManageChild(s_info);
  XtManageChild(frame);

  load_sites(disp_info);

  add_buttons(dialog, disp_info);

  setup_cbs(disp_info);

  XtSetSensitive(disp_info->b_edit, False);
  XtSetSensitive(disp_info->b_delete, False);
  XtSetSensitive(disp_info->b_connect, False);

  XtManageChild(dialog);
}
/***************************************************************************/
  
Widget do_common_add_mod_dialog(Widget main_window, int operation,
				Widget *b_add, Widget *b_cancel, 
				SiteAddInfo *add_info)
{
  Widget dialog, hostfield, loginlabel, loginfield, hostlabel, 
    passlabel, commlabel, commfield,
    nicklabel, nickfield, portfield, frame, rowcol, form, radio_box,
    genlabel;
  Widget stdport, nstdport;
  PassField *passfield;

  dialog = XmCreateFormDialog(main_window, operation ? "Edit Site" : 
			      "Add Site", NULL, 0);
  
  XtVaSetValues(dialog,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNresizePolicy, XmRESIZE_NONE,
		XmNentryAlignment, XmALIGNMENT_CENTER,
		XmNautoUnmanage, False,
		XmNwidth, 420,
		XmNheight, 350,
		NULL);

  hostlabel = XtVaCreateManagedWidget("Hostname", xmLabelWidgetClass, dialog, 
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNtopAttachment, XmATTACH_FORM,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 40,
				      NULL);

  hostfield = XtVaCreateManagedWidget("hostf", xmTextFieldWidgetClass, dialog,
				      XmNleftOffset, 5,
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, hostlabel,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 40,
				      NULL);
  
  nicklabel = XtVaCreateManagedWidget("Nickname", xmLabelWidgetClass, 
				      dialog,
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, hostfield,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 40,
				      NULL);

  nickfield = XtVaCreateManagedWidget("hostf", xmTextFieldWidgetClass, dialog,
				      XmNleftOffset, 5,
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, nicklabel,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 40,
				      NULL);

  loginlabel = XtVaCreateManagedWidget("Login", xmLabelWidgetClass, dialog, 
				       XmNleftAttachment, XmATTACH_POSITION,
				       XmNleftPosition, 60,
				       XmNtopAttachment, XmATTACH_FORM,
				       XmNrightAttachment, XmATTACH_FORM,
				       NULL);

  loginfield = XtVaCreateManagedWidget("hostf", xmTextFieldWidgetClass, dialog,
				       XmNleftAttachment, XmATTACH_POSITION,
				       XmNleftPosition, 60,
				       XmNtopAttachment, XmATTACH_WIDGET,
				       XmNtopWidget, loginlabel,
				       XmNrightOffset, 5,
				       XmNrightAttachment, XmATTACH_FORM,
				       NULL);

  passlabel = XtVaCreateManagedWidget("Password", xmLabelWidgetClass, dialog, 
				      XmNleftAttachment, XmATTACH_POSITION,
				      XmNleftPosition, 60,
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, loginfield,
				      XmNrightAttachment, XmATTACH_FORM,
				      NULL);
  /*
  passfield = XtVaCreateManagedWidget("passf", xmTextFieldWidgetClass, dialog,
				      XmNleftAttachment, XmATTACH_POSITION,
				      XmNleftPosition, 60,
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, passlabel,
				      XmNrightOffset, 5,
				      XmNrightAttachment, XmATTACH_FORM,
				      NULL);
				      */
  passfield = PField_Create(dialog, 1);
  XtVaSetValues(passfield->textf,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 60,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, passlabel,
		XmNrightOffset, 5,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);
  
  frame = XtVaCreateManagedWidget("frame", xmFrameWidgetClass, dialog,
				  XmNrightOffset, 5,
				  XmNleftOffset, 5,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, passfield->textf,
				  XmNtopOffset, 5,
				  XmNtopPosition, 40,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_POSITION,
				  XmNbottomPosition, 65,
				  NULL);
  XtVaCreateManagedWidget("Other Info", xmLabelWidgetClass, frame,
			  XmNchildType, XmFRAME_TITLE_CHILD,
			  XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
			  NULL);

  rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, frame, 
			    XmNorientation, XmVERTICAL,
			    XmNentryAlignment, XmALIGNMENT_CENTER,
			    NULL);
  
  radio_box = XmCreateRadioBox(rowcol, "radios", NULL, 0);

  stdport = XtVaCreateManagedWidget("Standard Port",
				    xmToggleButtonWidgetClass,
				    radio_box, NULL);

  nstdport = XtVaCreateManagedWidget("Nonstandard Port",
				     xmToggleButtonWidgetClass,
				     radio_box, NULL);
  XtAddCallback(radio_box, XmNentryCallback, set_port, add_info);
  
  XtManageChild(radio_box);
  XtManageChild(rowcol);

  form = XtVaCreateWidget("form", xmFormWidgetClass, rowcol, NULL);
  genlabel = XtVaCreateManagedWidget("Port #:", xmLabelWidgetClass, form,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNtopAttachment, XmATTACH_FORM,
				     XmNtopOffset, 5,
				     NULL);

  portfield = XtVaCreateManagedWidget("pfield", xmTextFieldWidgetClass, form,
				      XmNcolumns, 10,
				      XmNleftAttachment, XmATTACH_WIDGET,
				      XmNleftWidget, genlabel,
				      XmNtopAttachment, XmATTACH_FORM,
				      NULL);
  XtManageChild(form);
			    

  commlabel = XtVaCreateManagedWidget("Comment", xmLabelWidgetClass, dialog, 
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNleftOffset, 5, 
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, frame,
				      XmNtopOffset, 10,
				      NULL);
  
  commfield = XtVaCreateManagedWidget("passf", xmTextFieldWidgetClass, dialog,
				      XmNleftAttachment, XmATTACH_FORM,
				      XmNleftOffset, 5, 
				      XmNtopAttachment, XmATTACH_WIDGET,
				      XmNtopWidget, commlabel,
				      XmNrightOffset, 5,
				      XmNrightAttachment, XmATTACH_FORM,
				      NULL);
  
    
  *b_add = XtVaCreateManagedWidget(operation ? "OK" : "Add", 
				   xmPushButtonWidgetClass, dialog,
				   XmNleftAttachment, XmATTACH_POSITION,
				   XmNleftPosition, 10,
				   XmNrightAttachment, XmATTACH_POSITION,
				   XmNrightPosition, 40,
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNbottomOffset, 10,
				   NULL);

  *b_cancel = XtVaCreateManagedWidget("Cancel", xmPushButtonWidgetClass, 
				      dialog,
				      XmNleftAttachment, XmATTACH_POSITION,
				      XmNleftPosition, 60,
				      XmNrightAttachment, XmATTACH_POSITION,
				      XmNrightPosition, 90,
				      XmNbottomAttachment, XmATTACH_FORM,
				      XmNbottomOffset, 10,
				      NULL);

  add_info->hostfield = hostfield;
  add_info->loginfield = loginfield;
  add_info->passfield = passfield;
  add_info->commfield = commfield;
  add_info->nickfield = nickfield;
  add_info->dialog = dialog;
  add_info->portlabel = genlabel;
  add_info->portfield = portfield;
  add_info->stdport = stdport;
  add_info->nstdport = nstdport;
  return dialog;
}
/***************************************************************************/

void add_new_site(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  Main_CB *main_cb = disp_info->main_cb;
  Widget dialog;
  SiteAddInfo *add_info;
  Widget b_add, b_cancel;

  add_info = (SiteAddInfo *) malloc(sizeof(SiteAddInfo));
  add_info->disp_info = disp_info;

  dialog = do_common_add_mod_dialog(main_cb->main_window, 0,
				    &b_add, &b_cancel, add_info);

  XtVaSetValues(b_add, XmNuserData, NULL, NULL);
  XtVaSetValues(b_cancel, XmNuserData, NULL, NULL);

  XmToggleButtonSetState(add_info->stdport, True, True);

  XtAddCallback(b_add, XmNactivateCallback, do_addition, add_info);
  XtAddCallback(b_cancel, XmNactivateCallback, cancelled_add, add_info);
  XtAddCallback(add_info->loginfield, XmNvalueChangedCallback, 
		login_field_changed, add_info->passfield);
  XtManageChild(dialog);

}
/***************************************************************************/

void login_field_changed(Widget w, XtPointer client_data, XtPointer call_data)
{
  PassField *pf = (PassField *) client_data;
  char *login;

  login = XmTextFieldGetString(w);
  
  if(!login) {
    puts("null");
    return;
  }

  if(!strcmp(login, "anonymous"))
    PField_SetMode(pf, 0);
  else
    PField_SetMode(pf, 1); 
}

void edit_site(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  Main_CB *main_cb = disp_info->main_cb;
  Widget dialog;
  SiteAddInfo *add_info;
  Widget b_add, b_cancel;
  int *position_list;
  int position_count;
  SiteInfo site;
  char *text;
  XmString *items;
  char *directory;

  if(XmListGetSelectedPos(disp_info->sitelist, &position_list, &position_count)
     == False)
    return;

  free(position_list);

  /* Get the selected site information */
  XtVaGetValues(disp_info->sitelist, XmNselectedItems, &items, NULL);

  XmStringGetLtoR(*items, XmFONTLIST_DEFAULT_TAG, &text);

  if(!SiteInfoList_find_site(text, &site))
    return;

  add_info = (SiteAddInfo *) malloc(sizeof(SiteAddInfo));
  add_info->disp_info = disp_info;

  dialog = do_common_add_mod_dialog(main_cb->main_window, 1,
				    &b_add, &b_cancel, add_info);
  
  /* Modify the fields */
  XmTextFieldSetString(add_info->hostfield, site.hostname);
  XmTextFieldSetString(add_info->loginfield, site.login);

  if(!strcmp(site.login, "anonymous"))
    PField_SetMode(add_info->passfield, 0);

  PField_SetString(add_info->passfield, site.password);
  XmTextFieldSetString(add_info->commfield, site.comment);
  XmTextFieldSetString(add_info->nickfield, site.nickname);

  if(*(site.port)) {
    XmToggleButtonSetState(add_info->nstdport, True, True);
    XmTextFieldSetString(add_info->portfield, site.port);
  }
  else
    XmToggleButtonSetState(add_info->stdport, True, True);
    
  directory = (char *) malloc(strlen(site.directory) + 1);
  strcpy(directory, site.directory);
  XtVaSetValues(b_add, XmNuserData, directory, NULL);
  XtVaSetValues(b_cancel, XmNuserData, directory, NULL);

  /* do_addition will delete a site (if exists) and add it again */
  XtAddCallback(b_add, XmNactivateCallback, do_addition, add_info);
  XtAddCallback(b_cancel, XmNactivateCallback, cancelled_add, add_info);
  XtAddCallback(add_info->loginfield, XmNvalueChangedCallback, 
		login_field_changed, add_info->passfield);
		
  XtManageChild(dialog);
}
/***************************************************************************/

void do_addition(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteAddInfo *add_info = (SiteAddInfo *) client_data;
  SiteDispInfo *disp_info = add_info->disp_info;
  char *file;
  char *host, *login, *pass, *comm, *nick, *directory, *port;
  char *anon_login = "anonymous";
  SiteInfo site;
  XmString string;

  /* Get user site database file */
  file = get_sitelist_filename();

  /* Get the information user entered */
  host = XmTextFieldGetString(add_info->hostfield);
  login = XmTextFieldGetString(add_info->loginfield);

  pass = PField_GetString(add_info->passfield);
  comm = XmTextFieldGetString(add_info->commfield);
  nick = XmTextFieldGetString(add_info->nickfield);
  port = XmTextFieldGetString(add_info->portfield);

  if(!strlen(login)) {
    /* If user has blank login field, use anonymous login */
    login = anon_login;
    pass = get_email();
  }

  if(!strlen(host) || !strlen(login) || !strlen(nick)) {
    info_dialog(w, "You must fill out all the relevant fields.");
    return;
  }

  if(strlen(port) > 5) {
    info_dialog(w, "Port number can only be up to 5 digits long");
    return;
  }

  strcpy(site.login, login);
  strcpy(site.password, pass);
  strcpy(site.hostname, host);
  XtVaGetValues(w, XmNuserData, &directory, NULL);
  /* Save last directory if needed */
  if(directory) {
    strcpy(site.directory, directory);
    free(directory);
    XtVaSetValues(w, XmNuserData, NULL, NULL);
    directory = NULL;
  }
  else
    *(site.directory) = '\0';

  strcpy(site.comment, comm);
  strcpy(site.nickname, nick);
  strcpy(site.port, port);

  /* Default systype to unix, should change this */
  site.system_type = UNIX;  

  /* Delete the site if it exists (for edit and duplicates) */
  SiteInfoList_delete_site(site.nickname, NULL);
  SiteInfoList_add_site(&site);
  SiteInfoList_save_sites(file);
  load_sites(disp_info);
  
  PField_Destroy(add_info->passfield);
  XtPopdown(XtParent(add_info->dialog));
  XtDestroyWidget(XtParent(add_info->dialog));

  string = XmStringCreateLocalized(site.nickname);
  XmListSelectItem(disp_info->sitelist, string, True);
  free(add_info);
  XmStringFree(string);
}
/***************************************************************************/

void add_buttons(Widget w, SiteDispInfo *disp_info)
{
  Widget cancel;
  Main_CB *main_cb = disp_info->main_cb;

  if(IsConnected(main_cb) && !IsConnectedFromSiteMgr(main_cb)) {
    /* add a button to add current site connected to */
    disp_info->b_addcurr = 
      XtVaCreateManagedWidget("Add current site", 
			      xmPushButtonWidgetClass, w,
			      XmNleftAttachment, XmATTACH_POSITION,
			      XmNleftPosition, 45,
			      XmNrightAttachment, XmATTACH_POSITION,
			      XmNrightPosition, 85,
			      XmNtopAttachment, XmATTACH_POSITION,
			      XmNtopPosition, 70,
			      NULL);
  }
  else
    disp_info->b_addcurr = NULL;

  disp_info->b_add = XtVaCreateManagedWidget("Add Site",
					     xmPushButtonWidgetClass, w,
					     XmNleftAttachment, 
					     XmATTACH_POSITION,
					     XmNleftPosition, 30,
					     XmNrightAttachment, 
					     XmATTACH_POSITION,
					     XmNrightPosition, 50,
					     XmNtopAttachment, 
					     XmATTACH_POSITION,
					     XmNtopPosition, 80,
					     NULL);

  disp_info->b_delete = 
    XtVaCreateManagedWidget("Delete Site", xmPushButtonWidgetClass, w,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 55,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 75,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 80,
			    NULL);
  
  disp_info->b_edit = 
    XtVaCreateManagedWidget("Edit Site", xmPushButtonWidgetClass, w,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 80,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 100,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 80,
			    NULL);

  disp_info->b_connect = 
    XtVaCreateManagedWidget("Connect", xmPushButtonWidgetClass, w,
			    XmNleftAttachment, XmATTACH_POSITION,
			    XmNleftPosition, 42,
			    XmNrightAttachment, XmATTACH_POSITION,
			    XmNrightPosition, 62,
			    XmNtopAttachment, XmATTACH_POSITION,
			    XmNtopPosition, 90,
			    NULL);

  cancel = XtVaCreateManagedWidget("Cancel", xmPushButtonWidgetClass, w,
				   XmNleftAttachment, XmATTACH_POSITION,
				   XmNleftPosition, 68,
				   XmNrightAttachment, XmATTACH_POSITION,
				   XmNrightPosition, 88,
				   XmNtopAttachment, XmATTACH_POSITION,
				   XmNtopPosition, 90,
				   NULL);

  XtAddCallback(cancel, XmNactivateCallback, cancelled, 
		disp_info);
}
/***************************************************************************/

void setup_cbs(SiteDispInfo *disp_info)
{
  XtAddCallback(disp_info->sitelist, XmNbrowseSelectionCallback, 
		modify_display, disp_info);
  
  XtAddCallback(disp_info->b_connect, XmNactivateCallback,
		connect_from_site_mgr, disp_info);

  XtAddCallback(disp_info->sitelist, XmNdefaultActionCallback,
		connect_from_site_mgr, disp_info);

  XtAddCallback(disp_info->b_add, XmNactivateCallback,
		add_new_site, disp_info);

  XtAddCallback(disp_info->b_edit, XmNactivateCallback,
		edit_site, disp_info);

  XtAddCallback(disp_info->b_delete, XmNactivateCallback,
		delete_site, disp_info);
  
  if(disp_info->b_addcurr) 
    XtAddCallback(disp_info->b_addcurr, XmNactivateCallback,
		  add_current_site, disp_info);
}
/***************************************************************************/

void cancelled_add(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteAddInfo *add_info = (SiteAddInfo *) client_data;
  Widget blah;
  char *dir = NULL;

  XtVaGetValues(w, XmNuserData, &dir, NULL);
  
  if(dir)
    free(dir);

  blah = add_info->dialog;
  PField_Destroy(add_info->passfield);

  XtPopdown(XtParent(blah));
  XtDestroyWidget(XtParent(blah));
  free(add_info);
}
/***************************************************************************/

void cancelled(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  Widget blah = disp_info->dialog;

  XtPopdown(XtParent(blah));
  XtDestroyWidget(XtParent(blah));
  SiteInfoList_destroy_list();
  free(disp_info);
}
/***************************************************************************/

void clear_info(SiteDispInfo *disp_info)
{

  XtVaSetValues(disp_info->site, XtVaTypedArg, XmNlabelString, XmRString, "",
		1, NULL);
  XtVaSetValues(disp_info->login, XtVaTypedArg, XmNlabelString, XmRString, "",
		1, NULL);
  XtVaSetValues(disp_info->password, XtVaTypedArg, XmNlabelString, XmRString, 
		"", 1, NULL);
  XtVaSetValues(disp_info->dir, XtVaTypedArg, XmNlabelString, XmRString, "",
		1, NULL);
  XmTextSetString(disp_info->comment, "");
}
/***************************************************************************/

void delete_site(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  int *position_list, position_count;
  XmString *items;
  char *text;
  char *file;

  if(XmListGetSelectedPos(disp_info->sitelist, &position_list, &position_count)
     == False) {
    error_dialog(w, "Please select a site to delete.");
    return;
  }

  XtVaGetValues(disp_info->sitelist, XmNselectedItems, &items, NULL);
  
  XmStringGetLtoR(*items, XmFONTLIST_DEFAULT_TAG, &text);

  if(!SiteInfoList_delete_site(text, NULL))
    error_dialog(w, "Error deleting site!");

  file = get_sitelist_filename();

  SiteInfoList_save_sites(file);

  free(text);
  free(position_list);
  clear_info(disp_info);
  load_sites(disp_info);  
}
/***************************************************************************/

void connect_from_site_mgr(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  Main_CB *main_cb = disp_info->main_cb;
  int *position_list, position_count;
  SiteInfo *site;
  XmString *items;
  char *text;

  if(XmListGetSelectedPos(disp_info->sitelist, &position_list, &position_count)
     == False)
    return;

  XtVaGetValues(disp_info->sitelist, XmNselectedItems, &items, NULL);

  XmStringGetLtoR(*items, XmFONTLIST_DEFAULT_TAG, &text);

  if(IsConnectedFromSiteMgr(main_cb)) {
    do_kill_connection(main_cb);
    load_sites(disp_info);  /* for updating last directory */
  }
  else
    do_kill_connection(main_cb);

  site = (SiteInfo *) malloc(sizeof(SiteInfo));

  if(!SiteInfoList_find_site(text, site)) {
    free(site);
    return;
  }

  XtPopdown(XtParent(disp_info->dialog));
  XtDestroyWidget(disp_info->dialog);
  free(disp_info);

  if(!(*(site->password))) {
    /* prompt for password */
    prompt_password(main_cb, site);
    return;
  }

  perform_login(main_cb, site);
  free(site);
  SiteInfoList_destroy_list();
  free(text);
  free(position_list);
}
/***************************************************************************/
    
void prompt_password(Main_CB *main_cb, SiteInfo *site)
{
  Widget prompt;
  XmString string = XmStringCreateLocalized("Enter password:");
  Arg args[1];
  PassField *pf;

  XtSetArg(args[0], XmNselectionLabelString, string);
  prompt = XmCreatePromptDialog(main_cb->main_window, "Password", args, 1);

  pf = PField_CreateFromText(XmSelectionBoxGetChild(prompt, XmDIALOG_TEXT),
			     1);
  
  XtVaSetValues(prompt, XmNuserData, main_cb, NULL);
  XtVaSetValues(XmSelectionBoxGetChild(prompt, XmDIALOG_TEXT),
		XmNuserData, pf, NULL);

  XtAddCallback(prompt, XmNokCallback, ok_password, site);
  XtAddCallback(prompt, XmNcancelCallback, cancel_password, site);
  XtManageChild(prompt);
  XmStringFree(string);
}

void ok_password(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb;
  SiteInfo *site = (SiteInfo *) client_data;
  char *passwd;
  PassField *pf;

  XtVaGetValues(w, XmNuserData, &main_cb, NULL);
  XtVaGetValues(XmSelectionBoxGetChild(w, XmDIALOG_TEXT), XmNuserData,
		&pf, NULL);

  passwd = PField_GetString(pf);

  strcpy(site->password, passwd);
  if(perform_login(main_cb, site))
    main_cb->current_state.connection_state.connect_from_site_mgr = 2;

  PField_Destroy(pf);
  free(site);
}

void cancel_password(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteInfo *site = (SiteInfo *) client_data;
  PassField *pf;

  XtVaGetValues(XmSelectionBoxGetChild(w, XmDIALOG_TEXT), XmNuserData,
		&pf, NULL);

  PField_Destroy(pf);  
  free(site);
}

int perform_login(Main_CB *main_cb, SiteInfo *site)
{
  if(do_login(main_cb, site->hostname, site->login, site->password, 
	      *(site->directory) ? site->directory : NULL,
	      (*(site->port)) ? atoi(site->port) : -1)) {
    main_cb->current_state.connection_state.connect_from_site_mgr = 1;
    strcpy(main_cb->current_state.connection_state.site_info.nickname,
	   site->nickname);
    strcpy(main_cb->current_state.connection_state.site_info.comment,
	   site->comment);
    return 1;
  }
  return 0;
}

void modify_display(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  SiteInfo site;
  XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;
  char *text;
  XmString string;

  if(!(cbs->selected_item_count))
    return;
  
  XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &text);
  SiteInfoList_find_site(text, &site);

  string = XmStringCreateLocalized(site.hostname);
  XtVaSetValues(disp_info->site, XmNlabelString, string, NULL);
  XmStringFree(string);

  string = XmStringCreateLocalized(site.login);
  XtVaSetValues(disp_info->login, XmNlabelString, string, NULL);
  XmStringFree(string);

  string = XmStringCreateLocalized("*******");
  XtVaSetValues(disp_info->password, XmNlabelString, string, NULL);
  XmStringFree(string);

  string = XmStringCreateLocalized(site.directory);
  XtVaSetValues(disp_info->dir, XmNlabelString, string, NULL);
  XmStringFree(string);

  XmTextSetString(disp_info->comment, site.comment);

  free(text);

  XtSetSensitive(disp_info->b_edit, True);
  XtSetSensitive(disp_info->b_delete, True);
  XtSetSensitive(disp_info->b_connect, True);
  
}
/***************************************************************************/
  
void load_sites(SiteDispInfo *disp_info)
{
  SiteInfo site;
  int num, i = 0;
  XmString *items;
  char *config_file;

  config_file = get_sitelist_filename();

  XmListDeleteAllItems(disp_info->sitelist);

  if(!(num = SiteInfoList_read_list(config_file)))
    return;

  items = (XmString *) malloc(sizeof(XmString) * num);

  if(!items) {
    puts("error mallocing");
    exit(1);
  }

  SiteInfoList_traverse_list(NULL, 1);
 
  while(SiteInfoList_traverse_list(&site, 0)) {
    if(i >= num) puts("we got a problem");
    items[i] = XmStringCreateLocalized(site.nickname);
    i++;
  }

  XmListAddItems(disp_info->sitelist, items, num, 0);

  for(i = 0; i < num; i++)
    XmStringFree(items[i]);

  free(items);
}
/***************************************************************************/

void addto_site_info(Widget w, SiteDispInfo *disp_info)
{
  Widget site, login, password, dir, frame, form;

  form = XtVaCreateWidget("form", xmFormWidgetClass, w,
			  XmNtopAttachment, XmATTACH_FORM,
			  XmNbottomAttachment, XmATTACH_POSITION,
			  XmNbottomPosition, 50,
			  XmNrightAttachment, XmATTACH_FORM,
			  XmNleftAttachment, XmATTACH_FORM,
			  NULL);

  site = XtVaCreateManagedWidget("Site:", xmLabelWidgetClass, form, 
				 XmNalignment, XmALIGNMENT_END,
				 XmNtopOffset, 5,
				 XmNtopAttachment, XmATTACH_FORM,
				 XmNleftAttachment, XmATTACH_FORM,
				 XmNrightAttachment, XmATTACH_POSITION,
				 XmNrightPosition, 35,
				 NULL);
  login = XtVaCreateManagedWidget("Login:", xmLabelWidgetClass, form, 
				  XmNalignment, XmALIGNMENT_END,
				  XmNtopOffset, 5,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, site,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNrightAttachment, XmATTACH_POSITION,
				  XmNrightPosition, 35,
				  NULL);
  password = XtVaCreateManagedWidget("Password:", xmLabelWidgetClass, form, 
				     XmNalignment, XmALIGNMENT_END,
				     XmNtopOffset, 5,
				     XmNtopAttachment, XmATTACH_WIDGET,
				     XmNtopWidget, login,
				     XmNleftAttachment, XmATTACH_FORM,
				     XmNrightAttachment, XmATTACH_POSITION,
				     XmNrightPosition, 35,
				     NULL);

  dir = XtVaCreateManagedWidget("Directory:", xmLabelWidgetClass, form, 
				XmNalignment, XmALIGNMENT_END,
				XmNtopOffset, 5,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, password,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_POSITION,
				XmNrightPosition, 35,
				NULL);

  disp_info->site = XtVaCreateManagedWidget("", xmLabelWidgetClass, form, 
					    XmNalignment, 
					    XmALIGNMENT_BEGINNING,
					    XmNtopOffset, 5,
					    XmNtopAttachment, XmATTACH_FORM,
					    XmNleftAttachment, XmATTACH_WIDGET,
					    XmNleftWidget, site,
					    XmNrightAttachment, XmATTACH_FORM,
					    NULL);

  disp_info->login = XtVaCreateManagedWidget("", xmLabelWidgetClass, form, 
					     XmNalignment, 
					     XmALIGNMENT_BEGINNING,
					     XmNtopOffset, 5,
					     XmNtopAttachment, XmATTACH_WIDGET,
					     XmNtopWidget, disp_info->site,
					     XmNleftAttachment,
					     XmATTACH_WIDGET,
					     XmNleftWidget, login,
					     XmNrightAttachment, XmATTACH_FORM,
					     NULL);
  disp_info->password = XtVaCreateManagedWidget("", xmLabelWidgetClass, form, 
						XmNalignment, 
						XmALIGNMENT_BEGINNING,
						XmNtopOffset, 5,
						XmNtopAttachment, 
						XmATTACH_WIDGET,
						XmNtopWidget, disp_info->login,
						XmNleftAttachment, 
						XmATTACH_WIDGET,
						XmNleftWidget, password,
						XmNrightAttachment, 
						XmATTACH_FORM,
						NULL);


  disp_info->dir = XtVaCreateManagedWidget("", xmLabelWidgetClass, form, 
					   XmNalignment, XmALIGNMENT_BEGINNING,
					   XmNtopOffset, 5,
					   XmNtopAttachment, XmATTACH_WIDGET,
					   XmNtopWidget, disp_info->password,
					   XmNleftAttachment, XmATTACH_WIDGET,
					   XmNleftWidget, dir,
					   XmNrightAttachment, XmATTACH_FORM,
					   NULL);

  XtManageChild(form);
  
  frame = XtVaCreateWidget("frame1", xmFrameWidgetClass, w,
			   XmNtopAttachment, XmATTACH_POSITION,
			   XmNtopPosition, 50,
			   XmNleftAttachment, XmATTACH_FORM,
			   XmNleftOffset, 2,
			   XmNrightAttachment, XmATTACH_FORM,
			   XmNrightOffset, 2,
			   XmNbottomAttachment, XmATTACH_FORM,
			   XmNbottomOffset, 2,
			   NULL);

  XtVaCreateManagedWidget("Comment", xmLabelWidgetClass, frame,
			  XmNchildType, XmFRAME_TITLE_CHILD,
			  XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
			  NULL);

  disp_info->comment = XtVaCreateManagedWidget("text", xmTextWidgetClass, 
					       frame, NULL);
  XtVaSetValues(disp_info->comment, XmNeditable, False, 
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNwordWrap, True,
		XmNcursorPositionVisible, False,
		NULL);

  XtManageChild(frame);
}
/***************************************************************************/

void set_port(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteAddInfo *add_info = (SiteAddInfo *) client_data;

  if(XmToggleButtonGetState(add_info->stdport)) {
    XtSetSensitive(add_info->portlabel, False);
    XtSetSensitive(add_info->portfield, False);
    XtVaSetValues(add_info->portfield, XmNvalue, "", NULL);
  }
  else {
    XtSetSensitive(add_info->portlabel, True);
    XtSetSensitive(add_info->portfield, True);
  }
}

void add_current_site(Widget w, XtPointer client_data, XtPointer call_data)
{
  SiteDispInfo *disp_info = (SiteDispInfo *) client_data;
  Main_CB *main_cb = disp_info->main_cb;
  Widget dialog;
  SiteAddInfo *add_info;
  Widget b_add, b_cancel;
  SiteInfo *site = 
    &(main_cb->current_state.connection_state.site_info);
  char *directory;
  
  add_info = (SiteAddInfo *) malloc(sizeof(SiteAddInfo));
  add_info->disp_info = disp_info;
  
  dialog = do_common_add_mod_dialog(main_cb->main_window, 0,
				    &b_add, &b_cancel, add_info);

  XmTextFieldSetString(add_info->hostfield, site->hostname);
  XmTextFieldSetString(add_info->loginfield, site->login);

  if(!strcmp(site->login, "anonymous"))
    PField_SetMode(add_info->passfield, 0);
  
  PField_SetString(add_info->passfield, site->password);
  XmTextFieldSetString(add_info->commfield, "");
  XmTextFieldSetString(add_info->nickfield, "");
  
  if(*(site->port)) { 
    XmToggleButtonSetState(add_info->nstdport, True, True);
    XmTextFieldSetString(add_info->portfield, site->port);
  }
  else
    XmToggleButtonSetState(add_info->stdport, True, True);
  
  directory = (char *) malloc(strlen(site->directory) + 1);
  strcpy(directory, site->directory);
  XtVaSetValues(b_add, XmNuserData, directory, NULL);
  XtVaSetValues(b_cancel, XmNuserData, directory, NULL);
  
  /* do_addition will delete a site (if exists) and add it again */
  XtAddCallback(b_add, XmNactivateCallback, do_addition, add_info);
  XtAddCallback(b_cancel, XmNactivateCallback, cancelled_add, add_info);
  XtAddCallback(add_info->loginfield, XmNvalueChangedCallback, 
		login_field_changed, add_info->passfield);
		
  XtManageChild(dialog);
}



