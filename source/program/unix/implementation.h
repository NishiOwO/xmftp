/* implementation.h - Platform specific functions - Currently Unix
 *
 * I have tried to seperate the Unix specifics of xmftp from the main program.
 * However, operations such as transferring files still are Unix dependent
 * (file descriptors/pipes).
 *
 * These functions should be called from misc.c and nowhere else.
 */

/****************************************************************************/

#include "../../common/common.h"
#include "../misc.h"

/****************************************************************************/

/* Deletes the local file filename
 * returns 1 on success, 0 on failure
 */
int unix_delete_local_file(char *filename);

/* Removes the local directory dir
 * returns 1 on success, 0 on failure
 */
int unix_remove_local_dir(char *dir);

/* Renames the local file old to new
 * returns 1 on success, 0 on failure
 */
int unix_rename(char *old, char *new);

/* Changes the current local working directory to newdir
 * returns 1 on success, 0 on failure
 */
int change_local_directory(char *newdir);

/* Creates the local directory newdir
 * returns 1 on success, 0 on failure
 */
int make_local_directory(char *newdir);

/* Generates the full path to the Site Manager database file
 * (it is created if it does not exist)
 * returns pathname
 * This value should NOT be freed.
 */
char *unix_get_sitelist_filename(void);

/* Generates the user's email address in the form user@host.domain
 * returns email
 * This value should NOT be freed.
 */
char *get_unix_email(void);

/* Gets the current local working directory
 * returns directory
 * This value SHOULD be freed.
 */
char *get_local_directory(void);

/* Determines if a file exists (used for download resumes)
 * returns filesize if file exists, 0 otherwise
 */
int file_exists(char *filename, char *dirname, unsigned long int *size);

/* dont free return value */
char *unix_get_options_filename(void);

/* Creates an array of FileList structures corresponding to the files
 * in the directory named by dirname. This array is sorted by filename in
 * ascending order. num_files is changed to reflect the number of files
 * in the array. if skip_hidden == 1 skip hidden files
 * returns array, NULL on failure.
 *
 * return value should NOT be freed by rather destroyed with destroy_filelist()
 */
FileList *create_filelist_local_NA_or_TD(char *dirname, 
					 unsigned int *num_files,
					 int which, int skip_hidden);

/* Performs sort on file source, storing output in dest */
void sortfile(char *source, char *dest);

/****************************************************************************/
