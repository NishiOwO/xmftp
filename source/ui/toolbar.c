/* toolbar.c - toolbar creation operations */

/*************************************************************************/

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/MainW.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <stdio.h>
#include <stdlib.h>
#include "xmftp.h"
#include "operations.h"
#include "connection.h"
#include "view.h"
#include "site_mgr.h"
#include "pixmap.h"

/*************************************************************************/

void add_toolbar_section(Widget toolbar_form, Main_CB *main_cb)
{
  Pixmap pix;
  Widget button;
  
  pix = load_xmftp_pixmap(main_cb, SITE_MGR, toolbar_form);
  
  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);
  
  XtAddCallback(button, XmNactivateCallback, site_manager, main_cb);

  pix = load_xmftp_pixmap(main_cb, CONNECT, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);
  
  XtAddCallback(button, XmNactivateCallback, quick_connect, main_cb);
  
  pix = load_xmftp_pixmap(main_cb, DISCONNECT, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);

  XtAddCallback(button, XmNactivateCallback, disconnect, main_cb);


  pix = load_xmftp_pixmap(main_cb, RECONNECT, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);

  XtAddCallback(button, XmNactivateCallback, reconnect, main_cb);

  pix = load_xmftp_pixmap(main_cb, DOWNLOAD, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNleftOffset, 30,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);
  
  XtAddCallback(button, XmNactivateCallback, download, main_cb);

  pix = load_xmftp_pixmap(main_cb, UPLOAD, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);

  XtAddCallback(button, XmNactivateCallback, upload, main_cb);

  pix = load_xmftp_pixmap(main_cb, VIEW, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);

  XtAddCallback(button, XmNactivateCallback, view_remote, main_cb);

  pix = load_xmftp_pixmap(main_cb, QUIT, toolbar_form);

  button = XtVaCreateManagedWidget("label", xmPushButtonWidgetClass, 
				   toolbar_form,
				   XmNlabelType, XmPIXMAP,
				   XmNlabelPixmap, pix,
				   XmNleftAttachment, XmATTACH_WIDGET,
				   XmNleftWidget, button,
				   XmNleftOffset, 30,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   NULL);

  XtAddCallback(button, XmNactivateCallback, die_all, main_cb);

} 
/*************************************************************************/

