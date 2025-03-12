/* URL processing */

/* Parses the string 'url' into its various components
 * On success:
 *    1 is returned
 *    login = malloc'ed string referring to login name or NULL if none provided
 *    password = malloc'ed string referring to password or NULL if none
 *               provided
 *    site = malloc'ed string referring to the site name
 *    port = port or -1 if none specified (must be allocated int)
 *    directory = directory to CD to or NULL if not provided
 *    file = file to retrieve or NULL if not provided
 */

int process_url(char *url, char **login, char **password, char **site, 
		int *port, char **directory, char **file);

