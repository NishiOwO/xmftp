/* xmftp.h - UI specific information */

/**************************************************************************/

#ifndef XMFTP_H
#define XMFTP_H

#include <Xm/Xm.h>
#include "../common/common.h"

/* define this to do away with the color hungry toolbar */
/* #define NOTOOLBAR */

/* This is a directory listing section (2 on screen, one for local, one for
 * remote). Contains widgets and information common to directory listings.
 */
typedef struct {
  Widget cwd_field;       /* The current working directory TextField */
  Widget dir_list;        /* The directory listing ScrolledList */
  Widget cd;              /* pushbutton to change directories */
  FileList *files;        /* Array of files */
  unsigned int num_files; /* number of files in array */
} DirSection;

/* There will be one Main_CB structure representing all information about
 * the current application status, the address of which is passed to and
 * from almost every function in xmftp
 */
typedef struct {
  DirSection local_section;       /* The local/left directory section */
  DirSection remote_section;      /* The remote/right directory section */
  Widget status_output;           /* status output ScrolledList */
  Widget status_bottom_output;    /* Generic label at bottom of main window */
  Widget main_window;             /* MainWindowWidget */
  Widget toplevel;                /* Toplevel shell of application */
  Display *dpy;                   /* Display of toplevel shell (for pixmaps) */
  State current_state;            /* Current overall state of application */
} Main_CB; 

#endif

/**************************************************************************/
