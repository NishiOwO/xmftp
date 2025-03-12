/* Password Text Widget - *'s out user entry of characters */

/* data structure of widget */
typedef struct {
  Widget textf;
  char mode;
  char *passwd;
} PassField;

/* Creates a managed Password Text Widget of specified mode
 * mode 1 = Use *'ing out of characters
 * mode 0 = Do not * out characters (normal Text Widget behavior)
 * returns allocated PassField that is used in other functions, do not free!
 */
PassField *PField_Create(Widget parent, char mode);

/* Same as previous except this 'coerces' an already existing Text widget into
 * a Password Text widget
 */
PassField *PField_CreateFromText(Widget text, char mode);

/* Destroys (frees) password field inherent data. Does not destroy
 * the widget. Widget can be destroyed by its parent. This must be called to
 * insure proper cleanup.
 */
void PField_Destroy(PassField *pf);

/* Set the string in the Password to 'string'
 * behavior depends on mode of widget
 */
void PField_SetString(PassField *pf, char *string);

/* Return the string stored in the Password widget */
char *PField_GetString(PassField *pf);

/* Set the behavior of the Password widget
 * mode 0 = do not * out characters
 * mode 1 = * out characters
 * mode change kills anything stored in the widget
 */
void PField_SetMode(PassField *pf, int mode);
