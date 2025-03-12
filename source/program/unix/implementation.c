/* implementation.c - Unix specific functions */

/****************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "implementation.h"

/****************************************************************************/

#ifndef FILENAME_MAX
#define FILENAME_MAX LINE_MAX
#endif

/* The directory listing depends on this command */
#define FILELIST_CMD_SORTBY_ASNAME "ls -1 -A -p"
#define FILELIST_CMD_SORTBY_DENAME "ls -1 -A -p -r"
#define FILELIST_CMD_SORTBY_ASTIME "ls -1 -A -p -t -r"
#define FILELIST_CMD_SORTBY_DETIME "ls -1 -A -p -t"

#define SORTCMD "sort"

time_t get_modtime(char *file);

/****************************************************************************/

time_t get_modtime(char *file)
{
  struct stat buf;

  if(stat(file, &buf))
    return 0;
  return buf.st_mtime;
}

char *unix_get_sitelist_filename(void)
{
  char *file;
  unsigned long int size;

  file = (char *) malloc(sizeof(char) * FILENAME_MAX);

  /* Determine user's home dir and xmftp configuration dir */
  sprintf(file, "%s/%s", getenv("HOME"), CONFIG_DIR);
  if(!file_exists(file, ".", &size))
    make_local_directory(file);
  
  /* Site database file is stored in xmftp configuration dir */
  sprintf(file, "%s/%s", getenv("HOME"), SITELIST_FILE);
  return file;
}

char *unix_get_options_filename(void)
{
  char *file;
  unsigned long int size;

  file = (char *) malloc(sizeof(char) * FILENAME_MAX);

  /* Determine user's home dir and xmftp configuration dir */
  sprintf(file, "%s/%s", getenv("HOME"), CONFIG_DIR);
  if(!file_exists(file, ".", &size))
    make_local_directory(file);
  
  /* Site database file is stored in xmftp configuration dir */
  sprintf(file, "%s/%s", getenv("HOME"), OPTIONS_FILE);
  return file;
}

/****************************************************************************/

int unix_rename(char *old, char *new)
{
  return(!rename(old, new));
}
/****************************************************************************/

int unix_delete_local_file(char *filename)
{
  return(!remove(filename));
}
/****************************************************************************/

int unix_remove_local_dir(char *dir)
{
  return(!rmdir(dir));
}
/****************************************************************************/

char *get_unix_email(void)
{
  struct passwd *pwd;
  char *line;
  char hostname[LINE_MAX];
  char domain[LINE_MAX];

  line = (char *) malloc(sizeof(char) * (LINE_MAX*3));

  /* Try to find out user's email address (needs work) */
  pwd = getpwuid(getuid());
  gethostname(hostname, sizeof(hostname));
  getdomainname(domain, sizeof(domain));

  if(!strcmp(domain, "(none)"))
    *domain = '\0';

  sprintf(line, "%s@%s%s", pwd->pw_name, hostname, domain);
  
  return line;
}
/****************************************************************************/

int make_local_directory(char *newdir)
{
  return(!mkdir(newdir, 0755));
}
/****************************************************************************/

void sortfile(char *source, char *dest)
{
  char templine[LINE_MAX];

  sprintf(templine, "%s %s > %s 2>/dev/null", SORTCMD, source, dest);
  system(templine);                                           
}
/****************************************************************************/

int change_local_directory(char *newdir)
{
  return(!chdir(newdir));
}
/****************************************************************************/

char *get_local_directory(void)
{
  char *name;

  name = (char *) malloc(sizeof(char) * FILENAME_MAX);
  if(!name) {
    puts("Error mallocing");
    exit(1);
  }

  if(!getcwd(name, FILENAME_MAX)) {
    puts("buffer overflow getting current working dir");
    exit(1);
  }
  return name;
}
/****************************************************************************/

FileList *create_filelist_local_NA_or_TD(char *dirname, 
					 unsigned int *num_files,
					 int which, int skip_hidden)
{
  FILE *dirlist;
  char templine[LINE_MAX], *ch_ptr;
  unsigned int numfiles = 0, i = 0;
  FileList *files = NULL;
  unsigned long int size;
  char *type;

  switch(which) {
  case 0:
    type = FILELIST_CMD_SORTBY_ASNAME;
    break;
  case 1:
    type = FILELIST_CMD_SORTBY_ASTIME;
    break;
  case 2:
    type = FILELIST_CMD_SORTBY_DENAME;
    break;
  case 3:
    type = FILELIST_CMD_SORTBY_DETIME;
    break;
  }

  dirlist = popen(type, "r");
  
  /* We'll make one pass to get the number of files */
  while(fgets(templine, sizeof(templine), dirlist)) {
    if(skip_hidden && *templine == '.')
      continue;
    numfiles++;
  }

  pclose(dirlist);
  numfiles += 2;   /* for . and .. */
  *num_files = numfiles;

  files = (FileList *) malloc(sizeof(FileList) * numfiles);

  if(!files) {
    puts("error mallocing filelist");
    exit(1);
  }
  
  add_filename(&files[i], "./", 1, 0, 0); i++;
  add_filename(&files[i], "../", 1, 0, 0); i++;

  /* Now another pass adding the files */
  dirlist = popen(type, "r");
  while(fgets(templine, sizeof(templine), dirlist)) {
    if((ch_ptr = strchr(templine, '\n')))
      *ch_ptr = '\0';
    if(skip_hidden && *templine == '.')
      continue;
    if(strchr(templine, '/'))
      add_filename(&files[i], templine, 1, 0, get_modtime(templine));
    else
      if(templine[strlen(templine)-1] == '@') {
	templine[strlen(templine)-1] = '\0';
	add_filename(&files[i], templine, 2, 0, get_modtime(templine));
      }
      else {
	file_exists(templine, dirname, &size);
	add_filename(&files[i], templine, 0, size, get_modtime(templine));
      }
    i++;
  }
  pclose(dirlist);

  return files;
}
/****************************************************************************/

int file_exists(char *filename, char *dirname, unsigned long int *size)
{
  char templine[LINE_MAX];
  struct stat buf;

  sprintf(templine, "%s/%s", dirname ? dirname : "", filename);

  if(stat(templine, &buf))
    return 0;

  if(size)
    *size = buf.st_size;
  if(S_ISDIR(buf.st_mode))
    return 2;
  else
    return 1;
}
/****************************************************************************/






