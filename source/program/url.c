#include "../common/common.h"
#include "url.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

int is_url(char *string);

int is_url(char *string)
{
  return((!(strncmp(string, "ftp://", 6)) && (strlen(string) > 6)));
}

int process_url(char *url, char **login, char **password, char **site, 
		int *port, char **directory, char **file)
{
  char *ch_ptr, *ch_2ptr, *colon;
  int length = 0, end = 0;

  if(!is_url(url))
    return 0;
  
  ch_ptr = &url[6];  /* Get to site name or login info */

  /* get length of sitename and/or login info */
  for(ch_2ptr = ch_ptr; *ch_2ptr && *ch_2ptr != '/'; ch_2ptr++, length++);
  
  if(!(*ch_2ptr))
    end = 1;

  *ch_2ptr = '\0';
  
  if((colon = strchr(ch_ptr, ':'))) {
    /* everything up to colon should be login */
    *colon = '\0';
    *login = (char *) malloc((sizeof(char) * strlen(ch_ptr)) + 1);
    if(!(*login)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    strcpy(*login, ch_ptr);
    ch_ptr = ++colon;
    
    if(!(colon = strchr(ch_ptr, '@'))) {
      /* wrong format url */
      puts("wrong format");
      free(*login);
      return 0;
    }

    *colon = '\0';
    *password = (char *) malloc((sizeof(char) * strlen(ch_ptr)) + 1);
    if(!(*password)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    strcpy(*password, ch_ptr);
    ch_ptr = ++colon;
  }
  else
    *login = *password = NULL;
   
  /* sometimes people use two @'s for some reason ? */
  if(*ch_ptr == '@')
    ch_ptr++;

  if((colon = strchr(ch_ptr, ':'))) {
    /* has port */
    *colon = '\0';
    *site = (char *) malloc((sizeof(char) * strlen(ch_ptr)) + 1);
    if(!(*site)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    strcpy(*site, ch_ptr);
    colon++;
    *port = atoi(colon);
  }
  else {
    /* no port specified */
    *site = (char *) malloc((sizeof(char) * strlen(ch_ptr)) + 1);
    if(!(*site)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    strcpy(*site, ch_ptr);
    *port = -1;
  }
  
  if(end) {
    *directory = NULL;
    *file = NULL;
    return 1;
  }

  *ch_2ptr = '/';

  if(ch_2ptr[strlen(ch_2ptr) - 1] == '/') {
    /* looks like a directory url */
    *directory = (char *) malloc((sizeof(char) * strlen(ch_2ptr)) + 1);

    if(!(*directory)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    
    strcpy(*directory, ch_2ptr);
    *file = NULL;
    return 1;
  }
  else {
    /* url is pointing to a file (maybe) */
    ch_ptr = strrchr(ch_2ptr, '/');
    
    if(!ch_ptr)
      return 0;

    *ch_ptr = '\0';

    *directory = (char *) malloc((sizeof(char) * strlen(ch_2ptr)) + 1);
    
    if(!(*directory)) {
      fprintf(stderr, "malloc error");
      exit(1);
    }
    
    strcpy(*directory, ch_2ptr);
    ch_ptr++;  /* should be now pointing to filename */
    
    *file = (char *) malloc((sizeof(char) * strlen(ch_ptr)) + 1);
    
    strcpy(*file, ch_ptr);
    return 2;
  }
  return 0;
}

    
