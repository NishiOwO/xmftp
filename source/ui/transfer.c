/* transfer.c - common transfer operations */

/**************************************************************************/

#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#include <Xm/PushB.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include "xmftp.h"
#include "transfer.h"
#include "../program/misc.h"
#include "../program/ftp.h"
#include "operations.h"
#include "connection.h"

/**************************************************************************/

/* For recursion into subdirectories locally or remotely and
 * modifying XferInfo accordingly (adding files)
 */
int process_subdir(Main_CB *main_cb, char *filename, 
		   XferInfo **dirtree, int *num_in_dirtree,
		   int remote);

/**************************************************************************/

XferInfo *get_selected_files_to_XferInfo(Main_CB *main_cb, 
						int *number, int remote)
{
  DirSection section = remote ? main_cb->remote_section :
    main_cb->local_section;
  FileList *filelist = section.files;
  XferInfo *xfer_info;
  int *position_list;
  int position_count;
  int i, index;

  if(XmListGetSelectedPos(section.dir_list, &position_list,
			  &position_count) == False)
    return NULL;

  /* here the user selected "./" and we process this and ignore other
   * selections. A selection of "." means everything in the current dir
   */

  if(*position_list == 1) 
    position_count = 1;
  else
    if(*position_list == 2) {
      /* Cannot select "../" */
      free(position_list);
      return NULL;
    }
  
  xfer_info = (XferInfo *) malloc(sizeof(XferInfo) * position_count);

  if(!xfer_info) {
    fprintf(stderr, "Error mallocing XferInfo list\n");
    exit(1);
  }

  /* init list */
  
  for(i = 0; i < position_count; i++) {
    index = position_list[i]-1;
    xfer_info[i].file = filelist[index];

    xfer_info[i].file.filename = 
      (char *) malloc(sizeof(char) * (strlen(filelist[index].filename)+1));

    if(!(xfer_info[i].file.filename)) {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }

    strcpy(xfer_info[i].file.filename, filelist[index].filename);
    xfer_info[i].num_in_dirtree = 0;
    xfer_info[i].dirtree = NULL;
  }
  free(position_list);
  *number = position_count;

  /* now we must take care of subdirectories sigh */
  for(i = 0; i < position_count; i++) {

    if(xfer_info[i].file.directory) {

      process_subdir(main_cb, xfer_info[i].file.filename, 
		     &(xfer_info[i].dirtree), 
		     &(xfer_info[i].num_in_dirtree), remote);
    }
  }

  return xfer_info;
}
/**************************************************************************/
  
int process_subdir(Main_CB *main_cb, char *filename, 
			  XferInfo **dirtree, int *num_in_dirtree,
			  int remote)
{
  FileList *files;
  XferInfo *xfer_info;
  unsigned int num_files;
  int i;
  int dot;

  dot = !strcmp(filename, "./");


  /* we dont need to chdir . */
  if(!dot) {
    if(remote) {
      if(!change_remote_dir(filename))
	return 0;
    }
    else {
      if(!change_dir(filename))
	return 0;
    }
  }

  files = remote ?
    generate_remote_filelist(&num_files, 
			     main_cb->current_state.dir_state,
			     main_cb->current_state.connection_state.
			     site_info.system_type) :
    generate_local_filelist(".", &num_files, main_cb->current_state.dir_state);
  
  if(!files) {
    fprintf(stderr, "Error mallocing filelist\n");
    exit(1);
  }

  /* skip . and .. */
  *dirtree = (XferInfo *) malloc(sizeof(XferInfo) * (num_files-2));
  *num_in_dirtree = num_files - 2;

  if(!(*dirtree)) {
    fprintf(stderr, "Error mallocing XferInfo\n");
    exit(1);
  }

  xfer_info = *dirtree;

  for(i = 2; i < num_files; i++) {
    xfer_info[i-2].file = files[i];

    xfer_info[i-2].file.filename = 
      (char *) malloc(sizeof(char) * (strlen(files[i].filename)+1));

    if(!(xfer_info[i-2].file.filename)) {
      fprintf(stderr, "malloc error\n");
      exit(1);
    }
    
    strcpy(xfer_info[i-2].file.filename, files[i].filename);

    xfer_info[i-2].dirtree = NULL;
    xfer_info[i-2].num_in_dirtree = 0;
  }

  destroy_filelist(&files, num_files);

  for(i = 0; i < (num_files-2); i++) {
    if(xfer_info[i].file.directory)
      process_subdir(main_cb, xfer_info[i].file.filename,
		     &(xfer_info[i].dirtree),
		     &(xfer_info[i].num_in_dirtree), remote);
  }

  if(!dot) {
    if(remote) {
      if(!change_remote_dir(".."))
	return 0;
    }
    else {
      if(!change_dir(".."))
	return 0;
    }
  }
  return 1;
}
/**************************************************************************/

Output transfer_common_dialog(Main_CB *main_cb, XferInfo *xfer_info,
			      int num_files)
{
  Widget output, file_label, fscale, tscale, file_position_label,
    speed, stop, total_position_label, file_eta_label, total_eta_label,
    file_remaining_label, total_remaining_label;
  unsigned long int total_bytes;
  int total_files = 0;
  XmString title;
  char templine[LINE_MAX];
  Output xfer_output;

  total_bytes = get_total_bytes(xfer_info, num_files, &total_files);

  output = XmCreateFormDialog(main_cb->main_window, "Transferring", NULL, 0);

  XtVaSetValues(output,
		XmNautoUnmanage, False,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNwidth, 300,
		XmNresizePolicy, XmRESIZE_NONE,
		NULL);
  
  
  XtVaCreateManagedWidget("Filename:", xmLabelWidgetClass, output,
                          XmNalignment, XmALIGNMENT_END,
                          XmNleftAttachment, XmATTACH_FORM,
                          XmNtopAttachment, XmATTACH_FORM,
                          XmNrightAttachment, XmATTACH_POSITION,
                          XmNrightPosition, 50,
                          XmNtopOffset, 10,
                          NULL);           
  
  file_label = XtVaCreateManagedWidget("file", 
				       xmLabelWidgetClass,
				       output,
				       XmNalignment, XmALIGNMENT_BEGINNING,
				       XmNleftAttachment, XmATTACH_POSITION,
				       XmNleftPosition, 50,
				       XmNrightAttachment, XmATTACH_FORM,
				       XmNtopAttachment, XmATTACH_FORM,
				       XmNtopOffset, 10,
				       NULL);  
  
  /* create the sliders */

  title = XmStringCreateLocalized("File Status (%)");
  fscale = XtVaCreateManagedWidget("File", xmScaleWidgetClass,
				   output,
				   XmNmaximum, 100,
				   XmNminimum, 0,
				   XmNorientation, XmHORIZONTAL,
				   XmNshowValue, True,
				   XmNvalue, 0,
				   XmNtitleString, title,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNtopAttachment, XmATTACH_WIDGET,
				   XmNtopWidget, file_label,
				   XmNrightAttachment, XmATTACH_FORM,
				   NULL);
  XmStringFree(title);

  title = XmStringCreateLocalized("Total Status (%)");
  tscale = XtVaCreateManagedWidget("total", xmScaleWidgetClass,
				   output,
				   XmNmaximum, 100,
				   XmNminimum, 0,
				   XmNorientation, XmHORIZONTAL,
				   XmNshowValue, True,
				   XmNvalue, 0,
				   XmNtitleString, title,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNtopAttachment, XmATTACH_WIDGET,
				   XmNtopWidget, fscale,
				   XmNrightAttachment, XmATTACH_FORM,
				   NULL);
  XmStringFree(title);

  XtVaCreateManagedWidget("File Position:", xmLabelWidgetClass, output,
                          XmNalignment, XmALIGNMENT_END,
                          XmNleftAttachment, XmATTACH_FORM,
                          XmNrightAttachment, XmATTACH_POSITION,
                          XmNrightPosition, 50,
                          XmNtopAttachment, XmATTACH_WIDGET,
                          XmNtopWidget, tscale,
                          XmNtopOffset, 30,
                          NULL);
  
  file_position_label =
    XtVaCreateManagedWidget("0", xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, tscale,
                            XmNtopOffset, 30,
                            NULL);        

  XtVaCreateManagedWidget("File Remaining:", xmLabelWidgetClass, output,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, file_position_label,
			  XmNtopOffset, 5,
			  NULL);
  
  file_remaining_label =
    XtVaCreateManagedWidget("0", xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, file_position_label,
                            XmNtopOffset, 5,
                            NULL);        

  XtVaCreateManagedWidget("File ETA:", xmLabelWidgetClass, output,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, file_remaining_label,
			  XmNtopOffset, 5,
			  NULL);

  file_eta_label =
    XtVaCreateManagedWidget("0:00", xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, file_remaining_label,
                            XmNtopOffset, 5,
                            NULL);        
  
  XtVaCreateManagedWidget("Total Position:", xmLabelWidgetClass, output,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, file_eta_label,
			  XmNtopOffset, 5,
			  NULL);

  total_position_label =
    XtVaCreateManagedWidget("0", xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, file_eta_label,
                            XmNtopOffset, 5,
                            NULL);        

  XtVaCreateManagedWidget("Total remaining:", xmLabelWidgetClass, output,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, total_position_label,
			  XmNtopOffset, 5,
			  NULL);

  sprintf(templine, "%ld", total_bytes);

  total_remaining_label =
    XtVaCreateManagedWidget(templine, xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, total_position_label,
                            XmNtopOffset, 5,
                            NULL);        

  XtVaCreateManagedWidget("Total ETA:", xmLabelWidgetClass, output,
			  XmNalignment, XmALIGNMENT_END,
			  XmNleftAttachment, XmATTACH_FORM,
			  XmNrightAttachment, XmATTACH_POSITION,
			  XmNrightPosition, 50,
			  XmNtopAttachment, XmATTACH_WIDGET,
			  XmNtopWidget, total_remaining_label,
			  XmNtopOffset, 5,
			  NULL);

  total_eta_label =
    XtVaCreateManagedWidget("00:00", xmLabelWidgetClass, output,
                            XmNalignment, XmALIGNMENT_BEGINNING,
                            XmNleftAttachment, XmATTACH_POSITION,
                            XmNleftPosition, 50,
                            XmNrightAttachment, XmATTACH_FORM,
                            XmNtopAttachment, XmATTACH_WIDGET,
                            XmNtopWidget, total_remaining_label,
                            XmNtopOffset, 5,
                            NULL);        
  

  XtVaCreateManagedWidget("Speed:", xmLabelWidgetClass, output,
                          XmNalignment, XmALIGNMENT_END,
                          XmNleftAttachment, XmATTACH_FORM,
                          XmNrightAttachment, XmATTACH_POSITION,
                          XmNrightPosition, 50,
                          XmNtopAttachment, XmATTACH_WIDGET,
                          XmNtopWidget, total_eta_label,
                          XmNtopOffset, 5,
                          NULL);

  speed = XtVaCreateManagedWidget("0", xmLabelWidgetClass, output,
				  XmNalignment, XmALIGNMENT_BEGINNING,
				  XmNleftAttachment, XmATTACH_POSITION,
				  XmNleftPosition, 50,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNtopAttachment, XmATTACH_WIDGET,
				  XmNtopWidget, total_eta_label,
				  XmNtopOffset, 5,
				  NULL);

  stop = XtVaCreateManagedWidget("Stop", xmPushButtonWidgetClass, output,
                                 XmNtopAttachment, XmATTACH_WIDGET,
                                 XmNtopWidget, speed,
                                 XmNtopOffset, 30,
                                 XmNbottomAttachment, XmATTACH_FORM,
                                 XmNbottomOffset, 10,
                                 XmNleftAttachment, XmATTACH_POSITION,
                                 XmNleftPosition, 40,
                                 XmNrightAttachment, XmATTACH_POSITION,
                                 XmNrightPosition, 60,
                                 NULL);    

  XtManageChild(output);
  
  xfer_output.file_label = file_label;
  xfer_output.fscale = fscale;
  xfer_output.tscale = tscale;
  xfer_output.file_position_label = file_position_label;
  xfer_output.total_position_label = total_position_label;
  xfer_output.file_eta_label = file_eta_label;
  xfer_output.total_eta_label = total_eta_label;
  xfer_output.file_remaining_label = file_remaining_label;
  xfer_output.total_remaining_label = total_remaining_label;
  xfer_output.speed = speed;
  xfer_output.stop = stop;
  xfer_output.total_bytes = total_bytes;
  xfer_output.total_bytes_xferred = 0;
  xfer_output.dialog = output;
  xfer_output.kill_xfer = 0;
  xfer_output.resume_total_compensation = 0;
  xfer_output.resume_file_compensation = 0;

  return xfer_output;
}
/**************************************************************************/

unsigned long int get_total_bytes(XferInfo *xfer_info, int num_files,
				  int *total_files)
{
  int i;
  unsigned long int total = 0;

  for(i = 0; i < num_files; i++) {
    if(xfer_info[i].dirtree) {
      total += 
	get_total_bytes(xfer_info[i].dirtree, xfer_info[i].num_in_dirtree,
			total_files);
    }
    else {
      if(!xfer_info[i].file.link)
	total += xfer_info[i].file.size;
      (*total_files)++;
    }
  }

  return total;
}
/**************************************************************************/

void destroy_XferInfo(XferInfo **xfer_info, int number)
{
  XferInfo *temp;
  int i;

  if(*xfer_info) {
    temp = *xfer_info;

    for(i = 0; i < number; i++) {
      if(temp[i].file.filename) {
	free(temp[i].file.filename);
	temp[i].file.filename = NULL;
      }
      
      if(temp[i].dirtree)
	destroy_XferInfo(&(temp[i].dirtree), temp[i].num_in_dirtree);
      
    }
    free(temp);
    *xfer_info = NULL;
  }
}
/**************************************************************************/

void print_XferInfo(XferInfo *this, int number)
{
  int i;

  if(this) {
    for(i = 0; i < number; i++) {
      printf("File: %s\n", this[i].file.filename);
      print_XferInfo(this[i].dirtree, this[i].num_in_dirtree);
    }
  }
}
/**************************************************************************/

void kill_xfer(Widget w, XtPointer client_data, XtPointer call_data)
{
  Output *xfer_output = (Output *) client_data;

  if(w)
    /* one time button hit please */
    XtSetSensitive(w, False);
 
  xfer_output->kill_xfer = 1; 
}
/**************************************************************************/

void reset_stats(Main_CB *main_cb, Output *xfer_output, char *filename, 
		 unsigned long int size)
{
  XmString string;
  char templine[LINE_MAX];
  
  string = XmStringCreateLocalized(filename);
  XtVaSetValues(xfer_output->file_label, XmNlabelString, string, NULL);
  XmStringFree(string);

  sprintf(templine, "%ld", size);
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->file_remaining_label, XmNlabelString, string,
		NULL);
  XmStringFree(string);

  string = XmStringCreateLocalized("0");
  XtVaSetValues(xfer_output->file_position_label, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
  
  string = XmStringCreateLocalized("0:00");
  XtVaSetValues(xfer_output->file_eta_label, XmNlabelString, string, NULL);
  XmStringFree(string);
  
  XtVaSetValues(xfer_output->fscale, XmNvalue, 0, NULL);

  refresh_screen(main_cb);
}
/**************************************************************************/

void update_stats(Main_CB *main_cb, Output *xfer_output,
		  unsigned long int *file_bytes_xferred,
		  unsigned long int *size,
		  unsigned long int *total_bytes_xferred,
		  time_t *start_time)
{
  int percentage;
  XmString string;
  char templine[LINE_MAX];
  time_t difference, eta, sec_remaining, last_upd;
  unsigned int total_k_read, k_remaining, min_remaining;
  float k_per_sec, eta_calc;
  
  /* last update time */
  last_upd = time(NULL) - xfer_output->last_update_time;

  /* only update if more than 1 seconds passed */
  if(last_upd <= 0)
    /* less than second passed */
    return;

  /* Updata file percentage scale */
  if(*size) {
    percentage = (int) (((float)(*file_bytes_xferred)/(*size)) * 100);
    XtVaSetValues(xfer_output->fscale, XmNvalue, percentage, NULL);
  }

  /* Update total percentage scale */
  percentage = (int)
    (((float)(*total_bytes_xferred)/(xfer_output->total_bytes)) * 100);
  XtVaSetValues(xfer_output->tscale, XmNvalue, percentage, NULL);
  
  /* Update file bytes transfered */
  /* lets only update if byte position has changed > 1024 */
  /*
  XtVaGetValues(xfer_output->file_position_label, XmNlabelString, &string,
		NULL);
  XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &tempstring);
  templong = atol(tempstring);
  free(tempstring);
  */

  /*  if((*file_bytes_xferred - templong) >> 10) { */

  sprintf(templine, "%ld", *file_bytes_xferred);
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->file_position_label, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
    
  /* Update total bytes transferred */
  sprintf(templine, "%ld", *total_bytes_xferred);
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->total_position_label, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
  
  /* Update file bytes remaining */
  sprintf(templine, "%ld", (*size) - (*file_bytes_xferred));
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->file_remaining_label, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
  
  /* Update total bytes remaining */
  sprintf(templine, "%ld", (xfer_output->total_bytes) - 
	  (*total_bytes_xferred));
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->total_remaining_label, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
  
  /* calculate speed */

  xfer_output->last_update_time = time(NULL);
  
  /* How many seconds passed */
  difference = xfer_output->last_update_time - (*start_time);    
  
  total_k_read = ((*file_bytes_xferred) - 
		  (xfer_output->resume_file_compensation)) >> 10;
  if(difference)
    k_per_sec = ((float)total_k_read)/difference;
  else
    k_per_sec = (float)total_k_read;
  
  sprintf(templine, "%.2f k/second", k_per_sec); 
  string = XmStringCreateLocalized(templine);
  XtVaSetValues(xfer_output->speed, XmNlabelString, string, 
		NULL);
  XmStringFree(string);
  

  /* calculate file eta */
  
  if(k_per_sec) {
    eta_calc = 1/k_per_sec;
    k_remaining = (((*size) - (*file_bytes_xferred)) >> 10);
    
    eta = (time_t) ((eta_calc * k_remaining) +
		    .5);
    
    sec_remaining = (time_t ) eta % 60;
    min_remaining = (unsigned int) (eta / 60);
    
    if(min_remaining/60) {
      /* hours are involved */
      sprintf(templine, "%d hours %d:%s%ld", min_remaining/60, 
	      min_remaining % 60, (sec_remaining < 10) ? "0" : "", 
	      sec_remaining);
    }
    else
      sprintf(templine, "%d:%s%ld", min_remaining, 
	      (sec_remaining < 10) ? "0" : "", sec_remaining);
    
    string = XmStringCreateLocalized(templine);
    XtVaSetValues(xfer_output->file_eta_label, XmNlabelString, string, 
		  NULL);
    XmStringFree(string);    
    
    /* calculate total eta */
    
    k_remaining = ((xfer_output->total_bytes) - (*total_bytes_xferred)) >> 10;
    
    eta = (time_t) ((eta_calc * k_remaining) +
		    .5);
    
    sec_remaining = (time_t) eta % 60;
    min_remaining = (unsigned int) (eta / 60);
    
    if(min_remaining/60) {
      /* hours are involved */
      sprintf(templine, "%d hours %d:%s%ld", min_remaining/60, 
	      min_remaining % 60, (sec_remaining < 10) ? "0" : "", 
	      sec_remaining);
    }
    else
      sprintf(templine, "%d:%s%ld", min_remaining, 
	      (sec_remaining < 10) ? "0" : "", sec_remaining);
    
    string = XmStringCreateLocalized(templine);
    XtVaSetValues(xfer_output->total_eta_label, XmNlabelString, string, 
		  NULL);
    XmStringFree(string);    
  }
  
  refresh_screen(main_cb);
}  
/**************************************************************************/

int rtm_reconnect(Main_CB *main_cb, Output *xfer_output)
{
  char *resp;
  char templine[80];
  int i;

  reconnect(main_cb->main_window, main_cb, NULL);
  /* successful reconnect? man I hate the event driven model */
  XtVaGetValues(main_cb->main_window, XmNuserData, &resp, NULL);
  if(resp)
    return -3;

  do {
    sprintf(templine, "Sleeping for %d seconds...", RTMReconnectPause);
    add_string_to_status(main_cb, templine, NULL);
    for(i = 0; i < RTMReconnectPause; i++) {
      refresh_screen(main_cb);
      sleep(1);
      CheckForInterrupt(main_cb, xfer_output->dialog);

      if(xfer_output->kill_xfer) {
	/* User wants to stop */
	add_string_to_status(main_cb, "Transfer killed!", NULL);
	return -1; 
      }
    }      
    reconnect(main_cb->main_window, main_cb, NULL);
    /* successful reconnect? man I hate the event driven model */
    XtVaGetValues(main_cb->main_window, XmNuserData, &resp, NULL);
  } while(!resp);
  return -3;
}    
