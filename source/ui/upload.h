/* upload.h - upload operation */

/**************************************************************************/

/* Perform upload of files described in xfer_info (# being num_files)
 * basename is currently ignored
 */
int perform_upload(Main_CB *main_cb, char *basename, XferInfo *xfer_info,
		   int num_files);

/**************************************************************************/
