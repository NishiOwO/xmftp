/* fill_filelist.c - functions to setup the directory listing windows */

/**************************************************************************/

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include "fill_filelist.h"

/**************************************************************************/

void fill_slist(FileList *files, Widget slist, unsigned int num_files,
		int pt_size, int nocrop)
{
  int i;
  XmString *items;
  char templine[LINE_MAX + 70];
  char templine2[10];
  char temp[30];
  char time_str[30];
  struct tm *mod_tm;

  items = (XmString *) malloc(sizeof(XmString) * num_files);
  
  if(!items) {
    puts("error mallocing items");
    exit(1);
  }

  /* Create nicely formatted lines for each filename */
  for(i = 0; i < num_files; i++) {    
    if(files[i].mtime) {
      mod_tm = localtime(&(files[i].mtime));

      strftime(time_str, sizeof(time_str), "%b %d %Y %I:%M%p", mod_tm);
    }
    else
      *time_str = '\0';

    if(files[i].directory)
      sprintf(templine2, "%-9s", "<DIR>");
    else
      if(files[i].link)
	sprintf(templine2, "%-9s", "<LINK>");
      else
	sprintf(templine2, "%9ld", files[i].size);

    /* make it look pretty and consistent for long filenames */
    if(strlen(files[i].filename) > 28) {
      if(!nocrop) {
	strncpy(temp, files[i].filename, 25);
	temp[25] = '\0';
	strcat(temp, "...");
	sprintf(templine, "%-29s%s  %s   ", temp, templine2, time_str);
      }
      else 
	sprintf(templine, "%s\t%s   %s   ", files[i].filename, templine2,
		time_str);
    }
    else
      sprintf(templine, "%-29s%s  %s   ", files[i].filename, templine2,
	      time_str);
    items[i] = XmStringCreate(templine, pt_size ? "LG_LIST_TAG" : 
			      "SM_LIST_TAG");
  }
  
  XmListAddItems(slist, items, num_files, 0);

  for(i = 0; i < num_files; i++)
    XmStringFree(items[i]);

  free(items);
}
/**************************************************************************/

