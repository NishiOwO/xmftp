/* download.h */

/***************************************************************************/

#include "xmftp.h"

/***************************************************************************/

/* Performs a download from the current FTP connection of files
 * described in xfer_info, which is an array of size num_files.
 * returns 1 upon success, 0 upon failure.
 */
int perform_download(Main_CB *main_cb, char *basename, XferInfo *xfer_info,
		     int num_files);

/***************************************************************************/


