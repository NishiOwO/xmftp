/* setup_cbs.c - General layout callbacks */

/**************************************************************************/

#include <Xm/List.h>
#include <malloc.h>
#include "setup_cbs.h"
#include "operations.h"
#include "remote_refresh.h"

/**************************************************************************/

/* Callback that deselects selected item from a List (for status section) */
void dummy_status(Widget w, XtPointer client_data, XtPointer call_data);

/**************************************************************************/

void setup_callbacks(Main_CB *main_callback)
{
  
  /* a click on the CD button will bring up changedir dialog */
  XtAddCallback(main_callback->local_section.cd, 
		XmNactivateCallback,
		chdir_locally, main_callback);

  XtAddCallback(main_callback->remote_section.cd, XmNactivateCallback,
		chdir_remotely, main_callback);

  /* double-click in dir section causes traversal or view */
  XtAddCallback(main_callback->local_section.dir_list, 
		XmNdefaultActionCallback, traverse_locally, main_callback);

  XtAddCallback(main_callback->remote_section.dir_list,
		XmNdefaultActionCallback, traverse_remotely, main_callback);

  XtAddCallback(main_callback->status_output,
		XmNbrowseSelectionCallback, dummy_status, main_callback);

}

void dummy_status(Widget w, XtPointer client_data, XtPointer call_data)
{
  int *position_list = NULL;
  int position_count;

  if(XmListGetSelectedPos(w, &position_list, &position_count) == True) 
    XmListDeselectPos(w, *position_list);
  
  if(position_list)
    free(position_list);
}
