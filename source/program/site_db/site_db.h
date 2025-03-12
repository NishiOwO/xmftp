/* site_db.h - Site Manager database functions
 *
 * These functions work on an internal linked list invisible to the caller
 */

/***************************************************************************/

#include "../../common/common.h"

/***************************************************************************/

/* Add the site pointed to by 'site' to the current sitelist
 * returns 1 on success, 0 on failure
 */
int SiteInfoList_add_site(SiteInfo *site);

/* Deletes the site with nickname matching 'nickname', or, if NULL,
 * matching full_site. If 'nickname' is non-NULL, full_site is ignored.
 * returns 1 on success, 0 on failure
 */
int SiteInfoList_delete_site(char *nickname, char *full_site);

/* Save the sitelist to file filename
 * returns 1 on success, 0 on failure
 */
int SiteInfoList_save_sites(char *filename);

/* Destroy the current list freeing the memory */
void SiteInfoList_destroy_list(void);

/* Load a list from file filename
 * returns 1 on success, 0 on failure
 */
int SiteInfoList_read_list(char *filename);

/* Traverse list, reading one record at a time.
 * The caller should call this function first with reset being 1
 * (store_site being unused). This will initialize the traversal and
 * return 1.
 * Upon the next call(s), reset should be 0 and store_site should point
 * to a structure where the current record (siteinfo) is stored.
 * Each consecutive call will load consecutive records and return 1
 * when a record is successfully read. 0 is returned when there are no more
 * records to read. Consequently the next time you call this function, you
 * will have to reset.
 */
int SiteInfoList_traverse_list(SiteInfo *store_site, int reset);

/* Find the site with nickname matching 'nickname' and store it's information
 * in store_site.
 * returns 1 on success, 0 on failure.
 */
int SiteInfoList_find_site(char *nickname, SiteInfo *store_site);

int crypt_site(SiteInfo *original, SiteInfo *new);
/***************************************************************************/
