
#ifndef __BOOKMARK_H__
#define __BOOKMARK_H__



GtkWidget *bmark_gui(Tbfwin *bfwin); /* used in gui.c */
void bmark_init(void); /* only used once */
void bmark_cleanup(Tbfwin *bfwin);

GHashTable *bmark_get_lines(Tdocument *doc,gboolean temp);
void bmark_clean_for_doc(Tdocument *doc); /* set bookmark's doc to NULL when closing file */ 
void bmark_set_for_doc(Tdocument *doc); /* set bookmark's doc to proper doc when opening file */ 

void bmark_add_temp(Tbfwin *bfwin);
void bmark_del_all_temp(Tbfwin *bfwin);
void bmark_add_perm(Tbfwin *bfwin);
void bmark_del_all_perm(Tbfwin *bfwin);

void bmark_save_all();
void bmark_check_length(Tbfwin *bfwin,Tdocument *doc);
void bmark_adjust_visible(Tbfwin *win);


#endif /* __BOOKMARK_H__ */
