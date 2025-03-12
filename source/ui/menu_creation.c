/* menu_creation.c - Various menu creation functions */

/***************************************************************************/

#include <Xm/Xm.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>
#include <Xm/CascadeBG.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/RowColumn.h>
#include "xmftp.h"
#include "build_menu.h"
#include "menu_creation.h"
#include "operations.h"
#include "connection.h"
#include "site_mgr.h"
#include "view.h"
#include "local_remote_ops.h"
#include "options.h"
#include "remote_refresh.h"
#include "../program/misc.h"

/***************************************************************************/

/* Global variable used for popup menu */
struct {
  Main_CB *main_cb;
  Widget popup;
} PopupG;

/***************************************************************************/

void change_remote_sort(Widget w, XtPointer client_data, XtPointer call_data);
void change_remote_order(Widget w, XtPointer client_data, XtPointer call_data);

void local_process(Widget w, XtPointer client_data, XtPointer call_data);
void remote_process(Widget w, XtPointer client_data, XtPointer call_data);
void command_process(Widget w, XtPointer client_data, XtPointer call_data);
void ftp_process(Widget w, XtPointer client_data, XtPointer call_data);
void opt_process(Widget w, XtPointer client_data, XtPointer call_data);

void change_sort(Widget w, XtPointer client_data, XtPointer call_data);
void change_order(Widget w, XtPointer client_data, XtPointer call_data);

/* Pops up the popup menu */
void popup_popup(Widget w, XtPointer client_data, XEvent *event);

/***************************************************************************/

void create_Popup_menu(Main_CB *main_cb)
{
  Widget popup;

  MenuItem popup_items[] = {
    { "FTP Connection", &xmLabelGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "_sep1", &xmSeparatorGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "Site Manager", &xmPushButtonGadgetClass, 'S', NULL, NULL, site_manager,
      main_cb, NULL },
    { "Connect", &xmPushButtonGadgetClass, 'C', NULL, NULL, quick_connect,
      main_cb, NULL },
    { "Reconnect", &xmPushButtonGadgetClass, 'R', NULL, NULL, reconnect,
      main_cb, NULL },
    { "Disconnect", &xmPushButtonGadgetClass, 'D', NULL, NULL, disconnect,
      main_cb, NULL },
    { "_sep2", &xmSeparatorGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "FTP Commands", &xmLabelGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "_sep3", &xmSeparatorGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "Download", &xmPushButtonGadgetClass, 'D', NULL, NULL, download,
      main_cb, NULL },
    { "Upload", &xmPushButtonGadgetClass, 'U', NULL, NULL, upload,
      main_cb, NULL },
    { "_sep4", &xmSeparatorGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "Other", &xmLabelGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "_sep5", &xmSeparatorGadgetClass, 0, NULL, NULL, NULL, NULL, NULL },
    { "View Local", &xmPushButtonGadgetClass, 'V', NULL, NULL, view_local,
      main_cb, NULL },
    { "View Remote", &xmPushButtonGadgetClass, 'V', NULL, NULL, view_remote,
      main_cb, NULL },
    { NULL }
  };

  popup = BuildMenu(main_cb->main_window, XmMENU_POPUP, "Stuff", 0,
		    True, popup_items);

  PopupG.main_cb = main_cb;
  PopupG.popup = popup;
  XtAddEventHandler(main_cb->main_window, ButtonPressMask,
		    False, (XtEventHandler) popup_popup, NULL);
}
/***************************************************************************/

/* Pops up the popup menu */
void popup_popup(Widget w, XtPointer client_data, XEvent *event)
{
  XButtonPressedEvent *bevent = (XButtonPressedEvent *) event;

  if(bevent->button != 3)
    return;

  XmMenuPosition(PopupG.popup, bevent);
  XtManageChild(PopupG.popup);
}
/***************************************************************************/
  
void create_Local_menu(Widget menubar, Main_CB *main_cb)
{
  Widget menu, pullright, pullright2;
  XmString view, md, del, ren, sort, name, time, ordering, ascend,
    descend;

  view = XmStringCreateLocalized("View");
  md = XmStringCreateLocalized("Make Directory");
  del = XmStringCreateLocalized("Delete");
  ren = XmStringCreateLocalized("Rename");
  sort = XmStringCreateLocalized("Sorting");
  name = XmStringCreateLocalized("by Name");
  time = XmStringCreateLocalized("by Time");
  ordering = XmStringCreateLocalized("Sort Order");
  ascend = XmStringCreateLocalized("Ascending");
  descend = XmStringCreateLocalized("Descending");

  menu = XmVaCreateSimplePulldownMenu(menubar, "local_menu", 2, local_process, 
				      XmVaPUSHBUTTON, view, 'V', NULL,
				      NULL,
				      XmVaPUSHBUTTON, md, 'M', NULL, NULL,
				      XmVaPUSHBUTTON, del, 'D', NULL, NULL,
				      XmVaPUSHBUTTON, ren, 'R', NULL, NULL,
				      XmVaCASCADEBUTTON, sort, 'S', NULL,
				      NULL, NULL);

  XtVaSetValues(menu, XmNuserData, main_cb, NULL);

  pullright = XmVaCreateSimplePulldownMenu(menu, "pull", 4, change_sort,
					   XmVaRADIOBUTTON, name, 'N', NULL,
					   NULL,
					   XmVaRADIOBUTTON, time, 'T', NULL,
					   NULL,
					   XmVaSEPARATOR,
					   XmNradioBehavior, True,
					   XmNradioAlwaysOne, True,
					   XmVaCASCADEBUTTON, ordering, 'O',
					   NULL, NULL,
					   NULL);
  
  pullright2 = XmVaCreateSimplePulldownMenu(pullright, "order", 3, 
					    change_order,
					    XmVaRADIOBUTTON, ascend, 'A',
					    NULL, NULL,
					    XmVaRADIOBUTTON, descend, 'D',
					    NULL, NULL,
					    XmNradioBehavior, True,
					    XmNradioAlwaysOne, True,
					    NULL);

  XtVaSetValues(XtNameToWidget(pullright2, "button_0"), XmNset, True, NULL);
  XtVaSetValues(XtNameToWidget(pullright, "button_0"), XmNset, True, NULL);
  
  XmStringFree(view);
  XmStringFree(md);
  XmStringFree(del);
  XmStringFree(ren);
  XmStringFree(sort);
  XmStringFree(ascend);
  XmStringFree(descend);
  XmStringFree(name);
  XmStringFree(time);
  XmStringFree(ordering);
}
/***************************************************************************/

void create_Remote_menu(Widget menubar, Main_CB *main_cb)
{
  Widget menu, pullright, pullright2;
  XmString view, md, del, ren, sort, name, time, ordering, ascend,
    descend, site;

  view = XmStringCreateLocalized("View");
  md = XmStringCreateLocalized("Make Directory");
  del = XmStringCreateLocalized("Delete");
  ren = XmStringCreateLocalized("Rename");
  site = XmStringCreateLocalized("Site Command");
  sort = XmStringCreateLocalized("Sorting");
  name = XmStringCreateLocalized("by Name");
  time = XmStringCreateLocalized("by Time");
  ordering = XmStringCreateLocalized("Sort Order");
  ascend = XmStringCreateLocalized("Ascending");
  descend = XmStringCreateLocalized("Descending");

  menu = XmVaCreateSimplePulldownMenu(menubar, "rem_menu", 3, remote_process, 
				      XmVaPUSHBUTTON, view, 'V', NULL,
				      NULL,
				      XmVaPUSHBUTTON, md, 'M', NULL, NULL,
				      XmVaPUSHBUTTON, del, 'D', NULL, NULL,
				      XmVaPUSHBUTTON, ren, 'R', NULL, NULL,
				      XmVaPUSHBUTTON, site, 'C', NULL, NULL,
				      XmVaCASCADEBUTTON, sort, 'S', NULL,
				      NULL, NULL);

  XtVaSetValues(menu, XmNuserData, main_cb, NULL);

  pullright = XmVaCreateSimplePulldownMenu(menu, "pull", 5, change_remote_sort,
					   XmVaRADIOBUTTON, name, 'N', NULL,
					   NULL,
					   XmVaRADIOBUTTON, time, 'T', NULL,
					   NULL,
					   XmVaSEPARATOR,
					   XmNradioBehavior, True,
					   XmNradioAlwaysOne, True,
					   XmVaCASCADEBUTTON, ordering, 'O',
					   NULL, NULL,
					   NULL);
  
  pullright2 = XmVaCreateSimplePulldownMenu(pullright, "order", 3, 
					    change_remote_order,
					    XmVaRADIOBUTTON, ascend, 'A',
					    NULL, NULL,
					    XmVaRADIOBUTTON, descend, 'D',
					    NULL, NULL,
					    XmNradioBehavior, True,
					    XmNradioAlwaysOne, True,
					    NULL);

  XtVaSetValues(XtNameToWidget(pullright2, "button_0"), XmNset, True, NULL);
  XtVaSetValues(XtNameToWidget(pullright, "button_0"), XmNset, True, NULL);

  XmStringFree(view);
  XmStringFree(md);
  XmStringFree(del);
  XmStringFree(ren);
  XmStringFree(sort);
  XmStringFree(ascend);
  XmStringFree(descend);
  XmStringFree(name);
  XmStringFree(time);
  XmStringFree(ordering);
  XmStringFree(site);
}
/***************************************************************************/

void create_Commands_menu(Widget menubar, Main_CB *main_callback)
{
  Widget menu;
  XmString dload, uload;

  dload = XmStringCreateLocalized("Download");
  uload = XmStringCreateLocalized("Upload");

  menu = XmVaCreateSimplePulldownMenu(menubar, "comm_menu", 1, 
				      command_process, 
				      XmVaPUSHBUTTON, dload, 'D', NULL, NULL,
				      XmVaPUSHBUTTON, uload, 'U', NULL, NULL,
				      NULL);
  XtVaSetValues(menu, XmNuserData, main_callback, NULL);

  XmStringFree(dload);
  XmStringFree(uload);
}  
/***************************************************************************/

void create_FTP_menu(Widget menubar, Main_CB *main_callback)
{
  Widget menu;
  XmString sman, conn, reconn, disconn, xit;

  sman = XmStringCreateLocalized("Site Manager");
  conn = XmStringCreateLocalized("Connect");
  reconn = XmStringCreateLocalized("Reconnect");
  disconn = XmStringCreateLocalized("Disconnect");
  xit = XmStringCreateLocalized("Exit");

  menu = XmVaCreateSimplePulldownMenu(menubar, "ftp_menu", 0,
				      ftp_process,
				      XmVaPUSHBUTTON, sman, 'S', NULL, NULL,
				      XmVaPUSHBUTTON, conn, 'C', NULL, NULL,
				      XmVaPUSHBUTTON, reconn, 'R', NULL, NULL,
				      XmVaPUSHBUTTON, disconn, 'D', NULL,
				      NULL,
				      XmVaSEPARATOR,
				      XmVaPUSHBUTTON, xit, 'x', NULL, NULL,
				      NULL);

  XtVaSetValues(menu, XmNuserData, main_callback, NULL);

  XmStringFree(sman);
  XmStringFree(conn);
  XmStringFree(reconn);
  XmStringFree(disconn);
  XmStringFree(xit);

}
/***************************************************************************/

void create_Help_menu(Widget menubar, Main_CB *main_callback)
{
  Widget help;

  MenuItem Help_menu[] = { { NULL } };
  
  help = BuildMenu(menubar, XmMENU_PULLDOWN, "Help", 'H', False, Help_menu);

  XtVaSetValues(menubar, XmNmenuHelpWidget, help, NULL);
}  
/***************************************************************************/

void create_Options_menu(Widget menubar, Main_CB *main_cb)
{
  Widget menu;
  XmString settings;

  settings = XmStringCreateLocalized("General settings...");

  menu = XmVaCreateSimplePulldownMenu(menubar, "opt_menu", 4, opt_process,
				      XmVaPUSHBUTTON, settings, 'G', NULL,
				      NULL,
				      NULL);
  XtVaSetValues(menu, XmNuserData, main_cb, NULL);

  XmStringFree(settings);
}

void local_process(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb;

  main_cb = PopupG.main_cb;

  switch(item) {
  case 0:
    view_local(w, main_cb, call_data);
    break;
  case 1:
    mkdir_local(w, main_cb, call_data);
    break;
  case 2:
    delete_local(w, main_cb, call_data);
    break;
  case 3:
    rename_local(w, main_cb, call_data);
    break;
  default:
    break;
  }
}

void remote_process(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;

  switch(item) {
  case 0:
    view_remote(w, main_cb, call_data);
    break;
  case 1:
    mkdir_remote(w, main_cb, call_data);
    break;
  case 2:
    delete_remote(w, main_cb, call_data);
    break;
  case 3:
    rename_remote(w, main_cb, call_data);
    break;
  case 4:
    site_remote(w, main_cb, call_data);
    break;
  default:
    break;
  }
}

void command_process(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;

  switch(item) {
  case 0:
    download(w, main_cb, call_data);
    break;
  case 1:
    upload(w, main_cb, call_data);
    break;
  default:
    break;
  }
}
    
void ftp_process(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;

  switch(item) {
  case 0:
    site_manager(w, main_cb, call_data);
    break;
  case 1:
    quick_connect(w, main_cb, call_data);
    break;
  case 2:
    reconnect(w, main_cb, call_data);
    break;
  case 3:
    disconnect(w, main_cb, call_data);
    break;
  case 4:
    die_all(w, main_cb, call_data);
    break;
  default:
    break;
  }
}

void opt_process(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;

  switch(item) {
  case 0:
    general_opts(w, PopupG.main_cb, call_data);
    break;
  default:
    break;
  }
}
    
void change_sort(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) 
    call_data;

  switch(item) {
  case 0:
    if(cbs->set && main_cb->current_state.dir_state.sorting != NAME) {
      main_cb->current_state.dir_state.sorting = NAME;
      refresh_local_dirlist(main_cb, ".");
    }
    break;
  case 1:
    if(cbs->set && main_cb->current_state.dir_state.sorting != DATE) {
      main_cb->current_state.dir_state.sorting = DATE;
      refresh_local_dirlist(main_cb, ".");
    }
    break;
  default:
    break;
  }
}

void change_order(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) 
    call_data;

  /* XtVaGetValues(w, XmNuserData, &main_cb); */

  switch(item) {
  case 0:
    if(cbs->set && main_cb->current_state.dir_state.order != ASCENDING) {
      main_cb->current_state.dir_state.order = ASCENDING;
      refresh_local_dirlist(main_cb, ".");
    }
    break;
  case 1:
    if(cbs->set && main_cb->current_state.dir_state.order != DESCENDING) {
      main_cb->current_state.dir_state.order = DESCENDING;
      refresh_local_dirlist(main_cb, ".");
    }
    break;
  default:
    break;
  }
}

void change_remote_order(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) 
    call_data;

  switch(item) {
  case 0:
    if(cbs->set && main_cb->current_state.rdir_state.order != ASCENDING) {
      main_cb->current_state.rdir_state.order = ASCENDING;
      if(main_cb->current_state.connection_state.connected)
	sort_remote_dirlist(main_cb);
    }
    break;
  case 1:
    if(cbs->set && main_cb->current_state.rdir_state.order != DESCENDING) {
      main_cb->current_state.rdir_state.order = DESCENDING;
      if(main_cb->current_state.connection_state.connected)
	sort_remote_dirlist(main_cb);
    }
    break;
  default:
    break;
  }
}

void change_remote_sort(Widget w, XtPointer client_data, XtPointer call_data)
{
  int item = (int) client_data;
  Main_CB *main_cb = PopupG.main_cb;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) 
    call_data;

  switch(item) {
  case 0:
    if(cbs->set && main_cb->current_state.rdir_state.sorting != NAME) {
      main_cb->current_state.rdir_state.sorting = NAME;
      if(main_cb->current_state.connection_state.connected)
	sort_remote_dirlist(main_cb);
    }
    break;
  case 1:
    if(cbs->set && main_cb->current_state.rdir_state.sorting != DATE) {
      main_cb->current_state.rdir_state.sorting = DATE;
      if(main_cb->current_state.connection_state.connected)
	sort_remote_dirlist(main_cb);
    }
    break;
  default:
    break;
  }
}

