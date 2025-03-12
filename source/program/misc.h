/* misc.h - Miscellaneous operations */

/***************************************************************************/

#include "../common/common.h"
#include "../ui/xmftp.h"

/***************************************************************************/

/* Gets the current local working directory
 * returns allocated string containing full pathname
 * This value should be freed
 */
char *get_local_dir(void);

/* Creates a path pointing to the user's Site Manager database file
 * This path is created if it does not exist.
 * returns the full pathname of Site Manager database file
 * This value should NOT be freed
 */
char *get_sitelist_filename(void);
char *get_options_filename(void);
int sort_filelist(FileList *files, int num_files, int sort_type);

/* Create's the user's email address in the form user@host.domain
 * returns email address
 * This value should NOT be freed
 */
char *get_email(void);

/* Determines if a local file filename exists or not
 * returns 2 if its a dir, 1 if its a file, 0 if doesn't exist
 * if size is non-null, filesize is stored there
 */
int local_file_exists(char *filename, unsigned long int *size);

/* Generates an array of FileList structures representing the files in
 * the local directory specified by dirname in the sort order
 * specified in dir_state. Use "." as dirname for current directory.
 * num_files is modified to contain the number of files in the array 
 * returns allocated array or NULL on failure.
 *
 * array should NOT be freed but destroyed with destroy_filelist() 
 */
FileList *generate_local_filelist(char *dirname, unsigned int *num_files,
				  DirState dir_state);

/* Generates an array of FileList structures representing the files in
 * the current remote directory of the host system of type s_type.
 * The sort order is specified in dir_state.
 * num_files is modified to contain the number of files in the array.
 * returns allocated array or NULL on failure.
 *
 * array should NOT be freed but destroyed with destroy_filelist() 
 */
FileList *generate_remote_filelist(unsigned int *num_files, 
				   DirState dir_state, SystemType s_type);

/* Adds filename to the FileList structure pointed to by file.
 * The file information is specified in size and type:
 *
 *           if type == 0, file is a regular file
 *           if type == 1, file is a directory
 *           if type == 2, file is a symlink
 *
 */
void add_filename(FileList *file, char *filename, char type, 
		  unsigned long int size, time_t mtime);

/* Destroy the allocated structures returned by the previous functions 
 * num_files contains the number of files in the array
 */
void destroy_filelist(FileList **files, int num_files);

/* Change the local directory to newdir
 * returns 1 on success, 0 on failure
 */
int change_dir(char *newdir);

/* Make the local directory newdir
 * returns 1 on success, 0 on failure
 */
int make_dir(char *newdir);

/* Remove the local directory dir
 * returns 1 on success, 0 on failure
 */
int remove_local_dir(char *dir);

/* Delete the local file filename
 * returns 1 on success, 0 on failure
 */
int delete_local_file(char *filename);

/* Rename the local file old to new
 * returns 1 on success, 0 on failure
 */
int rename_local_file(char *old, char *new);

/***************************************************************************/
