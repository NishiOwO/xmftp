/* store_options.h - Store and retrieve current settings */

/* Loads the option file into OptionState structure. If option file does
 * not exist, it is created with default options
 * returns 1 on success, 0 else
 */
int load_options(OptionState *options);

/* Saves the current settings in 'options' to the options file.
 * returns 1 on success, 0 else
 */
int save_options(OptionState *options);
