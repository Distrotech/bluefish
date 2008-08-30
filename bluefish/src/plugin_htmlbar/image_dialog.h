/* Bluefish HTML Editor
 * image_dialog.h
 *
 * Copyright (C) 2008 James Hayward and Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 

#ifndef __IMAGE_DIALOG_H_
#define __IMAGE_DIALOG_H_

#include "../bluefish.h"


#define BLUEFISH_TYPE_IMAGE_DIALOG            (bluefish_image_dialog_get_type ())
#define BLUEFISH_IMAGE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BLUEFISH_TYPE_IMAGE_DIALOG, BluefishImageDialog))
#define BLUEFISH_IMAGE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BLUEFISH_TYPE_IMAGE_DIALOG, BluefishImageDialogClass))
#define BLUEFISH_IS_IMAGE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BLUEFISH_TYPE_IMAGE_DIALOG))
#define BLUEFISH_IS_IMAGE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BLUEFISH_TYPE_IMAGE_DIALOG))
#define BLUEFISH_IMAGE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BLUEFISH_TYPE_IMAGE_DIALOG, BluefishImageDialogClass))


typedef struct _BluefishImageDialogPrivate BluefishImageDialogPrivate;

typedef struct _BluefishImageDialog BluefishImageDialog;
  
struct _BluefishImageDialog
{
  GtkDialog parent;
  
  BluefishImageDialogPrivate *priv;
};

typedef struct _BluefishImageDialogClass BluefishImageDialogClass;

struct _BluefishImageDialogClass
{
  GtkDialogClass parent_class;
};

GType bluefish_image_dialog_get_type (void);

void bluefish_image_dialog_new (Tbfwin *bfwin);

#endif /* __IMAGE_DIALOG_H_ */
