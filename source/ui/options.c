#include "xmftp.h"
#include "options.h"
#include <Xm/RowColumn.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "../program/store_options.h"
#include "fill_filelist.h"
#include "operations.h"
#include "remote_refresh.h"

void toggled(Widget w, XtPointer client_data, XtPointer call_data);
void timeout_chg(Widget w, XtPointer client_data, XtPointer call_data);
void process_revert_options(Widget w, XtPointer client_data, XtPointer 
			    call_data);
void process_save_options(Widget w, XtPointer client_data, XtPointer
			  call_data);

static char *rtm_info = 
"RTMode means reliable transfer mode. If enabled and no data is recieved\n"
"during a transfer within the time amount specified by 'Network Timeout',\n"
"an attempt is made to redial the site and continue the transfer if the\n"
"remote system is supported. This feature is buggy, use at YOUR OWN RISK!";

static char *toggles[] = {
  "Show connection messages", "Use Large font for directory listing",
  "Don't show hidden local files", "Don't show hidden remote files",
  "Don't crop long filenames", "RTMode Enabled" };


void general_opts(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  Widget form_dlg, net_timeout_tfw, frame_tgls, rowcol,
    ok_btn, cancel_btn, tgl_btn;
  XmString info;
  char templine[10];
  OptionState *options = &(main_cb->current_state.options);
  int i, tgl_state;

  form_dlg = XmCreateFormDialog(main_cb->main_window, "General Settings",
				NULL, 0);

  XtVaSetValues(form_dlg, XmNresizePolicy, XmRESIZE_NONE, 
		XmNwidth, 500,
		XmNheight, 350,
		NULL);

  XtVaCreateManagedWidget("Network Timeout:", xmLabelWidgetClass, form_dlg,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNtopAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopOffset, 10,
			  NULL);

  net_timeout_tfw = XtVaCreateManagedWidget("time", xmTextFieldWidgetClass,
					    form_dlg,
					    XmNcolumns, 5,
					    XmNmaxLength, 5,
					    XmNleftAttachment, 
					    XmATTACH_POSITION,
					    XmNleftPosition, 50,
					    XmNtopAttachment, XmATTACH_FORM,
					    XmNtopOffset, 8,
					    NULL);

  frame_tgls = XtVaCreateWidget("frame1", xmFrameWidgetClass,
				form_dlg,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, net_timeout_tfw,
				XmNtopOffset, 10,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_FORM,
				XmNleftOffset, 5,
				XmNrightOffset, 5,
				NULL);

  XtVaCreateManagedWidget("Toggles", xmLabelWidgetClass, frame_tgls,
			  XmNchildType, XmFRAME_TITLE_CHILD,
			  XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
			  NULL);

  rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, frame_tgls,
			    XmNorientation, XmVERTICAL,
			    NULL);

  for(i = 0; i < XtNumber(toggles); i++) {
    tgl_btn = XtVaCreateManagedWidget(toggles[i], xmToggleButtonWidgetClass,
				      rowcol,
				      XmNindicatorType, XmN_OF_MANY,
				      NULL);

    XtAddCallback(tgl_btn, XmNvalueChangedCallback, toggled, (XtPointer) i);
    XtVaSetValues(tgl_btn, XmNuserData, main_cb, NULL);
 
    switch(i) {
    case 0:
      tgl_state = (options->flags & OptionShowConnMsg) ? 1 : 0;
      break;
    case 1:
      tgl_state = (options->flags & OptionLargeFont) ? 1 : 0;
      break;
    case 2:
      tgl_state = (options->flags & OptionDontShowHidLoc) ? 1 : 0;
      break;
    case 3:
      tgl_state = (options->flags & OptionDontShowHidRem) ? 1 : 0;
      break;      
    case 4:
      tgl_state = (options->flags & OptionDontCropFNames) ? 1 : 0;
      break;
    case 5:
      tgl_state = (options->flags & OptionRTMode) ? 1 : 0;
      break;
    default:
      tgl_state = 0;
    }
    XmToggleButtonSetState(tgl_btn, tgl_state, False);
  }
  
  XtManageChild(rowcol);
  XtManageChild(frame_tgls);

  info = XmStringCreateLtoR(rtm_info, XmFONTLIST_DEFAULT_TAG);
  XtVaCreateManagedWidget("info", xmLabelWidgetClass, form_dlg,
			  XmNlabelString, info,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, frame_tgls,
			  XmNtopOffset, 5,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_FORM,
			  NULL);
  XmStringFree(info);

  ok_btn = XtVaCreateManagedWidget("OK", xmPushButtonWidgetClass, form_dlg,
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNbottomOffset, 10,
				   XmNleftAttachment, XmATTACH_POSITION,
				   XmNleftPosition, 20,
				   XmNrightAttachment, XmATTACH_POSITION,
				   XmNrightPosition, 40,
				   NULL);

  cancel_btn = XtVaCreateManagedWidget("Cancel", xmPushButtonWidgetClass, 
				       form_dlg,
				       XmNbottomAttachment, XmATTACH_FORM,
				       XmNbottomOffset, 10,
				       XmNleftAttachment, XmATTACH_POSITION,
				       XmNleftPosition, 60,
				       XmNrightAttachment, XmATTACH_POSITION,
				       XmNrightPosition, 80,
				       NULL);

  
  /* Setup current settings */
  sprintf(templine, "%d", options->network_timeout);
  XmTextFieldSetString(net_timeout_tfw, templine);
  
  /* setup callbacks */

  XtAddCallback(net_timeout_tfw, XmNvalueChangedCallback, timeout_chg,
		options);
  XtAddCallback(ok_btn, XmNactivateCallback, process_save_options,
		main_cb);
  XtAddCallback(cancel_btn, XmNactivateCallback, process_revert_options,
		options);

  XtManageChild(form_dlg);
}

void process_save_options(Widget w, XtPointer client_data, XtPointer
			  call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  OptionState *options = &(main_cb->current_state.options);
  OptionState old_options;

  load_options(&old_options);
  save_options(options);

  if((old_options.flags & OptionDontShowHidLoc) !=
     (options->flags & OptionDontShowHidLoc)) {
    /* Must do REFRESH of local dirlist */
    refresh_local_dirlist(main_cb, ".");
  }

  if((old_options.flags & OptionDontShowHidRem) !=
     (options->flags & OptionDontShowHidRem)) {
    /* Must do REFRESH of remote dirlist */
    if(main_cb->current_state.connection_state.connected)
      refresh_remote_dirlist(main_cb);
  }
  
  if(((old_options.flags & OptionLargeFont) !=
     (options->flags & OptionLargeFont)) ||
     ((old_options.flags & OptionDontCropFNames) !=
     (options->flags & OptionDontCropFNames))) {
    /* If font size change, UPDATE (not refresh) displays */
    XmListDeleteAllItems(main_cb->local_section.dir_list);
    XmListDeleteAllItems(main_cb->remote_section.dir_list);
    fill_slist(main_cb->local_section.files, main_cb->local_section.dir_list,
	       main_cb->local_section.num_files, 
	       (main_cb->current_state.options.flags) & OptionLargeFont,
	       (main_cb->current_state.options.flags) & OptionDontCropFNames);
    /* only refresh remote if connected */
    if(main_cb->current_state.connection_state.connected) {
      fill_slist(main_cb->remote_section.files, 
		 main_cb->remote_section.dir_list,
		 main_cb->remote_section.num_files, 
		 (main_cb->current_state.options.flags) & OptionLargeFont,
		 (main_cb->current_state.options.flags) & 
		 OptionDontCropFNames);
      
    }
  }
}

void process_revert_options(Widget w, XtPointer client_data, XtPointer 
			    call_data)
{
  OptionState *options = (OptionState *) client_data;
  
  load_options(options);
}

void toggled(Widget w, XtPointer client_data, XtPointer call_data)
{
  int which = (int) client_data;
  Main_CB *main_cb;
  OptionState *options;
  XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct *) 
    call_data;

  XtVaGetValues(w, XmNuserData, &main_cb, NULL);
  options = &(main_cb->current_state.options);  
  
  switch(which) {
  case 0:
    if(cbs->set)
      options->flags |= OptionShowConnMsg;
    else
      options->flags &= ~(OptionShowConnMsg);
    break;
  case 1:
    if(cbs->set)
      options->flags |= OptionLargeFont;
    else
      options->flags &= ~(OptionLargeFont);
    break;
  case 2:
    if(cbs->set)
      options->flags |= OptionDontShowHidLoc;
    else
      options->flags &= ~(OptionDontShowHidLoc);
    main_cb->current_state.dir_state.skip_hidden = 
      options->flags & OptionDontShowHidLoc;
    break;
  case 3:
    if(cbs->set)
      options->flags |= OptionDontShowHidRem;
    else
      options->flags &= ~(OptionDontShowHidRem);
    main_cb->current_state.rdir_state.skip_hidden = 
      options->flags & OptionDontShowHidRem;
    break;
  case 4:
    if(cbs->set)
      options->flags |= OptionDontCropFNames;
    else
      options->flags &= ~(OptionDontCropFNames);
    break;
  case 5:
    if(cbs->set)
      options->flags |= OptionRTMode;
    else
      options->flags &= ~(OptionRTMode);
    break;
  default:
    break;
  }
}

void timeout_chg(Widget w, XtPointer client_data, XtPointer call_data)
{
  OptionState *options = (OptionState *) client_data;
  char *value;

  value = XmTextFieldGetString(w);

  options->network_timeout = (unsigned) atoi(value);
}
