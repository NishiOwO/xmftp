/* site_db.c - Site Manager database operations */

/***************************************************************************/

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include "site_db.h"

/***************************************************************************/

#define KEY "xmft"
#define DB_VERSION 104

typedef struct s_list {
  SiteInfo site;
  struct s_list *next;
} SiteInfoList;

SiteInfoList *list = NULL;

void make_backup(char *filename);
int SiteInfoList_oread_list(char *filename);
int read_field(char *store, FILE *fp);
int write_field(char *source, FILE *fp);
/***************************************************************************/

int SiteInfoList_traverse_list(SiteInfo *store_site, int reset)
{
  static SiteInfoList *where = NULL;
  
  if(reset) {
    where = list;
    return 1;
  }

  if(!where)
    return 0;
 
  /* Store record information */
  store_site->system_type = where->site.system_type;
  strcpy(store_site->login, where->site.login);
  strcpy(store_site->password, where->site.password);
  strcpy(store_site->hostname, where->site.hostname);
  strcpy(store_site->directory, where->site.directory);
  strcpy(store_site->comment, where->site.comment);
  strcpy(store_site->nickname, where->site.nickname);
  
  where = where->next;
  return 1;
}
/***************************************************************************/

int SiteInfoList_read_list(char *filename)
{
  FILE *fp;
  SiteInfo site;
  int i = 0, version = 0;
  int bread;

  memset(&site, 0, sizeof(SiteInfo));

  fp = fopen(filename, "r");
  if(!fp)
    return 0;
  SiteInfoList_destroy_list();

  /* version info */
  bread = fread(&version, sizeof(version), 1, fp);

  switch(version) {
  case DB_VERSION:
    if(fgetc(fp)) {
      puts("non null marker in sitelist file");
      fclose(fp);
      return 0;
    }
    
    while(!feof(fp)) {
      /* read systype */
      bread = fread(&(site.system_type), sizeof(SystemType), 1, fp);
      if(bread != 1)
	break;
      read_field(site.login, fp);
      read_field(site.password, fp);
      read_field(site.hostname, fp);
      read_field(site.directory, fp);
      read_field(site.comment, fp);
      read_field(site.nickname, fp);
      read_field(site.port, fp);
      SiteInfoList_add_site(&site);
      i++;
    }
    if(!feof(fp))
      puts("not at end of file, sitelist fatal");
    
    fclose(fp);
    
    return i;
    break;
  default:
    /* non revisioned */
    /* looks like an older version */
    /* printf("Converting your sitelist file to the new version...\n"); */
    fclose(fp);
    make_backup(filename);
    return(SiteInfoList_oread_list(filename));
    break;
  }

}
/***************************************************************************/

void SiteInfoList_destroy_list(void)
{
  SiteInfoList *kill = NULL;

  while(list) {
    kill = list;
    list = list->next;
    free(kill);
  }
}
/***************************************************************************/

int SiteInfoList_add_site(SiteInfo *site)
{
  SiteInfoList *previous = NULL, *current = NULL, *next = NULL, *new = NULL;
  int retval;

  /* get to position where to insert */
  for(current = list; current; current = current->next) {
    retval = strcmp(site->nickname, current->site.nickname);
    if(retval == 0)
      /* already exists */
      return 0;
    if(retval < 0)
      break;
    previous = current;
  }
  
  new = (SiteInfoList *) calloc(1, sizeof(SiteInfoList));
  
  if(previous) {
    next = previous->next;
    previous->next = new;
    new->next = next;
  }
  else {
    next = list;
    new->next = next;
    list = new;
  }
  
  /* add the info */
  new->site.system_type = site->system_type;
  strcpy(new->site.login, site->login);
  strcpy(new->site.password, site->password);
  strcpy(new->site.hostname, site->hostname);
  strcpy(new->site.directory, site->directory);
  strcpy(new->site.comment, site->comment);
  strcpy(new->site.nickname, site->nickname);
  strcpy(new->site.port, site->port);
  return 1;
}
/***************************************************************************/

int SiteInfoList_find_site(char *nickname, SiteInfo *store_site)
{
  SiteInfoList *current;

  for(current = list; current; current = current->next) {
    if(!strcmp(nickname, current->site.nickname))
      break;
  }
  
  if(!current)
    return 0;
  
  store_site->system_type = current->site.system_type;
  strcpy(store_site->login, current->site.login);
  strcpy(store_site->password, current->site.password);
  strcpy(store_site->hostname, current->site.hostname);
  strcpy(store_site->directory, current->site.directory);
  strcpy(store_site->comment, current->site.comment);
  strcpy(store_site->nickname, current->site.nickname);
  strcpy(store_site->port, current->site.port);
  return 1;
}
/***************************************************************************/

int SiteInfoList_delete_site(char *nickname, char *full_site)
{
  SiteInfoList *current = NULL, *previous = NULL, *kill = NULL;

  for(current = list; current; current = current->next) {
    if(!nickname) {
      if(!strcmp(full_site, current->site.hostname))
	break;
    }
    else {
      if(!strcmp(nickname, current->site.nickname))
	break;
    }
    previous = current;
  }
  if(!current)
    /* not found */
    return 0;

  kill = current;
  if(previous)
    previous->next = current->next;
  else
    list = current->next;
  free(current);
  return 1;
}
/***************************************************************************/

int SiteInfoList_save_sites(char *filename)
{
  FILE *fp;
  SiteInfoList *current;
  int version = DB_VERSION;
  char *ch_ptr;

  fp = fopen(filename, "w");
  if(!fp) 
    return 0;

  /* version, then NULL */
  fwrite(&version, sizeof(version), 1, fp);
  fputc(0, fp);

  for(current = list; current; current = current->next) {
    fwrite(&(current->site.system_type), sizeof(SystemType), 1, fp);
    ch_ptr = current->site.login;
    /* 7 fields */
    write_field(current->site.login, fp);
    write_field(current->site.password, fp);
    write_field(current->site.hostname, fp);
    write_field(current->site.directory, fp);
    write_field(current->site.comment, fp);
    write_field(current->site.nickname, fp);
    write_field(current->site.port, fp);
  }
  fclose(fp);
  return 1;
}
/***************************************************************************/

int write_field(char *source, FILE *fp)
{
  while(*source) {
    fputc(*source, fp);
    source++;
  }
  fputc(0, fp);
  return 1;
}

int read_field(char *store, FILE *fp)
{
  char *new = store;
  int blah, i = 0;

  for(;;) {
    blah = fgetc(fp);
    if(blah == EOF)
      break;
    *new = (char) blah;  
    if(!blah)
      break;
    new++;
    i++;
  }
  return i;
}

int SiteInfoList_oread_list(char *filename)
{
  FILE *fp;
  SiteInfo site;
  int i = 0;

  fp = fopen(filename, "r");
  if(!fp)
    return 0;
  SiteInfoList_destroy_list();

  while(fread(&site, sizeof(SiteInfo), 1, fp) == 1) {
    SiteInfoList_add_site(&site);
    i++;
  }
  if(!feof(fp))
    puts("not at end of file, sitelist file may be from older version");
  
  fclose(fp);
  return i;
}

void make_backup(char *filename)
{
  char *newname;

  newname = malloc((2 * strlen(filename)) + 10);

  sprintf(newname, "cp %s %s.bak", filename, filename);

  system(newname);
  free(newname);
}
