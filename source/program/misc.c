/* misc.c - Miscellaneous operations
 *
 * Most of these functions only make calls to functions in implementation.h
 * This is so there is less modifying of code when moving to different
 * platforms
 */

/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include "misc.h"
#include "unix/implementation.h"
#include "ftp.h"
#include "systems.h"

/****************************************************************************/

#ifndef FILENAME_MAX
#define FILENAME_MAX LINE_MAX
#endif

/****************************************************************************/

int name_compare(const void *one, const void *two);
int name_decompare(const void *one, const void *two);
int date_compare(const void *one, const void *two);
int date_decompare(const void *one, const void *two);

FileList *create_filelist_remote_NA(unsigned int *num_files, 
				    SystemType s_type, int skip_hidden);

/****************************************************************************/

char *get_email(void)
{
  static char *email = NULL;

  if(!email)
    email = get_unix_email();

  return email;
}
/****************************************************************************/

char *get_sitelist_filename(void)
{
  static char *file = NULL;
  
  if(!file)
    file = unix_get_sitelist_filename();

  return file;
}
/****************************************************************************/

char *get_options_filename(void)
{
  static char *file = NULL;
  
  if(!file)
    file = unix_get_options_filename();

  return file;
}

int local_file_exists(char *filename, unsigned long int *size)
{
  return(file_exists(filename, ".", size));
}
/****************************************************************************/

int make_dir(char *newdir)
{
  return(make_local_directory(newdir));
}
/****************************************************************************/

int delete_local_file(char *filename)
{
  return(unix_delete_local_file(filename));
}
/****************************************************************************/

int rename_local_file(char *old, char *new)
{
 return(unix_rename(old, new));
}
/****************************************************************************/

int remove_local_dir(char *dir)
{
  return(unix_remove_local_dir(dir));
}
/****************************************************************************/

int change_dir(char *newdir)
{
  return(change_local_directory(newdir));
}
/****************************************************************************/

char *get_local_dir(void)
{
  return(get_local_directory());
}
/****************************************************************************/

FileList *generate_local_filelist(char *dirname, unsigned int *num_files,
				  DirState dir_state)
{
  switch(dir_state.sorting) {
  case NAME:
    switch(dir_state.order) {
    case ASCENDING:
      return(create_filelist_local_NA_or_TD(dirname, num_files, 0,
					    dir_state.skip_hidden));
      break;
    case DESCENDING:
      return(create_filelist_local_NA_or_TD(dirname, num_files, 2,
					    dir_state.skip_hidden));
    default:
      puts("undefined dirstate order");
      break;
    }
    break;
  case DATE:
    switch(dir_state.order) {
    case ASCENDING:
      return(create_filelist_local_NA_or_TD(dirname, num_files, 1,
					    dir_state.skip_hidden));
      break;
    case DESCENDING:
      return(create_filelist_local_NA_or_TD(dirname, num_files, 3,
					    dir_state.skip_hidden));
      break;
    default:
      puts("undefined dirstate order");
      break;
    }
    break;
  default:
    puts("undefined dirstate sorting");
    break;
  }
  return NULL;
}
/****************************************************************************/

void destroy_filelist(FileList **files, int num_files)
{
  FileList *templist = *files;
  int i;
  
  for(i = 0; i < num_files; i++)
    free(templist[i].filename);
  free(templist);
  
  *files = NULL;
}
/****************************************************************************/

void add_filename(FileList *file, char *filename, char type, 
		  unsigned long int size, time_t mtime)
{
  file->filename = (char *) malloc(sizeof(char) * (strlen(filename) + 1));

  if(!(file->filename)) {
    puts("error mallocing filename string");
    exit(1);
  }

  strcpy(file->filename, filename);

  file->directory = (type == 1);
  file->link = (type == 2);
  file->size = size;
  file->mtime = mtime;
}
/****************************************************************************/

FileList *generate_remote_filelist(unsigned int *num_files, 
				   DirState dir_state, SystemType s_type)
{
  FileList *files;

  switch(dir_state.sorting) {
  case NAME:
    switch(dir_state.order) {
    case ASCENDING:
      files = create_filelist_remote_NA(num_files, s_type, 
					dir_state.skip_hidden);
      sort_filelist(files, *num_files, 0);
      return files;
      break;
    case DESCENDING:
      files = create_filelist_remote_NA(num_files, s_type,
					dir_state.skip_hidden);
      sort_filelist(files, *num_files, 1);
      return files;
      break;
    default:
      break;
    }
    break;
  case DATE:
    switch(dir_state.order) {
    case ASCENDING:
      files = create_filelist_remote_NA(num_files, s_type,
					dir_state.skip_hidden);
      sort_filelist(files, *num_files, 2);
      return files;
      break;
    case DESCENDING:
      files = create_filelist_remote_NA(num_files, s_type,
					dir_state.skip_hidden);
      sort_filelist(files, *num_files, 3);
      return files;
      break;
    default:
      break;
    }
    break;
  default:
    puts("undefined dirstate sorting");
    break;
  }
  return NULL;
}
/****************************************************************************/

FileList *create_filelist_remote_NA(unsigned int *num_files, SystemType
				    s_type, int skip_hidden)
{
  FILE *listfp;
  char listfile[FILENAME_MAX];
  FileList *files;

  tmpnam(listfile);

  create_listfile(listfile);
  
  listfp = fopen(listfile, "r");

  if(!listfp) {
    fprintf(stderr, "Error reading listing files! %p\n",
	    listfp);
    return NULL;
    exit(1);
  }
  
  files = create_filelist_from_listfp_NA(listfp, num_files, s_type, 
					 skip_hidden);

  fclose(listfp);

  remove(listfile);

  return files;
}
/****************************************************************************/
  
int sort_filelist(FileList *files, int num_files, int sort_type)
{
  if(!files)
    return 0;
  switch(sort_type) {
  case 0:
    /* on name ascending , skip . and .. */
    qsort(&files[2], num_files - 2, sizeof(FileList), name_compare);
    return 1;
    break;
  case 1:
    /* on name descending */
    qsort(&files[2], num_files - 2, sizeof(FileList), name_decompare);
    return 1;
    break;
  case 2:
    qsort(&files[2], num_files - 2, sizeof(FileList), date_compare);
    return 1;
    break;
  case 3:
    qsort(&files[2], num_files - 2, sizeof(FileList), date_decompare);
    return 1;
    break;    
  default:
    break;
  }
  return 0;
}

int name_compare(const void *one, const void *two)
{
  FileList *f1, *f2;

  f1 = (FileList *) one;
  f2 = (FileList *) two;
  return(strcmp(f1->filename, f2->filename));
}

int name_decompare(const void *one, const void *two)
{
  FileList *f1, *f2;

  f1 = (FileList *) one;
  f2 = (FileList *) two;
  return(strcmp(f2->filename, f1->filename));
}

int date_compare(const void *one, const void *two)
{
  FileList *f1, *f2;

  f1 = (FileList *) one;
  f2 = (FileList *) two;
  
  if(f1->mtime < f2->mtime)
    return -1;  
  if(f1->mtime > f2->mtime)
    return 1;
  return 0;
}

int date_decompare(const void *one, const void *two)
{
  FileList *f1, *f2;

  f1 = (FileList *) one;
  f2 = (FileList *) two;
  
  if(f1->mtime < f2->mtime)
    return 1;  
  if(f1->mtime > f2->mtime)
    return -1;
  return 0;
}
