/* local_remote_ops.c - local & Remote operations such as mkdir, 
 *                      delete, rename
 *
 * Dialogs and operations are similar for local & remote
*/

/***************************************************************************/

#include "xmftp.h"
#include "operations.h"
#include "remote_refresh.h"
#include "../program/ftp.h"
#include "../program/misc.h"
#include "transfer.h"
#include <malloc.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <stdio.h>
#include "local_remote_ops.h"
#include "view.h"

/***************************************************************************/

/* Used to pass information between the various dialogs being used
 * for deletion
 */
typedef struct {
  FileList *files;           /* List of all files in current section */
  int num_files;             /* Number of files in array */
  int *position_list;        /* Array of positions selected from array */
  int position_count;        /* Number of position selected */
  Widget dialog, file_label; /* Dialog info to show status during deletion */
  Main_CB *main_cb;
  int cancel_deletion;       /* Variable becomes 1 when user wants to stop */
  int remote;                /* used to determine if remote or local section */
} DelFType;

/***************************************************************************/

/* Processing of site command */
void process_site_command(Widget w, XtPointer client_data, 
			  XtPointer call_data);

/* Modifies the global variable deletion_files to reflect that the user
 * wants to cancel the deletion
 */
void stop_deletion(Widget w, XtPointer client_data, XtPointer call_data);

/* Deletes (or tries to delete) everything in directory 'filename'
 * from the proper section.
 * returns 1 on success, 0 on failure 
 */
int delete_subdir(char *filename, DelFType *dfs);

/* Actual performing of deletion (ok button callback) */
void perform_deletion(Widget w, XtPointer client_data, XtPointer call_data);

/* Perform actual remote or local directory creation */
void process_remote_mkdir(Widget w, XtPointer client_data, 
			  XtPointer call_data);
void process_local_mkdir(Widget w, XtPointer client_data, 
			  XtPointer call_data);

/* Dialog prompt to setup rename */
void perform_rename(Main_CB *main_cb, int remote);

/* Perform actual renaming */
void process_remote_rename(Widget w, XtPointer client_data, 
			   XtPointer call_data);
void process_local_rename(Widget w, XtPointer client_data, 
			   XtPointer call_data);

/* User was OK for deletion, brings up another preliminary dialog */
void ok_deletion(Widget w, XtPointer client_data, XtPointer call_data);

/* Cleanup when deletion is cancelled */
void cancel_deletion(Widget w, XtPointer client_data, XtPointer call_data);

/* Used to update in real-time the info dialog showing information on what
 * is being deleted.
 */
void update_delete_dialog(char *filename, int dir, DelFType *dfs);

/* Common deletion function for remote or local */
void delete(Main_CB *main_cb, int remote);

/* These functions do the specified operations by determining which
 * section the operation is to be performed on (via the global structure)
 */
int section_change_dir(char *filename, DelFType *dfs);
int section_remove_dir(char *filename, DelFType *dfs);
int section_delete_file(char *filename, DelFType *dfs);

/***************************************************************************/

void delete_remote(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  delete(main_cb, 1);
}
/***************************************************************************/

void delete_local(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;

  delete(main_cb, 0);
}
/***************************************************************************/

void delete(Main_CB *main_cb, int remote)
{
  Widget w = main_cb->main_window;
  char templine[80];
  DirSection section = remote ? main_cb->remote_section : 
    main_cb->local_section;
  int *position_list, position_count;
  DelFType *deletion_files;

  /* Make sure user does not select . or .. for deletion */
  if((XmListPosSelected(section.dir_list, 1) == True) ||
     (XmListPosSelected(section.dir_list, 2) == True)) {
    error_dialog(w, "You cannot select ./ or ../ for deletion.");
    return;
  }

  if(XmListGetSelectedPos(section.dir_list, &position_list,
			  &position_count) == False) {
    info_dialog(w, remote ? "You must select some remote files to delete." :
		"You must select some local files to delete.");
    return;
  }
  
  /* setup the global variable with information */
  deletion_files = (DelFType *) malloc(sizeof(DelFType));

  deletion_files->files = section.files;
  deletion_files->num_files = section.num_files;
  deletion_files->position_list = position_list;
  deletion_files->position_count = position_count;
  deletion_files->main_cb = main_cb;
  deletion_files->cancel_deletion = 0;
  deletion_files->remote = remote;

  sprintf(templine, "Are you sure you want to delete the %d selected "
	  "%s files?",
	  position_count, remote ? "remote" : "local");

  deletion_files->dialog = warning_y_n_dialog(main_cb->main_window, templine);

  XtAddCallback(XmMessageBoxGetChild(deletion_files->dialog, 
				     XmDIALOG_OK_BUTTON), 
		XmNactivateCallback, ok_deletion, deletion_files);
  XtAddCallback(XmMessageBoxGetChild(deletion_files->dialog, 
				     XmDIALOG_CANCEL_BUTTON), 
		XmNactivateCallback, cancel_deletion, deletion_files);
  
  XtManageChild(deletion_files->dialog);
}
/***************************************************************************/
  
void cancel_deletion(Widget w, XtPointer client_data, XtPointer call_data)
{
  DelFType *dfs = (DelFType *) client_data;

  XtDestroyWidget(dfs->dialog);
  free(dfs->position_list);
  free(dfs);
}
/***************************************************************************/

void ok_deletion(Widget w, XtPointer client_data, XtPointer call_data)
{
  Arg args[2];
  int n = 0;
  DelFType *deletion_files = (DelFType *) client_data;
  Main_CB *main_cb;
  Widget file_list, rowcol, dir_list, buttons, ok, cancel;
  int i, index;
  char *filename;
  XmString file;
  
  XtDestroyWidget(deletion_files->dialog);
  main_cb = deletion_files->main_cb;
  deletion_files->dialog = XmCreateFormDialog(main_cb->main_window, 
					      "Delete", NULL, 0);
  XtVaSetValues(deletion_files->dialog,
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  
  rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass, 
			    deletion_files->dialog,
			    XmNorientation, XmVERTICAL,
			    XmNentryAlignment, XmALIGNMENT_CENTER,
			    NULL);
  XtVaCreateManagedWidget("The following files will be deleted:", 
			  xmLabelWidgetClass, rowcol, NULL);

  /* setup some nice lists of directories and files to be deleted */
  
  XtSetArg(args[n], XmNvisibleItemCount, 5); n++;
  file_list = XmCreateScrolledList(rowcol, "filelist", args, n);

  XtVaCreateManagedWidget("The following directories AND all of their "
			  "subdirectories will be deleted:", 
			  xmLabelWidgetClass, rowcol, NULL);

  dir_list = XmCreateScrolledList(rowcol, "dirlist", args, n);
  
  /* Add each file selected to dir or filelist depending on whether it is a
   * dir or a file
   */
  for(i = 0; i < deletion_files->position_count; i++) {
    index = deletion_files->position_list[i] - 1;
    filename = deletion_files->files[index].filename;
    file = XmStringCreateLocalized(filename);
    if(deletion_files->files[index].directory)
      XmListAddItem(dir_list, file, 0);
    else
      XmListAddItem(file_list, file, 0);

    XmStringFree(file);
  }
		    
  XtManageChild(file_list);
  XtManageChild(dir_list);
  
  buttons = XtVaCreateWidget("form", xmFormWidgetClass, rowcol,
			     NULL);
  ok = XtVaCreateManagedWidget("Delete", xmPushButtonWidgetClass, buttons,
			       XmNleftAttachment, XmATTACH_POSITION,
			       XmNleftPosition, 20,
			       XmNrightAttachment, XmATTACH_POSITION,
			       XmNrightPosition, 40,
			       XmNtopAttachment, XmATTACH_FORM,
			       NULL);
  cancel = XtVaCreateManagedWidget("Cancel", xmPushButtonWidgetClass, buttons,
				   XmNleftAttachment, XmATTACH_POSITION,
				   XmNleftPosition, 60,
				   XmNrightAttachment, XmATTACH_POSITION,
				   XmNrightPosition, 80,
				   XmNtopAttachment, XmATTACH_FORM,
				   NULL);
 
  XtAddCallback(ok, XmNactivateCallback, perform_deletion, deletion_files);
  XtAddCallback(cancel, XmNactivateCallback, cancel_deletion, 
		deletion_files);

  XtManageChild(buttons);
  XtManageChild(rowcol);
  XtManageChild(deletion_files->dialog);
}
/***************************************************************************/
  
void stop_deletion(Widget w, XtPointer client_data, XtPointer call_data)
{
  DelFType *dfs = (DelFType *) client_data;

  /* The variable should be checked occasionally */
  dfs->cancel_deletion = 1;
}
/***************************************************************************/

void perform_deletion(Widget w, XtPointer client_data, XtPointer call_data)
{
  DelFType *dfs = (DelFType *) client_data;
  Widget main_w;
  Widget info;
  int i, index, dir_error = 0;
  char *filename;
  XmString text;
  char templine[LINE_MAX];
  Main_CB *main_cb;

  main_cb = dfs->main_cb;
  main_w = main_cb->main_window;
  /* Kill the confirmation dialog */
  XtDestroyWidget(dfs->dialog);
  
  /* Setup a dialog to show what files are being deleted in real-time */
  info = XmCreateInformationDialog(main_w, 
				   "Deleting", NULL, 0);
  dfs->dialog = info;
  
  text = XmStringCreateLocalized("Currently removing...");

  XtVaSetValues(info, 
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		XmNresizePolicy, XmRESIZE_NONE,
		XmNmessageString, text,
		XmNwidth, 400,
		NULL);
  XmStringFree(text);

  XtAddCallback(info, XmNcancelCallback, stop_deletion, dfs);
  XtUnmanageChild(XmMessageBoxGetChild(info, XmDIALOG_OK_BUTTON));
  XtUnmanageChild(XmMessageBoxGetChild(info, XmDIALOG_HELP_BUTTON));
  
  /* Provide a dummy filename for good sizing of dialog */
  dfs->file_label = 
    XtVaCreateManagedWidget("...", xmLabelWidgetClass, info, NULL);
  
  XtManageChild(info);
  refresh_screen(dfs->main_cb);

  /* Actual gist of deletion */
  for(i = 0; i < dfs->position_count; i++) {
    /* Check for cancel by user through each iteration */
    CheckForInterrupt(main_cb, info);

    if(dfs->cancel_deletion) {
      /* User cancelled */
      info_dialog(main_w, "Deletion was cancelled.");

      /* dir_error is used to know whether our sync of directory location and
       * directory location label may be messed.
       */
      dir_error = 1;
      break;
    }
    index = dfs->position_list[i] - 1;
    filename = dfs->files[index].filename;

    if(dfs->files[index].directory) {
      /* Delete everything in this directory */
      if(delete_subdir(filename, dfs)) {
	update_delete_dialog(filename, 1, dfs);

	/* Now remove the directory */
	if(!section_remove_dir(filename, dfs)) {
	  sprintf(templine, "Could not remove directory %s.", filename);
	  error_dialog(main_w, templine);
	  break;
	}
      }
      else {
	dir_error = 1;
	break;
      }
    }
    else {
      update_delete_dialog(filename, 0, dfs);
      if(!section_delete_file(filename, dfs)) {
	sprintf(templine, "Could not remove file %s.", filename);
	error_dialog(main_w, templine);
	/* should not be out of sync but who knows */
	dir_error = 0;
	break;
      }
    }
  }

  if(!dfs->cancel_deletion)
    /* user did not stop deletiong (dialog still up) kill it */
    XtDestroyWidget(info);
 
  if(dir_error) {
    /* Reset the CWD label */
    if(dfs->remote)
      set_remote_dir_label(main_cb);
    else
      set_local_dir_label(main_cb);
  }

  /* always refresh dirlist */
  if(dfs->remote)
    refresh_remote_dirlist(main_cb);
  else
    refresh_local_dirlist(main_cb, ".");

  free(dfs->position_list);
  free(dfs);
}
/***************************************************************************/

int delete_subdir(char *filename, DelFType *dfs)
{
  FileList *files;
  DirState state = { LONG, NAME, ASCENDING };
  int num_files, i;
  char templine[LINE_MAX];

  if(!section_change_dir(filename, dfs)) {
    sprintf(templine, "Could not change into %s directory %s.", 
	    dfs->remote ? "remote" : "local",
	    filename);
    error_dialog(dfs->main_cb->main_window, templine);
    return 0;
  }

  refresh_screen(dfs->main_cb);

  /* Get files in current directory */
  files = dfs->remote ? 
    generate_remote_filelist(&num_files, state, 
			     dfs->main_cb->current_state.
			     connection_state.site_info.system_type) :
    generate_local_filelist(".", &num_files, state); 
  
  if(!files) {
    /* no files in dir? */
    section_change_dir("..", dfs); /* might be prob */
    return 0;
  }

  /* Process filelist (recurse if necessary) similar to above */
  for(i = 2; i < num_files; i++) {
    CheckForInterrupt(dfs->main_cb, dfs->dialog);
    if(dfs->cancel_deletion) {
      destroy_filelist(&files, num_files);
      return 0;
    }
    if(files[i].directory) {
      if(!delete_subdir(files[i].filename, dfs)) {
	destroy_filelist(&files, num_files);
	return 0;
      }
      update_delete_dialog(files[i].filename, 1, dfs);
      if(!section_remove_dir(files[i].filename, dfs)) {
	sprintf(templine, "Could not remove directory %s.", files[i].filename);
	error_dialog(dfs->main_cb->main_window, templine);
	destroy_filelist(&files, num_files);
	return 0;
      }
    }
    else {
      update_delete_dialog(files[i].filename, 0, dfs);
      if(!section_delete_file(files[i].filename, dfs)) {
	sprintf(templine, "Could not remove file %s.", files[i].filename);
	error_dialog(dfs->main_cb->main_window, templine);
	destroy_filelist(&files, num_files);
	return 0;
      }
    }
  }
  destroy_filelist(&files, num_files);
  section_change_dir("..", dfs);

  return 1;
}
/***************************************************************************/
  
void mkdir_remote(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XmString prompt;
  Widget dialog;

  if(!main_cb->current_state.connection_state.connected) {
    info_dialog(w, 
     "You must first be connected to a site to perform this operation.");
    return;
  }

  prompt = XmStringCreateLocalized("Enter remote directory to create:");
  dialog = XmCreatePromptDialog(GetTopShell(w), "Mkdir", NULL, 0);  
  XtVaSetValues(dialog, XmNselectionLabelString, prompt, NULL);
  
  XmStringFree(prompt);
  XtAddCallback(dialog, XmNokCallback, process_remote_mkdir, main_cb);
  
  XtManageChild(dialog);

}
/***************************************************************************/

void process_remote_mkdir(Widget w, XtPointer client_data, 
			  XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname;
  XmString dir;

  XtVaGetValues(w, XmNtextString, &dir, NULL);
  XmStringGetLtoR(dir, XmFONTLIST_DEFAULT_TAG, &dirname);

  if(!make_remote_dir(dirname))
    error_dialog(w, "Could not create directory!");
  else
    refresh_remote_dirlist(main_cb);
  
  free(dirname);
}
/***************************************************************************/

void mkdir_local(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XmString prompt;
  Widget dialog;

  prompt = XmStringCreateLocalized("Enter local directory to create:");
  dialog = XmCreatePromptDialog(GetTopShell(w), "Mkdir", NULL, 0);  
  XtVaSetValues(dialog, XmNselectionLabelString, prompt, NULL);
  
  XmStringFree(prompt);
  XtAddCallback(dialog, XmNokCallback, process_local_mkdir, main_cb);
  
  XtManageChild(dialog);

}
/***************************************************************************/

void process_local_mkdir(Widget w, XtPointer client_data, 
			  XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname;
  XmString dir;

  XtVaGetValues(w, XmNtextString, &dir, NULL);
  XmStringGetLtoR(dir, XmFONTLIST_DEFAULT_TAG, &dirname);

  if(!make_dir(dirname))
    error_dialog(w, "Could not create directory!");
  else
    refresh_local_dirlist(main_cb, ".");
  
  free(dirname);
}
/***************************************************************************/

void update_delete_dialog(char *filename, int dir, DelFType *dfs)
{
  XmString text;
  int length;
  char *temp;

  /* Give a pretty output instead of dialog changing sizes on us */
  length = strlen(filename);
  
  if(length > 40)
    temp = &filename[length - 40];
  else
    temp = filename;

  text = XmStringCreateLocalized(temp);
  XtVaSetValues(dfs->file_label, XmNlabelString, text, NULL);
  refresh_screen(dfs->main_cb);
  XmStringFree(text);
}
/***************************************************************************/

int section_change_dir(char *filename, DelFType *dfs)
{
  if(dfs->remote)
    return(change_remote_dir(filename));
  else
    return(change_dir(filename)); 
}
/***************************************************************************/

int section_remove_dir(char *filename, DelFType *dfs)
{
  if(dfs->remote)
    return(remove_remote_dir(filename));
  else 
    return(remove_local_dir(filename));
}
/***************************************************************************/
    
int section_delete_file(char *filename, DelFType *dfs)
{
  if(dfs->remote)
    return(delete_remote_file(filename));
  else 
    return(delete_local_file(filename));
}
/***************************************************************************/

void rename_remote(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  
  perform_rename(main_cb, 1);
}
/***************************************************************************/

void rename_local(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  
  perform_rename(main_cb, 0);
}
/***************************************************************************/

void perform_rename(Main_CB *main_cb, int remote)
{
  DirSection section = remote ? main_cb->remote_section : 
    main_cb->local_section;
  int *position_list, position_count;
  char *text;
  XmString prompt;
  Widget dialog;
  char templine[LINE_MAX];
  int index;

  /* Make sure user does not select . or .. for renaming */
  if((XmListPosSelected(section.dir_list, 1) == True) ||
     (XmListPosSelected(section.dir_list, 2) == True)) {
    error_dialog(main_cb->main_window, 
		 "You cannot select ./ or ../ for renaming.");
    return;
  }

  if(XmListGetSelectedPos(section.dir_list, &position_list,
			  &position_count) == False) {

    info_dialog(main_cb->main_window, remote ? 
		"No remote file was selected to rename." :
		"No local file was selected to rename.");
    return;
  }

  if(position_count != 1) {
    info_dialog(main_cb->main_window, remote ? 
		"Please select only one remote file to rename." :
		"Please select only one local file to rename.");
    free(position_list);
    return;
  }
  
  index = *position_list - 1;
  text = section.files[index].filename;

  sprintf(templine, "Rename %s to:", text);

  free(position_list);

  prompt = XmStringCreateLocalized(templine);
  dialog = XmCreatePromptDialog(main_cb->main_window, "Rename", NULL, 0);  
  XtVaSetValues(dialog, XmNselectionLabelString, prompt, 
		XmNuserData, text,
		NULL);

  XtAddCallback(dialog, XmNokCallback, remote ? process_remote_rename :
		process_local_rename, main_cb);

  XtManageChild(dialog);

}
/***************************************************************************/

void process_remote_rename(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *file, *newname;
  XmString new;
  
  XtVaGetValues(w, XmNtextString, &new, 
		XmNuserData, &file,
		NULL);
  XmStringGetLtoR(new, XmFONTLIST_DEFAULT_TAG, &newname);
  XtVaGetValues(w, XmNuserData, &file, NULL);

  if(!rename_remote_file(file, newname))
    error_dialog(main_cb->main_window, "Could not rename file.");
  else
    refresh_remote_dirlist(main_cb);
  free(newname);
}
/***************************************************************************/

void process_local_rename(Widget w, XtPointer client_data, 
			   XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *file, *newname;
  XmString new;
  
  XtVaGetValues(w, XmNtextString, &new, 
		XmNuserData, &file,
		NULL);
  XmStringGetLtoR(new, XmFONTLIST_DEFAULT_TAG, &newname);
  XtVaGetValues(w, XmNuserData, &file, NULL);

  if(!rename_local_file(file, newname))
    error_dialog(main_cb->main_window, "Could not rename file.");
  else
    refresh_local_dirlist(main_cb, ".");
  free(newname);
}
/***************************************************************************/

void site_remote(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  Widget dialog;
  XmString prompt;

  if(!(main_cb->current_state.connection_state.connected)) {
    /* not connected */
    error_dialog(w, "You must first connect to a site.");
    return;
  }
  prompt = XmStringCreateLocalized("Enter SITE command (excluding 'SITE'):");
  
  dialog = XmCreatePromptDialog(GetTopShell(w), "SITE", NULL, 0);
  XtVaSetValues(dialog, XmNselectionLabelString, prompt, 
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  XmStringFree(prompt);

  XtAddCallback(dialog, XmNokCallback, process_site_command, main_cb);
  XtManageChild(dialog);
}

void process_site_command(Widget w, XtPointer client_data, 
			  XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XmString cmd;
  char *command;
  FILE *fp;

  XtVaGetValues(w, XmNtextString, &cmd, NULL);
  XmStringGetLtoR(cmd, XmFONTLIST_DEFAULT_TAG, &command);
  
  fp = tmpfile();

  execute_site_cmd(command, fp);
  viewFP(main_cb->main_window, main_cb, fp);
  
  fclose(fp);
  free(command);
}
