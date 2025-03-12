typedef enum {
  CONNECT, DISCONNECT, DOWNLOAD, LOGO, QUIT, RECONNECT, SITE_MGR, UPLOAD,
  VIEW } Icon;

Pixmap load_xmftp_pixmap(Main_CB *main_cb, Icon which, Widget parent);

Pixmap gimme_pix(Main_CB *main_cb, char *filename, Widget toolbar);
Pixmap gimme_pix_from_data(Main_CB *main_cb, char **data, Widget toolbar);
/* Error message loading pixmap */
void error_pix(void);

