/* Functions to build menus */

#include <Xm/Xm.h>

/* needs the following structure */
typedef struct _menu_item {
    char        *label;         /* the label for the item */
    WidgetClass *class;         /* pushbutton, label, separator... */
    char         mnemonic;      /* mnemonic; NULL if none */
    char        *accelerator;   /* accelerator; NULL if none */
    char        *accel_text;    /* to be converted to compound string */
    void       (*callback)();   /* routine to call; NULL if none */
    XtPointer    callback_data; /* client_data for callback() */
    struct _menu_item *subitems; /* pullright menu items, if not NULL */
} MenuItem;

Widget
BuildMenu(Widget parent, int menu_type, char *menu_title, char menu_mnemonic,
	  Boolean tear_off, MenuItem *items);


