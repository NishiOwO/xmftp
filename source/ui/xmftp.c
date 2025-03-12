/* xmftp.c - Main program */

/***************************************************************************/

#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xmftp.h"
#include "menu_creation.h"
#include "layout.h"
#include "operations.h"
#include "setup_cbs.h"
#include "connection.h"
#include "../program/misc.h"
#include "../program/store_options.h"

/***************************************************************************/

/* The font of the directory listings is LIST_TAG */

String fallbacks[] = { 
  "*fontList: fixed,-*-courier-medium-r-*--*-180-*=LG_LIST_TAG,"
  "fixed,-*-courier-medium-r-*--*-120-*=SM_LIST_TAG",
  "*background: gray",
  "*foreground: black",
  NULL };

#define DEFAULT_WIDTH 900
#define DEFAULT_HEIGHT 600

/***************************************************************************/

/* Sets up all the menus and their callbacks */
void create_menus(Widget main_window, Main_CB *main_callback);

/* Initializes the Main_CB structure to default values */
void setup_defaults(Main_CB *main_callback);

/* Initializes the local directory section to reflect the current directory */
void init_windows(Main_CB *main_callback);

/***************************************************************************/

void main(int argc, char **argv)
{
  Widget toplevel, main_window;
  XtAppContext app;
  Main_CB main_callback;    /* This variable is the gist of our program */
  int width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;

  toplevel = XtVaAppInitialize(&app, *argv, NULL, 0,
			       &argc, argv, fallbacks, NULL);
  
  main_callback.toplevel = toplevel;
  main_callback.dpy = XtDisplay(toplevel);

  /*  
  printf("%d %d\n", WidthOfScreen(XtScreen(toplevel)), HeightOfScreen(XtScreen(toplevel)));
  */

  if(HeightOfScreen(XtScreen(toplevel)) < DEFAULT_HEIGHT) 
    height = HeightOfScreen(XtScreen(toplevel)) * .90;

  if(WidthOfScreen(XtScreen(toplevel)) < DEFAULT_WIDTH)
    width = WidthOfScreen(XtScreen(toplevel)) * .90;

  main_window = XtVaCreateWidget("main_w", xmMainWindowWidgetClass,
				 toplevel,
				 XmNcommandWindowLocation, 
				 XmCOMMAND_BELOW_WORKSPACE,
				 XmNwidth, width,
				 XmNheight, height,
				 NULL);
  main_callback.main_window = main_window;
  
  /* Setup the overall layout */
  create_layout(main_window, &main_callback);
  
  /* Setup the menu bar */
  create_menus(main_window, &main_callback);

  /* setup some callback operations for the widgets */
  setup_callbacks(&main_callback);

  /* setup some defaults */
  setup_defaults(&main_callback);

  /* init local window */
  init_windows(&main_callback);
  
  XtManageChild(main_window);
  XtRealizeWidget(toplevel);

  if(argc > 1) {
    command_line_url(&main_callback, argv[1]);
  }

  XtAppMainLoop(app);
}
  
/* Initializes the local directory section to reflect the current directory */
void init_windows(Main_CB *main_callback)
{
  char *local_dirname;

  local_dirname = get_local_dir();
  
  XtVaSetValues(main_callback->local_section.cwd_field, XmNvalue,
		local_dirname, NULL);

  /* Do directory listing update to current directory */
  refresh_local_dirlist(main_callback, local_dirname);

  free(local_dirname);
}

/* Some configuration defaults */
void setup_defaults(Main_CB *main_callback)
{
  main_callback->current_state.connection_state.xfer_type = BINARY;
  main_callback->current_state.connection_state.connected = 0;
  main_callback->current_state.connection_state.connect_from_site_mgr = 0;
  main_callback->current_state.dir_state.listing = LONG;
  main_callback->current_state.dir_state.sorting = NAME;
  main_callback->current_state.dir_state.order = ASCENDING;
  main_callback->current_state.rdir_state.listing = LONG;
  main_callback->current_state.rdir_state.sorting = NAME;
  main_callback->current_state.rdir_state.order = ASCENDING;
  main_callback->local_section.files = NULL;
  main_callback->remote_section.files = NULL;
  main_callback->local_section.num_files = 0;
  main_callback->remote_section.num_files = 0;

  load_options(&(main_callback->current_state.options));

  main_callback->current_state.dir_state.skip_hidden = 
    (main_callback->current_state.options.flags) & OptionDontShowHidLoc;
  main_callback->current_state.rdir_state.skip_hidden = 
    (main_callback->current_state.options.flags) & OptionDontShowHidRem;
}

/* Setup the menus */
void create_menus(Widget main_window, Main_CB *main_callback)
{
  Widget menubar;
  XmString ftp, commands, local, remote, options;

  ftp = XmStringCreateLocalized("FTP");
  commands = XmStringCreateLocalized("Commands");
  local = XmStringCreateLocalized("Local");
  remote = XmStringCreateLocalized("Remote");
  options = XmStringCreateLocalized("Options");

  menubar = XmVaCreateSimpleMenuBar(main_window, "menubar",
				  XmVaCASCADEBUTTON, ftp, 'F',
				  XmVaCASCADEBUTTON, commands, 'C',
				  XmVaCASCADEBUTTON, local, 'L',
				  XmVaCASCADEBUTTON, remote, 'R',
				  XmVaCASCADEBUTTON, options, 'O',
				  NULL);
  XmStringFree(ftp);
  XmStringFree(commands);
  XmStringFree(local);
  XmStringFree(remote);
  XmStringFree(options);

  create_FTP_menu(menubar, main_callback);
  create_Commands_menu(menubar, main_callback);
  create_Local_menu(menubar, main_callback);
  create_Remote_menu(menubar, main_callback);
  create_Options_menu(menubar, main_callback);

  /*
  create_Help_menu(menubar, main_callback);
  */

  create_Popup_menu(main_callback);
  XtManageChild(menubar);
}


