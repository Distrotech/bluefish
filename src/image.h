#ifndef __IMAGE_H_
#define __IMAGE_H_

#include "html_diag.h"

void image_insert_dialog(Tbfwin *bfwin, Ttagpopup *data);
void thumbnail_insert_dialog(Tbfwin *bfwin, Ttagpopup *data);
void image_insert_from_filename(gchar *filename);
void multi_thumbnail_dialog(Tbfwin *bfwin, Ttagpopup *data);
#endif /* __IMAGE_H_ */
