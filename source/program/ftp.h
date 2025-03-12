/* ftp.h - FTP remote site functions */

/***************************************************************************/

#include "../common/common.h"
#include <stdio.h>

/***************************************************************************/

/* Establish a connection to the host specified by hostname.
 * returns 1 on success, 0 on failure
 * last_response will contain the last response from the server
 * (this is NOT to be freed)
 * if fp is non-NULL, output from server is written to the FILE *
 */
int make_connection(char *hostname, char **last_response, int port,
		    FILE *fp);

/* Log in to the server (connection must already be made) with login
 * and password.
 * returns 1 on success, 0 on failure
 * last_response will contain the last response from the server
 * (this is NOT to be freed)
 * if fp is non-NULL, output from server is written to the FILE *
 */
int login_server(char *login, char *password, char **last_response,
		 FILE *fp);

/* Determines the system type and stores result in s_type.
 * returns 1 on successful determination, 0 otherwise
 * last_response will contain the last response from the server
 * (this is NOT to be freed)
 */
int get_system_type(SystemType *s_type, char **last_response);

/* Destroy the current established connection */
void kill_connection(void);

/* Determines the remote site's current working directory and store this
 * result in the site_info structure.
 * returns 1 on successful determination, 0 otherwise
 */
int get_remote_directory(SiteInfo *site_info);

/* Determines size of file filename on remote host (current connection).
 * returns 1 and stores size in size if successful, 0 if failure
 */
int get_remote_size(char *filename, unsigned long int *size);

/* Generate a long listing of remote files and store it in the filename
 * listfile
 */
void create_listfile(char *listfile);

/* Generate an NLST listing of remote files and store it in the filename
 * nlistfile
 */
void create_nlistfile(char *nlistfile);

/* Change the remote site's current directory to dirname
 * returns 1 on success, 0 on failure
 */
int change_remote_dir(char *dirname);

/* Create the remote directory dirname
 * returns 1 on success, 0 on failure
 */
int make_remote_dir(char *dirname);

/* Remove the remote directory dirname (directory must be empty)
 * returns 1 on success, 0 on failure
 */
int remove_remote_dir(char *dirname);

/* Delete the remote file filename
 * returns 1 on success, 0 on failure
 */
int delete_remote_file(char *filename);

/* Rename the remote file old to new
 * returns 1 on success, 0 on failure
 */
int rename_remote_file(char *old, char *new);

/* Establish an binary upload of filename to the current connected site
 * returns a file descriptor that is the read end of a pipe
 * A series of int's will be transmitted through this pipe, the calling
 * function should grab these integers, which are values representing
 * the amount of bytes sent (the calling function should keep a running
 * total, which at the end of the transfer will sum to the size of the file)
 *
 * If at any point these values become negative, an error has occured.
 */
int get_fd_from_ftpput(char *filename);

/* Establish a binary download of filename from the current connected site
 * returns a file descriptor that is the read end of a pipe
 * A series of int's will be transmitted through this pipe, the calling
 * function should grab these integers, which are values representing
 * the amount of bytes received (the calling function should keep a running
 * total, which at the end of the transfer will sum to the size of the file)
 *
 * If at any point these values become negative, an error has occured.
 */
int get_fd_from_ftpget(char *filename);

/* Establish a binary resume download of filename from the current 
 * connected site resuming at position offset.
 * returns a file descriptor that is the read end of a pipe
 * A series of int's will be transmitted through this pipe, the calling
 * function should grab these integers, which are values representing
 * the amount of bytes received (the calling function should keep a running
 * total, which at the end of the transfer will, when added with offset, 
 * equal the size of the file)
 *
 * If at any point these values become negative, an error has occured.
 */
int get_fd_from_ftpresume(char *filename, unsigned long int offset);

/* Return the last response of the remote system
 * This result should NOT be freed.
 */
char *get_ftp_last_resp(void);

/* Send an ABOR (abort) command to the server with a standard expected
 * response. 
 * returns 1 if abort successfully sent, 0 if the response seems out 
 * of the ordinary.
 */
int send_abort(void);

/* Execute a SITE command on the server, store output in fp */
int execute_site_cmd(char *cmd, FILE *fp);
/***************************************************************************/

