#ifndef __IMAGE_H_
#define __IMAGE_H_

#include "html_diag.h"

void image_insert_dialog(Tbfwin *bfwin, Ttagpopup *data);
void thumbnail_insert_dialog(Tbfwin *bfwin);
void image_insert_from_filename(Tbfwin *bfwin, gchar *filename);
void multi_thumbnail_dialog(Tbfwin *bfwin);
#endif /* __IMAGE_H_ */
