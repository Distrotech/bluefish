/* Bluefish HTML Editor
 * encodings_dialog.h
 *
 * Copyright (C) 2008 James Hayward and Olivier Sessink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ENCODINGS_DIALOG_H_
#define	__ENCODINGS_DIALOG_H_

#include "bluefish.h"


#define BLUEFISH_TYPE_ENCODINGS_DIALOG            (bluefish_encodings_dialog_get_type ())
#define BLUEFISH_ENCODINGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), BLUEFISH_TYPE_ENCODINGS_DIALOG, BluefishEncodingsDialog))
#define BLUEFISH_ENCODINGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), BLUEFISH_TYPE_ENCODINGS_DIALOG, BluefishEncodingsDialogClass))
#define BLUEFISH_IS_ENCODINGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), BLUEFISH_TYPE_ENCODINGS_DIALOG))
#define BLUEFISH_IS_ENCODINGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), BLUEFISH_TYPE_ENCODINGS_DIALOG))
#define BLUEFISH_ENCODINGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BLUEFISH_TYPE_ENCODINGS_DIALOG, BluefishEncodingsDialogClass))


typedef struct _BluefishEncodingsDialogPrivate BluefishEncodingsDialogPrivate;

typedef struct _BluefishEncodingsDialog BluefishEncodingsDialog;
  
struct _BluefishEncodingsDialog
{
  GtkDialog parent;
  
  BluefishEncodingsDialogPrivate *priv;
};

typedef struct _BluefishEncodingsDialogClass BluefishEncodingsDialogClass;

struct _BluefishEncodingsDialogClass
{
  GtkDialogClass parent_class;
};

GType bluefish_Encodings_dialog_get_type (void);


void bluefish_encodings_dialog_new (Tbfwin *bfwin);


#endif	/* __ENCODINGS_DIALOG_H_ */
