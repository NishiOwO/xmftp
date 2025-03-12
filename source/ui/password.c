/* Password Text Widget */

#include <Xm/Xm.h>
#include <Xm/Text.h>
#include "password.h"
#include <malloc.h>

/* Initialize the specified mode in pf */
void PField_InitMode(PassField *pf);

/* modifyverify callback for text widget */
void check_passwd(Widget w, XtPointer client_data, XtPointer call_data);

/* add and remove the callbacks */
void remove_cbs(Widget textf);
void add_cbs(PassField *pf);

void PField_SetString(PassField *pf, char *string)
{
  char *str_ptr = NULL;

  if(pf->passwd) {
    free(pf->passwd);
    pf->passwd = NULL;
  }

  if(pf->mode == 1) {
    remove_cbs(pf->textf);
    if(strlen(string)) {
      pf->passwd = (char *) malloc(strlen(string) + 1);
      strcpy(pf->passwd, string);
      str_ptr = (char *) malloc(strlen(string) + 1);
      strcpy(str_ptr, string);
      memset(str_ptr, '*', strlen(string));
      str_ptr[strlen(string)] = '\0';      
      XmTextSetString(pf->textf, str_ptr);
      free(str_ptr);
    }
    else 
      /* for blank string */
      XmTextSetString(pf->textf, "");
    add_cbs(pf);
  }
  else {
    remove_cbs(pf->textf);
    /* regular field */
    XmTextSetString(pf->textf, string);
  }
}

PassField *PField_Create(Widget parent, char mode)
{
  PassField *pf;
  
  pf = (PassField *) malloc(sizeof(PassField));

  pf->textf = XtVaCreateManagedWidget("text_w", xmTextWidgetClass, parent,
				     NULL);
  pf->mode = mode;
  pf->passwd = NULL;

  PField_InitMode(pf);
  return pf;
}

PassField *PField_CreateFromText(Widget text, char mode)
{
  PassField *pf;
  
  pf = (PassField *) malloc(sizeof(PassField));

  pf->textf = text;
  pf->mode = mode;
  pf->passwd = NULL;

  PField_InitMode(pf);
  return pf;
}  

void PField_Destroy(PassField *pf)
{
  if(pf->passwd) {
    free(pf->passwd);
    pf->passwd = NULL;
  }

  free(pf);
}

void PField_SetMode(PassField *pf, int mode)
{
  pf->mode = mode;
  PField_InitMode(pf);
}

void remove_cbs(Widget textf)
{
  XtRemoveAllCallbacks(textf, XmNmodifyVerifyCallback);
  XtRemoveAllCallbacks(textf, XmNactivateCallback);    
}

void add_cbs(PassField *pf)
{
  XtAddCallback(pf->textf, XmNmodifyVerifyCallback, check_passwd, pf);
  XtAddCallback(pf->textf, XmNactivateCallback, check_passwd, pf);
}

void PField_InitMode(PassField *pf)
{
  if(pf->passwd) {
    free(pf->passwd);
    pf->passwd = NULL;
  }
  remove_cbs(pf->textf);
  XmTextSetString(pf->textf, "");

  switch(pf->mode) {
  case 0:
    break;
  case 1:
    add_cbs(pf);
    break;
  default:
    break;
  }
}

char *PField_GetString(PassField *pf)
{
  if((pf->mode == 1) && pf->passwd)
    return(pf->passwd);
  else
    return(XmTextGetString(pf->textf));
}


void check_passwd(Widget w, XtPointer client_data, XtPointer call_data)
{
  char *new;
  PassField *pf = (PassField *) client_data;
  XmTextVerifyCallbackStruct *cbs =
    (XmTextVerifyCallbackStruct *) call_data;

  if (cbs->reason == XmCR_ACTIVATE)
    return;
 
  if(cbs->text->length > 1) {
    cbs->doit = False; /* don't allow paste operations; make the */
    return;            /* user type the password! */
  }

  if(cbs->startPos < cbs->endPos) {
    memmove(&(pf->passwd[cbs->startPos]), &(pf->passwd[cbs->endPos]),
    strlen(pf->passwd) - cbs->endPos + 1);

    if( cbs->text->length == 0 ) /* then just a delete, not a replace */
      return;           
    
  }

  new = malloc(cbs->text->length + 1);

  strncpy(new, cbs->text->ptr, cbs->text->length );

  new[cbs->text->length]='\0';

  if(pf->passwd) {
    pf->passwd = realloc(pf->passwd, strlen(pf->passwd) + 
			   cbs->text->length + 1);
    memmove(&(pf->passwd[cbs->startPos + cbs->text->length]),
	    &(pf->passwd[cbs->startPos]),
    strlen(pf->passwd) - cbs->startPos + 1 );
    
    memcpy( &(pf->passwd[cbs->startPos]), new, cbs->text->length );
  } 
  else 
    pf->passwd = new;

  memset(cbs->text->ptr, '*', cbs->text->length); /* makes it all stars */
}
