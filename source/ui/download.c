/* download.c - functions to perform retrieval of remote files */

/*************************************************************************/

#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "download.h"
#include "../program/ftp.h"
#include "../program/misc.h"
#include "../ftplib/ftplib.h"
#include "remote_refresh.h"
#include "operations.h"
#include "transfer.h"
#include "connection.h"
#include "../program/systems.h"

/*************************************************************************/

/* We'll use an alarm that will set this flag when transfer has timed out
 * Will only be used when RTMode is enabled 
 */
char sigbreak = 0;

/* Main routine that starts actual download */
int do_retrieve(Main_CB *main_cb, Output *xfer_output, XferInfo *xfer_info,
		int num_files);

/* If the file being transferred already exists and the host does not support
 * resume, this callback will overwrite the local file.
 * XmNuserData of 'w' must be a malloc'd string of the filename which 
 * will be freed
 */
void delete_download(Widget w, XtPointer client_data, XtPointer call_data);

/* If the file being transferred already exists and the host does not support
 * resume, this callback will skip the local file.
 * Currently, this also cancels the WHOLE download!
 * XmNuserData of 'w' must be a malloc'd string of the filename which
 * will be freed
*/
void cancel_del_download(Widget w, XtPointer client_data, XtPointer call_data);

/* Setup the signal catching system
 * this sets sigbreak = 0, sets an alarm() at 'timeout', and
 * assigns the function sig() to catch the SIGALRM
 */
void setup_alarm(unsigned int timeout);

/* simple sets sigbreak = 1 to be checked somewhere else */
void sig(int signo);

/*************************************************************************/

int perform_download(Main_CB *main_cb, char *basename, XferInfo *xfer_info,
		     int num_files)
{
  Output xfer_output;
  int retval;

  /* Create the dialog common to transfers (uploading and downloading) */
  xfer_output = transfer_common_dialog(main_cb, xfer_info, num_files);

  /* When user hits Stop button in transfer dialog, a variable in
   * xfer_output is set by kill_xfer(). During the download, this variable
   * is checked. this is wierd in some window managers
   */
  XtAddCallback(xfer_output.stop, XmNactivateCallback, kill_xfer, 
		&xfer_output);
  /* afterstep causing probs? map to disarm as well */
  XtAddCallback(xfer_output.stop, XmNdisarmCallback, kill_xfer, 
		&xfer_output);
  

  /* Start transfer */
  retval = do_retrieve(main_cb, &xfer_output, xfer_info, num_files);
  if(retval == -1) {
    /* The user killed the transfer. Unfortunately the connection to the
     * FTP server seems corrupt so we must reconnect
     */
    add_string_to_status(main_cb, "Connection is corrupt", NULL);
    kill_connection();
    reconnect(main_cb->toplevel, main_cb, main_cb);

    /* No zombie process please */
    waitpid(0, NULL, WNOHANG);
  }
  
  waitpid(0, NULL, WNOHANG);
  XtDestroyWidget(xfer_output.dialog);

  /* if basename is NULL, downloading was called through some other
   * operation indirectly (most likely the view file operation). If
   * this is so, we don't have to refresh the local dir list because
   * nothing was meant to be downloaded as perceived by the user.
   */
  if(basename)
    refresh_local_dirlist(main_cb, ".");

  if(retval == -3)
    /* Transfer cancelled due to timeout (RTMode)
     * notify caller that we must re-download
     */
    return 0;
  else
    return 1;
}
/*************************************************************************/

int do_retrieve(Main_CB *main_cb, Output *xfer_output, XferInfo *xfer_info,
		int num_files)
{
  Widget warning;
  unsigned long int file_bytes_xferred = 0;
  OptionFlagType options = main_cb->current_state.options.flags;
  int i, fd, bread, retval, dot, sel_retval;
  time_t start_time;
  char templine[LINE_MAX], *ch_ptr, downloading = 0;
  fd_set rfds;
  struct timeval tv;

  /* for recursion */
  if(xfer_output->kill_xfer)
    return -1;

  /* Loop through the current filelist */
  for(i = 0; i < num_files; i++) {
    if(xfer_info[i].dirtree) {
      /* Current file is a directory */

      dot = !strcmp(xfer_info[i].file.filename, "./");

      if(!dot) {
	/* The selected file is NOT './' so we must make a local directory
	 * with the selected file's name
	 */
	if(!make_dir(xfer_info[i].file.filename)) {
	  sprintf(templine, "Local directory %s already exists...",
		  xfer_info[i].file.filename);
	  add_string_to_status(main_cb, templine, NULL);
	  /* We don't stop here because we may want to resume a transfer
	   * of some file(s) in this directory!
	   */
	}
      
	if(!change_dir(xfer_info[i].file.filename)) {
	  puts("error cding into new directory");
	  continue;
	}

	if(!change_remote_dir(xfer_info[i].file.filename)) {
	  puts("error changing remotely");
	  continue;
	}
      }

      /* Recurse into the directory */
      retval = do_retrieve(main_cb, xfer_output, xfer_info[i].dirtree, 
			   xfer_info[i].num_in_dirtree);
      if(retval < 0) {
	change_dir("..");
	return retval;
      }

      if(!dot) {
	/* Must come back out of directory if it's not './' */
	if(!change_remote_dir("..")) {
	  puts("fatal error coming back to normal dir on remote!");
	  exit(1);
	}
	change_dir("..");
      }
    }
    else {
      /* If its a link, get its size now */
      if(xfer_info[i].file.link) {
	if(!get_remote_size(xfer_info[i].file.filename, 
			    &(xfer_info[i].file.size))) {
	  add_string_to_status(main_cb, "Could not determine filesize of"
			       " link, skipping file...", NULL);
	  continue;
	}
	/* must modify total bytes now */
	(xfer_output->total_bytes) += xfer_info[i].file.size;
      }
      /* Initialize dialog stats of file transfer information */
      reset_stats(main_cb, xfer_output, xfer_info[i].file.filename,
		  xfer_info[i].file.size);
      
      if(local_file_exists(xfer_info[i].file.filename, &file_bytes_xferred)) {
	/* Current file exists */

	sprintf(templine, "Local file %s exists already", 
		xfer_info[i].file.filename);
	add_string_to_status(main_cb, templine, NULL);

	if(file_bytes_xferred >= xfer_info[i].file.size) {
	  add_string_to_status(main_cb, 
	    "Already have file with same or bigger size... skipping",
	    NULL);
	  (xfer_output->total_bytes_xferred) += file_bytes_xferred;
	  continue;
	}
	else
	  if(system_can_resume(main_cb->current_state.connection_state.
			       site_info.system_type)) {
	    /* If we are working with a unix ftpd, we should be able to
	     * resume the file
	     */
	    add_string_to_status(main_cb, "Resuming file...", NULL);

	    (xfer_output->total_bytes_xferred) += file_bytes_xferred;

	    /* assuming resuming IS always possible!! */
	    (xfer_output->resume_total_compensation) += file_bytes_xferred;
	    xfer_output->resume_file_compensation = file_bytes_xferred;
	
	    /* must change for other remote system types !!! */
	    fd = get_fd_from_ftpresume(xfer_info[i].file.filename,
				       file_bytes_xferred);
	  }
	  else {
	    /* cannot resume should add prompt for overwrite here */
	    sprintf(templine, "Remote system does not support the resume "
		    "feature. Overwrite local file %s ?", 
		    xfer_info[i].file.filename);
	    warning = warning_y_n_dialog(main_cb->main_window, templine);
	    
	    ch_ptr = (char *) malloc(sizeof(char) * 
				     (strlen(xfer_info[i].file.filename) +
				      1));
	    strcpy(ch_ptr, xfer_info[i].file.filename);
	    XtVaSetValues(warning, XmNuserData, ch_ptr, NULL);
	    
	    XtAddCallback(XmMessageBoxGetChild(warning, XmDIALOG_OK_BUTTON),
			  XmNactivateCallback, delete_download, main_cb);
	    XtAddCallback(XmMessageBoxGetChild(warning, 
					       XmDIALOG_CANCEL_BUTTON),
			  XmNactivateCallback, cancel_del_download, main_cb);
	    
	    XtManageChild(warning);
	    return -2;
	  }
      }
      else {
	/* regular transfer of shiny brand new file */
	xfer_output->resume_file_compensation = 0;
	file_bytes_xferred = 0;
	fd = get_fd_from_ftpget(xfer_info[i].file.filename);
      }

      start_time = xfer_output->last_update_time = 
	time(NULL);   /* Transfer start time */
      downloading = 1;
      
      if(options & OptionRTMode)
	setup_alarm(main_cb->current_state.options.network_timeout);

      while(downloading) {
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	/* Wait up to five seconds. */
	tv.tv_sec = 5;
	tv.tv_usec = 0;        
	sel_retval = select(fd+1, &rfds, NULL, NULL, &tv);
	/* Check for user yelling, "STOP!!!" 
	 * interrupt will call a function that sets xfer_output->kill_xfer
	 * to 1
	 */
	CheckForInterrupt(main_cb, xfer_output->dialog);
	
	if(xfer_output->kill_xfer) {
	  /* User wants to stop */
	  add_string_to_status(main_cb, "Transfer killed!", NULL);
	  close(fd);
	  if(!send_abort())
	    /* unsuccessful kill, remote system is responding very ugly */
	    return -1; 
	  return 0; /* successful kill */	  
	}
	
	if(sel_retval > 0) {
	  if((retval = read(fd, &bread, sizeof(int))) > 0) {
	    if(bread == -1) {
	      /* Something bad happened, maybe connection lost? */
	      close(fd);
	      kill_xfer(NULL, xfer_output, NULL);
	      return -2;
	    }
	    
	    if((bread == -3) && (options & OptionRTMode)) {
	      /* reliable transfer */
	      add_string_to_status(main_cb, "Connection lost!", NULL);
	      
	      close(fd);
	      return(rtm_reconnect(main_cb, xfer_output));
	      
	      /* whole transfer will restart, remote better support resume!  */
	    }
	    
	    /* Modify and update display of running totals */
	    file_bytes_xferred += bread;
	    (xfer_output->total_bytes_xferred) += bread;
	  }
	  else
	    if(retval <= 0)
	      downloading = 0;
	}  
	update_stats(main_cb, xfer_output, &file_bytes_xferred,
		     &(xfer_info[i].file.size),
		     &(xfer_output->total_bytes_xferred),
		     &start_time);
	
	/* signal break? */
	if(sigbreak == 1) {
	  sigbreak = 0;
	  
	  /* Child transfer is stubborn, so lets be forceful and kill it */
	  kill(FtpXferPID(), SIGKILL);
	  close(fd);
	  waitpid(0, NULL, WNOHANG);
	  add_string_to_status(main_cb, "No data transferred in a long time", 
			       NULL);
	  
	  retval = rtm_reconnect(main_cb, xfer_output);
	  XtUnmanageChild(xfer_output->dialog);
	  refresh_screen(main_cb);
	  return retval;
	}
	
	if((sel_retval > 0) && (options & OptionRTMode))
	  /* Reset alarm */
	  setup_alarm(main_cb->current_state.options.network_timeout);
      }
      close(fd);
      waitpid(0, NULL, WNOHANG);
    }
  }
  return 1; /* good xfer */
}
/*************************************************************************/

void delete_download(Widget w, XtPointer client_data, XtPointer call_data)
{
  char *ch_ptr;

  XtVaGetValues(XtParent(w), XmNuserData, &ch_ptr, NULL);

  if(!delete_local_file(ch_ptr)) {
    error_dialog(w, "Could not delete local file!");
    free(ch_ptr);
    return;
  }
  free(ch_ptr);
  download(w, client_data, call_data);
}

void cancel_del_download(Widget w, XtPointer client_data, XtPointer call_data)
{
  char *ch_ptr;

  XtVaGetValues(XtParent(w), XmNuserData, &ch_ptr, NULL);
  
  free(ch_ptr);
}

void setup_alarm(unsigned int timeout)
{
  sigbreak = 0;
  alarm(timeout);
  signal(SIGALRM, sig);
}

void sig(int signo)
{
  sigbreak = 1;
}            
