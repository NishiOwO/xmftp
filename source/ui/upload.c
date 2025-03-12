/* upload.c - Upload operations */

/**************************************************************************/

#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include "xmftp.h"
#include "transfer.h"
#include "../program/ftp.h"
#include "operations.h"
#include "../program/misc.h"
#include "remote_refresh.h"
#include "upload.h"
#include "connection.h"

/**************************************************************************/

int do_put(Main_CB *main_cb, Output *xfer_output, XferInfo *xfer_info,
	   int num_files);

/**************************************************************************/

int perform_upload(Main_CB *main_cb, char *basename, XferInfo *xfer_info,
		   int num_files)
{
  Output xfer_output;

  xfer_output = transfer_common_dialog(main_cb, xfer_info, num_files);
  XtAddCallback(xfer_output.stop, XmNactivateCallback, kill_xfer, 
		&xfer_output);
  /* afterstep causing probs? map to disarm as well */
  XtAddCallback(xfer_output.stop, XmNdisarmCallback, kill_xfer, 
		&xfer_output);
  
  if(do_put(main_cb, &xfer_output, xfer_info, num_files) < 0) {
    /* Connection is messed */
    add_string_to_status(main_cb, "Connection is corrupt", NULL);
    kill_connection();
    reconnect(main_cb->toplevel, main_cb, NULL);
    waitpid(0, NULL, WNOHANG);
  }
  waitpid(0, NULL, WNOHANG);
  XtDestroyWidget(xfer_output.dialog);
  refresh_remote_dirlist(main_cb);
  return 1;
}
/**************************************************************************/
  
int do_put(Main_CB *main_cb, Output *xfer_output, XferInfo *xfer_info,
	   int num_files)
{
  unsigned long int file_bytes_xferred = 0;
  int i, fd, bread, retval;
  time_t start_time;
  int dot;
  fd_set rfds;
  struct timeval tv;
  int sel_retval;
  char uploading = 0;

  /* for recursion */
  if(xfer_output->kill_xfer)
    return 0;
  
  for(i = 0; i < num_files; i++) {
    if(xfer_info[i].dirtree) {
      dot = !strcmp(xfer_info[i].file.filename, "./");
      
      if(!dot) {
	if(!make_remote_dir(xfer_info[i].file.filename)) {
	  add_string_to_status(main_cb, "Could not make remote directory",
			       NULL);
	  add_string_to_status(main_cb, get_ftp_last_resp(), NULL);
	}
      
	if(!change_remote_dir(xfer_info[i].file.filename)) {
	  puts("error cding into new directory");
	  continue;
	}

	if(!change_dir(xfer_info[i].file.filename)) {
	  puts("error changing locally (???)");
	  continue;
	}
      }

      if(do_put(main_cb, xfer_output, xfer_info[i].dirtree, 
		xfer_info[i].num_in_dirtree) == -1) {
	change_remote_dir("..");
	return -1;
      }

      if(!dot) {
	change_dir("..");
	if(!change_remote_dir("..")) {
	  puts("fatal error coming back to normal dir on remote!");
	  exit(1);
	}
      }
    }
    else {
      if(xfer_info[i].file.link) {
	if(local_file_exists(xfer_info[i].file.filename,
			     &(xfer_info[i].file.size)) != 1) {
	  /* link is maybe a dir */
	  add_string_to_status(main_cb, "Could not determine filesize of"
			       " link, skipping file...", NULL);
	  continue;
	}
	/* must modify total bytes now */
	(xfer_output->total_bytes) += xfer_info[i].file.size;
      }
      
      reset_stats(main_cb, xfer_output, xfer_info[i].file.filename,
		  xfer_info[i].file.size);
      xfer_output->resume_file_compensation = 0;
      file_bytes_xferred = 0;
      fd = get_fd_from_ftpput(xfer_info[i].file.filename);

      start_time = xfer_output->last_update_time = 
	time(NULL);   /* Transfer start time */
      uploading = 1;

      while(uploading) {
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	/* Wait up to five seconds. */
	tv.tv_sec = 5;
	tv.tv_usec = 0;        
	sel_retval = select(fd+1, &rfds, NULL, NULL, &tv);
	if(sel_retval) {
	  if((retval = read(fd, &bread, sizeof(int))) > 0) {
	    
	    /*	    while((retval = read(fd, &bread, sizeof(int))) > 0) { */

	    if(bread == -1) {
	      kill_xfer(NULL, xfer_output, NULL);
	      return -2;
	    }
	    CheckForInterrupt(main_cb, xfer_output->dialog);
	    if(xfer_output->kill_xfer) {
	      add_string_to_status(main_cb, "Transfer killed!", NULL);
	      close(fd);
	      if(!send_abort())
		return -1;  /* unsuccessful kill */
	      return 0; /* successful kill */	  
	    }
	    file_bytes_xferred += bread;
	    (xfer_output->total_bytes_xferred) += bread;
	  }
	  else
	    uploading = 0;
	}
	update_stats(main_cb, xfer_output, &file_bytes_xferred,
		     &(xfer_info[i].file.size),
		     &(xfer_output->total_bytes_xferred),
		     &start_time);
      }
      close(fd);
      waitpid(0, NULL, WNOHANG);
    }
  }
  return 1; /* good xfer */
}
/**************************************************************************/
      
	
