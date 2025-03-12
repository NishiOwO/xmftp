/* view.c - View text file operations */

/************************************************************************/

#include <Xm/Text.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/List.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "xmftp.h"
#include "view.h"
#include "../program/misc.h"
#include "transfer.h"
#include "download.h"
#include "operations.h"

/************************************************************************/

#ifndef FILENAME_MAX
#define FILENAME_MAX LINE_MAX
#endif

/************************************************************************/

Widget create_view_window(Main_CB *main_cb);

/* Read the file into the text widget, return 1 if ok, 0 on failure */
int read_file(Widget text_w, char *file, FILE *fp);

/* View all-purpose, where is 1 for remote, 0 for local */
void view(Widget w, Main_CB *main_cb, int where);

/************************************************************************/

void view_local(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  view(w, main_cb, 0);
}
/************************************************************************/

void view_remote(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  view(w, main_cb, 1);
}
/************************************************************************/

void viewFP(Widget w, Main_CB *main_cb, FILE *fp)
{
  Widget output, text;

  output = create_view_window(main_cb);
  XtVaGetValues(output, XmNuserData, &text, NULL);

  if(read_file(text, NULL, fp)) {
    XtManageChild(text);
    XtManageChild(output);
  }
}

Widget create_view_window(Main_CB *main_cb)
{
  Widget output, text;
  Arg args[10];
  int n = 0;

  output = XmCreateFormDialog(main_cb->main_window, "View", NULL, 0);
  
  XtSetArg(args[n], XmNrows, 24); n++;
  XtSetArg(args[n], XmNcolumns, 80); n++;
  XtSetArg(args[n], XmNeditable, False); n++;
  XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;

  text = XmCreateScrolledText(output, "text_w", args, n);
  XtVaSetValues(output, XmNuserData, text, NULL);

  XtVaCreateManagedWidget("OK", xmPushButtonWidgetClass, output,
			  XmNleftAttachment, XmATTACH_POSITION,
			  XmNleftPosition, 40,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 60,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, text,
			  XmNtopOffset, 20,
			  XmNbottomAttachment, XmATTACH_FORM,
			  XmNbottomOffset, 20,
			  NULL);
  return output;
}  

void view(Widget w, Main_CB *main_cb, int where)
{
  Widget output, text;
  DirSection section = where ? main_cb->remote_section : 
    main_cb->local_section;
  int *position_list, position_count;
  char *curr_dir, *temp_dir;
  XferInfo *xfer_info;
  int number, error = 0;

  /* Make sure user does not select . or .. for renaming */
  if((XmListPosSelected(section.dir_list, 1) == True) ||
     (XmListPosSelected(section.dir_list, 2) == True)) {
    error_dialog(w, 
		 "You cannot view a directory.");
    return;
  }

  if(XmListGetSelectedPos(section.dir_list, &position_list,
			  &position_count) == False) {
    if(where) 
      info_dialog(w, "No remote file was selected to view.");
    else
      info_dialog(w, "No local file was selected to view.");      
    return;
  }
  
  if(section.files[*position_list - 1].directory) {
    error_dialog(w, "You cannot view a directory.");
    free(position_list);
    return;
  }

  free(position_list);

  if(position_count != 1) {
    if(where) 
      info_dialog(w, "Please select only one remote file to view.");
    else
      info_dialog(w, "Please select only one local file to view.");      
    return;
  }

  xfer_info = get_selected_files_to_XferInfo(main_cb, &number, where);

  if(!xfer_info)
    return;
 
  if(xfer_info->file.directory) {
    error_dialog(w, "You cannot view a directory.");
    destroy_XferInfo(&xfer_info, number);
    return;
  }

  if(number > 1) {
    destroy_XferInfo(&xfer_info, number);
    return;
  }

  /* need to download file now if remote (where == 1)*/      
  if(where) {
    /* remember where we are */
    curr_dir = get_local_dir();

    temp_dir = tmpnam(NULL);
    make_dir(temp_dir);

    if(!change_dir(temp_dir)) {
      error_dialog(w, "An error occured setting up a temporary directory.");
      free(curr_dir);
      destroy_XferInfo(&xfer_info, number);
      return;
    }

    perform_download(main_cb, NULL, xfer_info, number);
  }
  
  output = create_view_window(main_cb);
  XtVaGetValues(output, XmNuserData, &text, NULL);

  if(!read_file(text, xfer_info->file.filename, NULL)) {
    error_dialog(main_cb->main_window, "Error viewing file.");
    error = 1;
  }
    
  if(where) {
    /* remove temp file */
    delete_local_file(xfer_info->file.filename);
    change_dir(curr_dir);
    remove_local_dir(temp_dir);
    free(curr_dir);
  }

  destroy_XferInfo(&xfer_info, number);

  if(error)
    return;

  XtManageChild(text);
  XtManageChild(output);
}
/************************************************************************/
  
int read_file(Widget text_w, char *file, FILE *fpread)
{
  char *text;
  FILE *fp;
  unsigned long int size;

  if(!fpread) {
    if(!local_file_exists(file, &size))
      return 0;
    
    if(!(fp = fopen(file, "r")))
      return 0;
  }
  else {
    fp = fpread;
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);
  }
    
  if(!(text = (char *) malloc((sizeof(char) * size) + 1))) {
    fprintf(stderr, "Could not allocate enough memory :(\n");
    if(!fpread)
      fclose(fp);
    return 0;
  }

  fread(text, sizeof(char), size + 1, fp);
  text[size] = 0;

  XmTextSetString(text_w, text);

  free(text);
  if(!fpread)
    fclose(fp);
  return 1;
}
/************************************************************************/
