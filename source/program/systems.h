/* Creates an array of FileList structures from the NLST (ftp) and long
 * listing files. This is to be used for processing remote directories.
 * nlistfp is an opened NLST file, listfp is an opened long listing file.
 * num_files is changed to reflect the number of files in the array
 * s_type is the remote system type
 * returns array, NULL on failure.
 *
 * return value should NOT be freed by rather destroyed with destroy_filelist()
 */
FileList *create_filelist_from_listfp_NA(FILE *listfp, 
					 unsigned int *num_files,
					 SystemType s_type,
					 int skip_hidden);

int parse_working_directory(char *server_response, SiteInfo *site_info);
SystemType parse_system_type(char *response);
int system_can_resume(SystemType s_type);
char *convert_time(char *time_str, time_t *mtime);







