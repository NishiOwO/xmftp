/* transfer.h - transfer operations/dialogs common to upload and download */

/*************************************************************************/

typedef struct {
  Widget dialog;                    /* The transfer stats main dialog */
  Widget file_label;                /* Filename label */
  Widget fscale;                    /* File percentage scale */
  Widget tscale;                    /* Total (multi-file) percentage scale */
  Widget file_position_label;       /* File position in bytes label */
  Widget total_position_label;      /* Total position in bytes label */
  Widget file_eta_label;            /* File ETA label */
  Widget total_eta_label;           /* Total ETA label */
  Widget file_remaining_label;      /* File remaining in bytes label */
  Widget total_remaining_label;     /* Total remaining in bytes label */
  Widget speed;                     /* Speed of transfer */
  Widget stop;                      /* Stop button */
  unsigned long int total_bytes;                /* Total bytes of files */
  unsigned long int total_bytes_xferred;        /* Total bytes transferred */
  unsigned long int resume_file_compensation;   /* Resume modifications */
  unsigned long int resume_total_compensation;
  time_t last_update_time;
  char kill_xfer;                   /* if goes to 1 user wants to stop */
} Output;

/*************************************************************************/

/* Get the selected files to an XferInfo array
 * remote is 1 if working with remote section (downloads), 
 * 0 for local section (uploads).
 * number is modified to number of files in array
 * returns allocated array else NULL on error
 * return value should NOT be freed, but destroyed with destroy_XferInfo()
 */
XferInfo *get_selected_files_to_XferInfo(Main_CB *main_cb, 
					 int *number, int remote);

/* Get total bytes from xferinfo array */
unsigned long int get_total_bytes(XferInfo *xfer_info, int num_files,
				  int *total_files);

/* Cleanly destroy XferInfo array */
void destroy_XferInfo(XferInfo **xfer_info, int number);

/* Print XferInfo array for debugging purposes */
void print_XferInfo(XferInfo *this, int number);

/* Create the dialog common to uploading and downloading, return info */
Output transfer_common_dialog(Main_CB *main_cb, XferInfo *xfer_info,
			      int num_files);

/* Signal to stop the transfer (modifies variable in struct) 
 * this variable must be checked in order to really stop the transfer
 */
void kill_xfer(Widget w, XtPointer client_data, XtPointer call_data);

/* Reset the transfer dialog stats (zero most) */
void reset_stats(Main_CB *main_cb, Output *xfer_output, char *filename, 
		 unsigned long int size);

/* Update the stats during a transfer */
void update_stats(Main_CB *main_cb, Output *xfer_output,
		  unsigned long int *file_bytes_xferred,
		  unsigned long int *size,
		  unsigned long int *total_bytes_xferred,
		  time_t *start_time);

int rtm_reconnect(Main_CB *main_cb, Output *xfer_output);

/*************************************************************************/
