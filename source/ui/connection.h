/* connection.h - Operations specific to FTP connection */

/****************************************************************************/

/* Command line processing (URL)
 * makes actual connection and transfer if needed
 */
void command_line_url(Main_CB *main_cb, char *url);

/* Quick Connect dialog - Allows user to quickly connect to a remote host */
void quick_connect(Widget w, XtPointer client_data, XtPointer call_data);

/* Reconnects to the host (used if connection becomes corrupt) */
void reconnect(Widget w, XtPointer client_data, XtPointer call_data);

/* Disconnect from the host */
void disconnect(Widget w, XtPointer client_data, XtPointer call_data);

/* Disconnects from the remote host and perform cleanup
 * this is called from disconnect()
 */
void do_kill_connection(Main_CB *main_cb);

/* Attempt login with login information.
 * returns 1 on success, 0 on failure
 * check get_ftp_last_resp() for reason of failure
 * if directory is non-null, an attempt is made to change to that dir
 * if port is -1, use default port
 * all other arguments must be non-null
 */
int do_login(Main_CB *main_cb, char *hostname, char *login, char *password, 
	     char *directory, int port);

/****************************************************************************/

#define IsConnected(main_cb)  \
          ((main_cb->current_state.connection_state.connected))

#define IsConnectedFromSiteMgr(main_cb)  \
          ((main_cb->current_state.connection_state.connect_from_site_mgr))



