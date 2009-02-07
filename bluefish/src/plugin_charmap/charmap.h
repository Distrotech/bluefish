#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      /* getopt() */

#include "../config.h"
#include "../plugins.h"
#include "../bluefish.h" /* BLUEFISH_SPLASH_FILENAME */

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *chaptersv;
	GtkWidget *gcm;
}Tcharmap;
