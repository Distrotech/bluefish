
#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__

#ifdef BOOKMARKS

/* I'am preparing code structure similar to current fref made by Olivier 
   O.S. */

GtkWidget *bmark_gui(Tbfwin *bfwin); /* used in gui.c */
void bmark_init(void); /* only used once */
void bmark_cleanup(Tbfwin *bfwin);

GHashTable *bmark_get_temp_lines(Tdocument *doc);
void bmark_clean_for_doc(Tdocument *doc); /* set bookmark's doc to NULL when closing file */ 

void bmark_add_temp(Tbfwin *bfwin);
void bmark_del_all_temp(Tbfwin *bfwin);

void bmark_add_perm(Tbfwin *bfwin);


#endif /* BOOKMARKS */

#endif /* __BOOKMARK_H__ */
