/* remote_refresh.c - remote section specific operations */

/*************************************************************************/

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/SelectioB.h>
#include <malloc.h>
#include <stdio.h>
#include "remote_refresh.h"
#include "../program/misc.h"
#include "operations.h"
#include "../program/ftp.h"
#include "fill_filelist.h"
#include "view.h"

/*************************************************************************/

/* Fille the remote section dirlist nicely formatting */
void fill_remote_slist(Main_CB *main_cb);

/* ok button callback to change dir explicit */
void process_remote_dir_selection(Widget w, XtPointer client_data, 
				  XtPointer call_data);

/*************************************************************************/

void chdir_remotely(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  Widget dialog;
  XmString prompt;

  if(!(main_cb->current_state.connection_state.connected)) {
    /* not connected */
    error_dialog(w, "You must first connect to a site.");
    return;
  }

  prompt = XmStringCreateLocalized("Enter directory:");
  
  dialog = XmCreatePromptDialog(GetTopShell(w), "CD", NULL, 0);
  XtVaSetValues(dialog, XmNselectionLabelString, prompt, 
		XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
		NULL);
  XmStringFree(prompt);

  XtAddCallback(dialog, XmNokCallback, process_remote_dir_selection, main_cb);
  XtManageChild(dialog);
}
/*************************************************************************/

void process_remote_dir_selection(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  XmString dir;
  char *dirname, *total_dir_name;
  SiteInfo *site_info = &(main_cb->current_state.connection_state.site_info);

  XtVaGetValues(w, XmNtextString, &dir, NULL);
  XmStringGetLtoR(dir, XmFONTLIST_DEFAULT_TAG, &dirname);
  
  if(!change_remote_dir(dirname))
    error_dialog(w, "Could not change to specified directory!");
  else {
    /* Store current dir info in application state */
    get_remote_directory(site_info);
    total_dir_name = site_info->directory;
    XtVaSetValues(main_cb->remote_section.cwd_field, XmNvalue, total_dir_name, 
		  XmNcursorPosition, strlen(total_dir_name),
		  NULL);
    
    refresh_remote_dirlist(main_cb);
  }
  free(dirname);
}
/*************************************************************************/
  
void traverse_remotely(Widget w, XtPointer client_data, XtPointer call_data)
{
  Main_CB *main_cb = (Main_CB *) client_data;
  char *dirname, *total_dir_name;
  int *position_list, position_count;
  FileList file;
  SiteInfo *site_info = &(main_cb->current_state.connection_state.site_info);
  
  BusyCursor(main_cb, 1);

  if(XmListGetSelectedPos(w, &position_list, &position_count) == False)
    return;
  file = main_cb->remote_section.files[(*position_list-1)];
  free(position_list);

  dirname = file.filename;

  if(!file.link) {
    /* file is NOT a link */
    if(!file.directory) {
      /* looks like a regular file, view it */
      BusyCursor(main_cb, 0);
      view_remote(w, main_cb, NULL);
      return;
    }

    /* must be a dir */
    if(!change_remote_dir(dirname)) {
      BusyCursor(main_cb, 0);
      error_dialog(w, "Could not change to specified directory!");
      return;
    }
  }
  else {
    /* it's a link, what to do? */
    if(get_remote_size(file.filename, &(file.size))) {
      /* It looks like a regular file, let's view it */
      BusyCursor(main_cb, 0);
      view_remote(w, main_cb, NULL);
      return;
    }
    /* It must be a directory, but still may not be able to cd (permission) */
    if(!change_remote_dir(dirname)) {
      BusyCursor(main_cb, 0);
      error_dialog(w, "Could not change to specified directory!");
      return;
    }
  }

  /* current dir has been updated, modify connection state */
  get_remote_directory(site_info);
  
  total_dir_name = site_info->directory;
  XtVaSetValues(main_cb->remote_section.cwd_field, XmNvalue, total_dir_name, 
		XmNcursorPosition, strlen(total_dir_name),
		NULL);
  
  BusyCursor(main_cb, 0);
  
  refresh_remote_dirlist(main_cb);
  
}
/*************************************************************************/
  
void refresh_remote_dirlist(Main_CB *main_cb)
{
  unsigned int num_files;
  FileList *files;

  BusyCursor(main_cb, 1);

  if(main_cb->remote_section.files)
    destroy_list(&(main_cb->remote_section));


  files = generate_remote_filelist(&num_files, 
				   main_cb->current_state.rdir_state,
				   main_cb->current_state.connection_state.
				   site_info.system_type);

  if(!files) {
    puts("remote files is null!");
    BusyCursor(main_cb, 0);
    return;
    exit(1);
  }
  
  main_cb->remote_section.files = files;
  main_cb->remote_section.num_files = num_files;

  fill_slist(files, main_cb->remote_section.dir_list, num_files,
	     (main_cb->current_state.options.flags) & OptionLargeFont,
	     (main_cb->current_state.options.flags) & OptionDontCropFNames);

  BusyCursor(main_cb, 0);

}
/*************************************************************************/
    
void sort_remote_dirlist(Main_CB *main_cb)
{
  if(!main_cb->remote_section.files)
    return;

  BusyCursor(main_cb, 1);
  switch(main_cb->current_state.rdir_state.sorting) {
  case NAME:
    switch(main_cb->current_state.rdir_state.order) {
    case ASCENDING:
      sort_filelist(main_cb->remote_section.files, 
		    main_cb->remote_section.num_files, 0);
      break;
    case DESCENDING:
      sort_filelist(main_cb->remote_section.files,
		    main_cb->remote_section.num_files, 1);
      break;
    default:
      break;
    }
    break;
  case DATE:
    switch(main_cb->current_state.rdir_state.order) {
    case ASCENDING:
      sort_filelist(main_cb->remote_section.files, 
		    main_cb->remote_section.num_files, 2);
      break;
    case DESCENDING:
      sort_filelist(main_cb->remote_section.files, 
		    main_cb->remote_section.num_files, 3);
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }

  XmListDeleteAllItems(main_cb->remote_section.dir_list);
  
  fill_slist(main_cb->remote_section.files, main_cb->remote_section.dir_list, 
	     main_cb->remote_section.num_files,
	     (main_cb->current_state.options.flags) & OptionLargeFont,
	     (main_cb->current_state.options.flags) & OptionDontCropFNames);
  
  BusyCursor(main_cb, 0);

}
/*************************************************************************/
