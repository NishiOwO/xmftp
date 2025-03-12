/* ftp.c - FTP functions
 * Most of these are calls to the FTP library functions in ftplib.h
 * This is so a different library can be used easily without too much effort
 */

/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "../ftplib/ftplib.h"
#include "ftp.h"
#include "systems.h"

/****************************************************************************/

#ifndef FILENAME_MAX
#define FILENAME_MAX LINE_MAX
#endif


/****************************************************************************/

/* Holds information on the current connection */
netbuf *connection = NULL;

/****************************************************************************/

int rename_remote_file(char *old, char *new)
{
  return(FtpRename(old, new, connection));
}
/****************************************************************************/

int delete_remote_file(char *filename)
{
  return(FtpDelete(filename, connection));
}
/****************************************************************************/

int remove_remote_dir(char *dirname)
{
  return(FtpRmdir(dirname, connection));
}
/****************************************************************************/

int make_remote_dir(char *dirname)
{
  char temp[FILENAME_MAX], *ch_ptr;

  /* take care of when dirname has '/' in it */  
  strcpy(temp, dirname);
  if((ch_ptr = strchr(temp, '/')))
    *ch_ptr = '\0';

  return(FtpMkdir(temp, connection));
}
/****************************************************************************/

char *get_ftp_last_resp(void)
{
  return(FtpLastResponse(connection));
}
/****************************************************************************/

int send_abort(void)
{
  if(!FtpSendCmd("abor", '2', connection))
    return 0;
  
  if(strstr(FtpLastResponse(connection), "ABOR"))
    return 1;
  else
    return 0;
}
/****************************************************************************/

int get_fd_from_ftpresume(char *filename, unsigned long int offset)
{
  return(FtpResume(filename, filename, offset, 'I', connection));
}
/****************************************************************************/

int get_fd_from_ftpget(char *filename)
{
  return(FtpGet(filename, filename, 'I', connection));
}
/****************************************************************************/

int get_fd_from_ftpput(char *filename)
{
  return(FtpPut(filename, filename, 'I', connection));
}
/****************************************************************************/

int change_remote_dir(char *dirname)
{
  return(FtpChdir(dirname, connection));
}
/****************************************************************************/

void create_listfile(char *listfile)
{
  FtpDir(listfile, ".", connection);
}
/****************************************************************************/

void create_nlistfile(char *nlistfile)
{
  FtpNlst(nlistfile, ".", connection);
}
/****************************************************************************/

int get_remote_directory(SiteInfo *site_info)
{
  if(!connection)
    return 0;
  
  FtpSendCmd("pwd", '2', connection);

  /* We need to parse the output of PWD response */
  return(parse_working_directory(FtpLastResponse(connection), site_info));
}      
/****************************************************************************/

int get_remote_size(char *filename, unsigned long int *size)
{
  char templine[LINE_MAX];
  char *ch_size;

  sprintf(templine, "size %s", filename);

  if(FtpSendCmd(templine, '2', connection)) {
    ch_size = FtpLastResponse(connection);
    ch_size += 4;  /* get to size */
    *size = atol(ch_size);
    return 1;
  }
  return 0;
}

void kill_connection(void)
{
  if(connection) {
    FtpQuit(connection);
    connection = NULL;
  }
}
/****************************************************************************/

int get_system_type(SystemType *s_type, char **last_response)
{
  char tempfile[FILENAME_MAX];
  char templine[LINE_MAX];
  char *ch_ptr;
  FILE *fp;

  if(!connection) {
    *last_response = NULL;
    return 0;
  }
  if(FtpSendCmd("syst", '2', connection)) {
    /* Supported remote type is UNIX only for now */
    *last_response = FtpLastResponse(connection);
    *s_type = parse_system_type(*last_response);
  }
  else {
    /* lets try another 'hack' for unix ftpd's that dont have syst cmd 
     * SunOS seems to fall in this category - MUST FIX THIS!
     */
    tmpnam(tempfile);
    FtpDir(tempfile, ".", connection);
    fp = fopen(tempfile, "r");
    if(!fp)
      *s_type = UNKNOWN;
    else {
      fgets(templine, sizeof(templine), fp);
      while((ch_ptr = strpbrk(templine, "\r\n")))
	*ch_ptr = '\0';
      if(strstr(templine, "total")) {
	/* Remote 'looks' like UNIX but can't be sure */
	*s_type = UNIX_LOOK;
      }
      fclose(fp);
      remove(tempfile);
    }
  }
  return 1;
}
/****************************************************************************/

int make_connection(char *hostname, char **last_response, int port,
		    FILE *fp)
{
  int ret_value;

  if(!FtpConnectFP(hostname, &connection, port, fp))
    ret_value = 0;
  else
    ret_value = 1;
  *last_response = FtpLastResponse(connection);
  return ret_value;
}
/****************************************************************************/

int login_server(char *login, char *password, char **last_response,
		 FILE *fp)
{
  int ret_value;

  if(!FtpLoginFP(login, password, connection, fp))
    ret_value = 0;
  else
    ret_value = 1;

  *last_response = FtpLastResponse(connection);
  return ret_value;
}
/****************************************************************************/
    
int execute_site_cmd(char *cmd, FILE *fp)
{
  return(FtpSiteFP(cmd, connection, fp));
}

