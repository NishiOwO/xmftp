/* fill_filelist.h - filling of directory listings on screen */

/***********************************************************************/

#include "xmftp.h"

/***********************************************************************/

/* Adds the files described by 'files' to the ScrolledList slist.
 * num_files is number of items in the array 'files'.
 * if pt_size == 0 use small font, else large font
 * if nocrop == 1 dont crop long filenames
 */
void fill_slist(FileList *files, Widget slist, unsigned int num_files,
		int pt_size, int nocrop);

/***********************************************************************/
