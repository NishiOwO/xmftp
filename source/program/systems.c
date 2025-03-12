#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../common/common.h"
#include "misc.h"
#include "systems.h"

typedef struct FListTag {
  FileList file;
  struct FListTag *next;
} Filez;

/* Checks to see what type of file 'query' is by lookup in the file opened
 * by listfp. Used to determine types of remote files on a unix host.
 * returns 0 if query is a regular file, 1 if query is a directory, and
 * 2 if query is a symlink.
 * size is modified to the size of the file (set to 0 if directory)
 */
int unix_is_directory_from_listfp(char *query, FILE *listfp, 
				  unsigned long int *size, time_t *mtime);

FileList *parse_unix_filelist(FILE *listfp, unsigned int
			      *num_files, int skip_hidden);

char *get_next_field(char *curr_field);
int parse_unix_dirline(char *dirline, FileList *file);
void addto_list(Filez **filelist, FileList *file);
void copy_into_filelist(FileList *array, Filez *list);
void kill_filez(Filez *list);
void print_filez(Filez *list);

/****************************************************************************/

int system_can_resume(SystemType s_type)
{
  switch(s_type) {
  case UNIX:
  case WAR_FTPD:
  case FUNNY_AIX:
    return 1;
    break;
  case UNIX_LOOK: /* this could be anything :( */
    return 0;
    break;
  default:
    return 0;
    break;
  }
}

SystemType parse_system_type(char *response)
{
  if(strstr(response, "UNIX"))
    return UNIX;
  else
    if(strstr(response, "Windows_NT"))
      return WINDOWS_NT;
    else
      if(strstr(response, "WAR-FTPD"))
	return WAR_FTPD;
      else
	if(strstr(response, "UNKNOWN"))
	  return FUNNY_AIX;
	else
	  /* Something else */
	  return UNKNOWN;
}

int parse_working_directory(char *server_response, SiteInfo *site_info)
{
  char *begin, *end;
  
  switch(site_info->system_type) {
  case UNIX:
  case UNIX_LOOK:
  case WINDOWS_NT:
  case WAR_FTPD:
  case FUNNY_AIX:
    begin = strchr(server_response, '\"');
    begin++;
    
    /* - 25 should get us to the end of the dirname on normal wu-ftpd */
    if(!(end = strrchr(server_response, '\"')))
      return 0;
    /*
    end = &(server_response[strlen(server_response) - 25]);
    */
    *end = '\0';
    strcpy(site_info->directory, begin);
    return 1;
    break;
  default:
    /* no support for other system types as of yet */
    break;
  }
  return 0;
}


FileList *create_filelist_from_listfp_NA(FILE *listfp, 
					 unsigned int *num_files,
					 SystemType s_type,
					 int skip_hidden)
{  
  /* Currently only following systems supported */
  switch(s_type) {
  case UNIX:
  case UNIX_LOOK:
  case WINDOWS_NT:
  case WAR_FTPD:
  case FUNNY_AIX:
    return(parse_unix_filelist(listfp, num_files, skip_hidden));
    break;
  default:
    break;
  }
  return NULL;
}
/****************************************************************************/

char *convert_time(char *time_str, time_t *mtime)
{
  char work[30], *ch_ptr;
  /* month first */
  int number = 1;
  struct tm filetime;
  time_t junk;
  struct tm *cur_time;

  switch(*time_str) {
  case 'A':
    time_str++;
    switch(*time_str) {
    case 'p':
      /* april */
      filetime.tm_mon = 3;
      break;
    case 'u':
      /* august */
      filetime.tm_mon = 7;
      break;
    default:
      puts(time_str); return NULL;
      break;
    }
    break;
  case 'D':
    /* december */
    filetime.tm_mon = 11;
    break;
  case 'F':
    /* feb */
    filetime.tm_mon = 1;
    break;
  case 'M':
    time_str++;
    switch(*time_str) {
    case 'a':
      time_str++;
      switch(*time_str) {
      case 'y':
	/* may */
	filetime.tm_mon = 4;
	break;
      case 'r':
	/* march */
	filetime.tm_mon = 2;
	break;
      default:
	puts(time_str); return NULL;
	break;
      }
      break;
    default:
      puts(time_str); return NULL;
      break;
    }
    break;
  case 'J':
    time_str++;
    switch(*time_str) {
    case 'a':
      /* jan */
      filetime.tm_mon = 0;
      break;
    case 'u':
      time_str++;
      switch(*time_str) {
      case 'l':
	/* july */
	filetime.tm_mon = 6;
	break;
      case 'n':
	/* june */
	filetime.tm_mon = 5;
	break;
      default:
	puts(time_str); return NULL;
	break;
      }
      break;
    default:
      puts(time_str); return NULL;
      break;
    }
    break;
  case 'N':
    /* nov */
    filetime.tm_mon = 10;
    break;
  case 'O':
    /* oct */
    filetime.tm_mon = 9;
    break;
  case 'S':
    /* sept */
    filetime.tm_mon = 8;
    break;
  default:
    puts(time_str); return NULL;
    break;
  }

  /* now get day */

  while(isalpha(*time_str))
    time_str++;
  time_str++;

  if(!isspace(*time_str)) {
    number = (*time_str++ - '0') * 10;
    number += (*time_str++ - '0');
    filetime.tm_mday = number;
  }
  else {
    time_str++;
    number = (*time_str++ - '0');
    filetime.tm_mday = number;
  }

  /* now get time */
  
  while(isspace(*time_str))
    time_str++;

  strncpy(work, time_str, 5);
  work[5] = '\0';
  
  if(strchr(work, ':')) {
    /* its a time */
    ch_ptr = work;
    number = (*ch_ptr++ - '0') * 10;
    number += (*ch_ptr++ - '0');
    ch_ptr++; /* now past ':' */
    filetime.tm_hour = number;

    number = (*ch_ptr++ - '0') * 10;
    number += (*ch_ptr++ - '0');
    filetime.tm_min = number;

    /* it may be current year */
    junk = time(NULL);
    cur_time = localtime(&junk);

    filetime.tm_year = cur_time->tm_year;

    if(filetime.tm_mon > cur_time->tm_mon)
      /* it must be the year before ?? */
      filetime.tm_year--;

  }
  else {
    /* no time available */
    filetime.tm_hour = 0;
    filetime.tm_min = 0;
    
    /* year should be */
    ch_ptr = work;
    while(isspace(*ch_ptr))
      ch_ptr++;
    filetime.tm_year = atoi(ch_ptr) - 1900;
  }

  filetime.tm_sec = 0;
  filetime.tm_isdst = 0;

  *mtime = mktime(&filetime);
  return(get_next_field(time_str));
}

FileList *parse_unix_filelist(FILE *listfp, unsigned int
			      *num_files, int skip_hidden)
{
  FileList file, *files;
  char *ch_ptr;
  char templine[LINE_MAX];
  int i = 0;
  unsigned int numfiles = 0;
  Filez *filelist = NULL;

  while((ch_ptr = fgets(templine, sizeof(templine), listfp))) {
    /* skip 'total' header */
    if(*templine == '\r' && !templine[1])
      continue;
    if(!strncmp(templine, "total", 5))
      continue;
    /* skip permissions */
    if(!parse_unix_dirline(templine, &file))
      continue;
    if(skip_hidden && (*(file.filename) == '.'))
      continue;
    
    addto_list(&filelist, &file);
    numfiles++;
  }

  numfiles += 2;  /* for . and .. */
  *num_files = numfiles;
  
  files = (FileList *) malloc(sizeof(FileList) * numfiles);

  if(!files) {
    puts("error mallocing filelist");
    exit(1);
  }

  /* Add the two dirs that are always present in a dir listing */
  add_filename(&files[i], "./", 1, 0, 0); i++;
  add_filename(&files[i], "../", 1, 0, 0); i++;

  copy_into_filelist(&files[i], filelist);

  kill_filez(filelist);

  return files;
}

int parse_unix_dirline(char *dirline, FileList *file)
{
  char *ch_ptr, *xtra;
  int i;

  while((ch_ptr = strpbrk(dirline, "\r\n")))
    *ch_ptr = '\0';

  if(strlen(dirline) < 10)
    return 0;

  ch_ptr = dirline;

  /* get passed fields */
  for(i = 0; i < 3; i++)
    ch_ptr = get_next_field(ch_ptr);

  /* should be at group or bytesize field */

  xtra = ch_ptr;
  /* look ahead */
  xtra = get_next_field(xtra);
  if(isdigit(*xtra))
    ch_ptr = xtra;

  /* now we should be definitely at bytesize */

  file->size = atol(ch_ptr);

  ch_ptr = get_next_field(ch_ptr);
  
  file->link = 0;
  file->directory = 0;

  /* links and dirs */
  if(*dirline == 'l') {
    file->link = 1;
    file->size = 0;
    if((xtra = strstr(dirline, "->")))
      *(--xtra) = '\0';
  }
  else
    if(*dirline == 'd') {
      file->directory = 1;
      file->size = 0;
    }
  
  /* now I'm at date */
    
  if(!(xtra = convert_time(ch_ptr, &(file->mtime)))) {
    printf("error getting time");
    puts(ch_ptr);
  }
  /* should be at filename now! */
  
  if(!strcmp(xtra, ".") || !strcmp(xtra, ".."))
    return 0;

  file->filename = (char *) malloc(strlen(xtra) + 2);
  
  strcpy(file->filename, xtra);
  if(file->directory)
    strcat(file->filename, "/");
  return 1;
}

char *get_next_field(char *curr_field)
{
  while(!isspace(*curr_field))
    curr_field++;
  while(isspace(*curr_field))
    curr_field++;
  return(curr_field);
}
  
void copy_into_filelist(FileList *array, Filez *list)
{
  int type;

  while(list) {
    if(list->file.directory)
      type = 1;
    else
      if(list->file.link)
	type = 2;
      else
	type = 0;
    
    add_filename(array, list->file.filename, type, list->file.size,
		 list->file.mtime);
    list = list->next;
    array++;
  }
}

void addto_list(Filez **filelist, FileList *file)
{
  Filez *newfile;

  /* stack based */
  newfile = (Filez *) malloc(sizeof(Filez));

  newfile->next = *filelist;

  *filelist = newfile;

  memcpy(newfile, file, sizeof(FileList));
}

void print_filez(Filez *list)
{
  while(list) {
    printf("%s\t%ld\td%d\tl%d\n", list->file.filename, list->file.size,
	   list->file.directory, list->file.link);
    list = list->next;
  }
}

void kill_filez(Filez *list)
{
  Filez *next;
  
  while(list) {
    if(list->file.filename)
      free(list->file.filename);
    next = list->next;
    free(list);
    list = next;
  }
}
