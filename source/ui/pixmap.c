#include <stdio.h>
#include <Xm/Xm.h>
#include <X11/xpm.h>
#include <stdlib.h>
#include "xmftp.h"
#include "pixmap.h"
#include <malloc.h>

/* Get all the icons 16bit */
#include "icons/connect.xpm"
#include "icons/disconnect.xpm"
#include "icons/download.xpm"
#include "icons/quit.xpm"
#include "icons/logo.xpm"
#include "icons/reconnect.xpm"
#include "icons/site_mgr.xpm"
#include "icons/upload.xpm"
#include "icons/view.xpm"

/* 8bit icons */
#include "icons/8bit/logo.xpm"
#include "icons/8bit/connect.xpm"
#include "icons/8bit/disconnect.xpm"
#include "icons/8bit/download.xpm"
#include "icons/8bit/quit.xpm"
#include "icons/8bit/reconnect.xpm"
#include "icons/8bit/site_mgr.xpm"
#include "icons/8bit/upload.xpm"
#include "icons/8bit/view.xpm"

char *icon_filenames[] = {
  "connect.xpm", "disconnect.xpm", "download.xpm", "logo.xpm", "quit.xpm",
  "reconnect.xpm", "site_mgr.xpm", "upload.xpm", "view.xpm" };

Pixmap load_xmftp_pixmap(Main_CB *main_cb, Icon which, Widget parent)
{
  Pixmap pix;
  char *envar;
  char **read_16icon, **read_8icon, *templine;
  int icon_index;

  envar = getenv("XMFTP_ICON_DIR");
  
  switch(which) {
  case CONNECT:
    read_16icon = connect_16bit_icon;
    read_8icon = connect_8bit_icon;
    icon_index = 0;
    break;
  case DISCONNECT:
    read_16icon = disconnect_16bit_icon;
    read_8icon = disconnect_8bit_icon;
    icon_index = 1;
    break;
  case DOWNLOAD:
    read_16icon = download_16bit_icon;
    read_8icon = download_8bit_icon;
    icon_index = 2;
    break;
  case LOGO:
    read_16icon = logo_16bit_icon;
    read_8icon = logo_8bit_icon;
    icon_index = 3;
    break;
  case QUIT:
    read_16icon = quit_16bit_icon;
    read_8icon = quit_8bit_icon;
    icon_index = 4;
    break;
  case RECONNECT:
    read_16icon = reconnect_16bit_icon;
    read_8icon = reconnect_8bit_icon;
    icon_index = 5;
    break;
  case SITE_MGR:
    read_16icon = site_mgr_16bit_icon;
    read_8icon = site_mgr_8bit_icon;
    icon_index = 6;
    break;
  case UPLOAD:
    read_16icon = upload_16bit_icon;
    read_8icon = upload_8bit_icon;
    icon_index = 7;
    break;
  case VIEW:
    read_16icon = view_16bit_icon;
    read_8icon = view_8bit_icon;
    icon_index = 8;
    break;
  default:
    return 0;
    break;
  }

  if(envar) {
    /* environment variable is set */

    templine = (char *) malloc(strlen(envar) + 
			       strlen(icon_filenames[icon_index]) + 1);
    sprintf(templine, "%s/%s", envar, icon_filenames[icon_index]);

    if(!(pix = gimme_pix(main_cb, templine, parent))) {
      fprintf(stderr, "\nusing your setting of XMFTP_ICON_DIR\n\n"
	      "could not open %s\n", templine);
      free(templine);
      error_pix();
      exit(1);
    }
    free(templine);
    return pix;
  }

  if(!(pix = gimme_pix_from_data(main_cb, read_16icon, 
				 parent))) {
    if(!(pix = gimme_pix_from_data(main_cb, read_8icon, 
				   parent))) {
      error_pix();
      exit(1);
    }
  }
  return pix;
}  

Pixmap gimme_pix_from_data(Main_CB *main_cb, char **data, Widget toolbar)
{
  Display *dpy = main_cb->dpy;
  Pixmap pix_returned, mask;
  XpmAttributes xpmatts;
  XpmColorSymbol transparentColor[1] = {{NULL, "none", 0}}; 
  Pixel bg;

  XtVaGetValues(toolbar, XmNbackground, &bg, NULL);

  
  transparentColor[0].pixel = bg;
  xpmatts.closeness = 40000;
  xpmatts.valuemask = XpmColorSymbols | XpmCloseness;  
  xpmatts.colorsymbols = transparentColor;
  xpmatts.numsymbols = 1;

  if(XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), data, 
			     &pix_returned, &mask, &xpmatts) != XpmSuccess) {
    return 0;
  }
  
  return pix_returned;
}  

Pixmap gimme_pix(Main_CB *main_cb, char *filename, Widget toolbar)
{
  Display *dpy = main_cb->dpy;
  Pixmap pix_returned, mask;
  XpmAttributes xpmatts;
  XpmColorSymbol transparentColor[1] = {{NULL, "none", 0}}; 
  Pixel bg;

  XtVaGetValues(toolbar, XmNbackground, &bg, NULL);

  
  transparentColor[0].pixel = bg;
  xpmatts.closeness = 40000;
  xpmatts.valuemask = XpmColorSymbols | XpmCloseness;  
  xpmatts.colorsymbols = transparentColor;
  xpmatts.numsymbols = 1;

  if(XpmReadFileToPixmap(dpy, DefaultRootWindow(dpy), filename,
			 &pix_returned, &mask, &xpmatts) != XpmSuccess) {
    return 0;
  }

  return pix_returned;
}

void error_pix(void)
{
  fprintf(stderr, "\nAn icon could not be loaded.\n\n");

}
/*************************************************************************/
