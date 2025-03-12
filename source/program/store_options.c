#include "../common/common.h"
#include "store_options.h"
#include "misc.h"
#include <stdio.h>
#include "unix/implementation.h"

void read_options(char *file, OptionState *options);
void write_options(char *file, OptionState *options);
void create_options_file(char *file, OptionState *options);

int load_options(OptionState *options)
{
  char *file;
  unsigned long int size;

  file = get_options_filename();

  if(!file) {
    fprintf(stderr,"error getting config file\n");
    exit(1);
  }
  
  if(!file_exists(file, NULL, &size)) 
    /* must create new options file with default settings */
    create_options_file(file, options);

  read_options(file, options);
  return 1;
}

int save_options(OptionState *options)
{
  char *file;
  unsigned long int size;

  file = get_options_filename();

  if(!file) {
    fprintf(stderr,"error getting config file\n");
    exit(1);
  }
  
  if(local_file_exists(file, &size))
    /* must delete */
    delete_local_file(file);

  write_options(file, options);
  return 1;
}
  
void read_options(char *file, OptionState *options)
{
  int version;
  FILE *fp;

  fp = fopen(file, "r");

  if(!fp) {
    fprintf(stderr, "could not read options file %s\n", file);
    exit(1);
  }
  
  fread(&version, sizeof(version), 1, fp);

  if(version != OPT_VERSION) {
    fprintf(stderr, "version mismatch of options file\n");
    exit(1);
  }

  fread(options, sizeof(OptionState), 1, fp);
  fclose(fp);
}

void write_options(char *file, OptionState *options)
{
  int version = OPT_VERSION;
  FILE *fp;

  fp = fopen(file, "w");

  if(!fp) {
    fprintf(stderr, "could not create options file %s\n", file);
    exit(1);
  }
  
  fwrite(&version, sizeof(version), 1, fp);

  fwrite(options, sizeof(OptionState), 1, fp);
  fclose(fp);
}

void create_options_file(char *file, OptionState *options)
{
  FILE *fp;
  int version = OPT_VERSION;

  fp = fopen(file, "w");
  
  if(!fp) {
    fprintf(stderr, "could not create options file %s\n", file);
    exit(1);
  }
  
  /* Default options */
  options->flags = 0;
  options->flags |= DEFAULT_OPTION_FLAGS;
  options->network_timeout = NET_TIMEOUT;

  /* write a version magic number */
  fwrite(&version, sizeof(version), 1, fp);
  fwrite(options, sizeof(OptionState), 1, fp);

  fclose(fp);
}
