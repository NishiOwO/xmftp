/* common.h - Common application specific definitions (no UI information) */

/***************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#include <time.h>

/* Number of seconds of no data transferred that constitutes a timeout
 * When recieving files, if no data is recieved in NET_TIMEOUT seconds
 * the site is redialed and the transfer is restarted (resuming where it
 * left off) IF RTMode is enabled. This is the default setting that can
 * be changed via the program.
 */
#define NET_TIMEOUT 30

/* The amount of seconds to wait before redialing in RTMode */
#define RTMReconnectPause 30

#define HOSTNAME_MAX 255
#define LINE_MAX 255

/* Directory within user's HOME dir that will hold xmftp configuration */
#define CONFIG_DIR ".xmftp"
#define SITELIST_FILE CONFIG_DIR"/sitelist"
#define OPTIONS_FILE CONFIG_DIR"/options"

#define VERSION_INFO "xmftp v1.0.4 by Viraj Alankar (valankar@bigfoot.com)"
#define OPT_VERSION 100

/* The state of a directory listing including sort method and long or short
 * listing (currently sorting on anything other than filename is
 * unimplemented)
 */
typedef struct {
  enum { SHORT, LONG } listing;
  enum { NAME, SIZE, DATE } sorting;
  enum { ASCENDING, DESCENDING } order;
  char skip_hidden;
} DirState;

/* The remote system types supported by xmftp. UNIX_LOOK represents a 
 * Unix system that does not understand the FTP SYST command. 
 */
typedef enum { UNIX, UNIX_LOOK, WINDOWS_NT, WAR_FTPD, FUNNY_AIX,
	       UNKNOWN } SystemType;

/* Site information - the structure stored in the flat file database */
typedef struct {
  SystemType system_type;
  char login[LINE_MAX];
  char password[LINE_MAX];
  char hostname[LINE_MAX];
  char directory[LINE_MAX];      /* Current directory or last directory */
  char comment[LINE_MAX];        /* User comment about site */
  char nickname[LINE_MAX-5];
  char port[5];
} SiteInfo;

/* The current connection state holding information about the site, the type
 * of connection, and whether we are connected or not. Currently only
 * BINARY xfer_type is supported. We need to determine if connection is
 * made via the Site Manager so the 'last directory' information is saved
 * in the database upon disconnect.
 */
typedef struct {
  enum { ASCII, BINARY } xfer_type;
  char connected;                /* are we connected or not? */
  SiteInfo site_info;            /* if connected, site information here */
  char connect_from_site_mgr;    /* Connected from site manager? */
} ConnectionState;

typedef unsigned int OptionFlagType;

typedef struct {
  OptionFlagType flags;
  unsigned int network_timeout;
} OptionState;

/* Generic application 'state' representation */
typedef struct {
  DirState dir_state;       /* Local directory state */
  DirState rdir_state;      /* remote directory state */
  ConnectionState connection_state;
  OptionState options;
} State;

/* Information for each file in a directory list.
 * Most likely will add permission bits to this later as well.
 */
typedef struct {
  char *filename;
  unsigned long int size; /* Probably should use off_t instead */
  char directory;         /* is it a directory ? */
  char link;              /* is it a link ? */
  time_t mtime;           /* modification time */
} FileList;

/* Information/filelist for downloading/uploading */
typedef struct xfer_info_type {
  FileList file;
  int num_in_dirtree;              /* for recursive fetches/sends */
  struct xfer_info_type *dirtree;  /* for recursive fetches/sends */
} XferInfo;

#define OptionRTMode          (1<<0)
#define OptionShowConnMsg     (1<<1)
#define OptionLargeFont       (1<<2)
#define OptionDontShowHidLoc  (1<<3)
#define OptionDontShowHidRem  (1<<4)
#define OptionDontCropFNames  (1<<5)

#define DEFAULT_OPTION_FLAGS    (OptionShowConnMsg)
#endif
