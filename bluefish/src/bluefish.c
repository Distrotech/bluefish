/* Bluefish HTML Editor
 * bluefish.c - the main function
 *
 * Copyright (C) 1998 Olivier Sessink and Chris Mazuc
 * Copyright (C) 1999-2002 Olivier Sessink
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <gtk/gtk.h>

#include "bluefish.h"
#include "gui.h"
/*********************************************/
/* this var is global for all bluefish files */
/*********************************************/
Tmain *main_v;



int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	
	main_v = g_new0(Tmain, 1);
	
	{
		GList *filenames=NULL;
		
		/* get filesnames from commandline and from message queue */
		
		gui_create_main(filenames);
	}

	gtk_main();
	return 0;
}

void bluefish_exit_request() {

	/* check for changed documents here */

	gtk_main_quit();
}
