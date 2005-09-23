/* Bluefish HTML Editor
 * authen.c - handle authentication requests for gnome-vfs
 *
 * Copyright (C) 2004 Salvador Fandino
 * changes/bugfixes (C) 2004 Olivier Sessink
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

/*#define DEBUG*/

#include <gtk/gtk.h>
#include <string.h>
#include "bluefish.h"

#include "authen.h"
#include "gtk_easy.h"

#ifdef HAVE_ATLEAST_GNOMEVFS_2_6
#ifndef HAVE_ATLEAST_GNOMEUI_2_6

#include <libgnomevfs/gnome-vfs-standard-callbacks.h>

typedef struct {
	GHashTable *hash;
} AuthenData;

typedef struct {
	gchar *user;
	gchar *passwd;
} AuthenValue;

static void authen_value_free(AuthenValue * val)
{
	g_free(val->user);
	g_free(val->passwd);
	g_free(val);
}

static AuthenValue *authen_value_new(gchar * user, gchar * passwd)
{
	AuthenValue *val = g_new0(AuthenValue, 1);
	val->user = g_strdup(user);
	val->passwd = g_strdup(passwd);
	return val;
}

/**
 * authen_ask_user
 *
 * display authentication request
 *
 * returns true if the user accepted to authenticate
 */

static gint authen_ask_user(gchar const *uri, gchar const *object, gchar const *user,
							gchar ** newuser, gchar ** newpasswd)
{
	GtkWidget *dialog, *box, *label;
	GtkWidget *textuser, *textpasswd;
	gint ret;
	gchar *labelstr;
	dialog =
		gtk_dialog_new_with_buttons(_("Authentication required"), NULL,
									GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
									GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
									GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	box = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), box);
	gtk_container_set_border_width(GTK_CONTAINER(box), 12);

	labelstr = g_strdup_printf(_("Authentication is required for %s."), uri);
	label = gtk_label_new(labelstr);
	g_free(labelstr);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_container_add(GTK_CONTAINER(box), label);

	label = gtk_label_new(_("Username:"));
	gtk_container_add(GTK_CONTAINER(box), label);

	textuser = boxed_entry_with_text(user, 80, box);

	label = gtk_label_new(_("Password:"));
	gtk_container_add(GTK_CONTAINER(box), label);

	textpasswd = boxed_entry_with_text("", 80, box);
	gtk_entry_set_visibility(GTK_ENTRY(textpasswd), FALSE);

	gtk_widget_show_all(dialog);
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	
	DEBUG_MSG("authen_ask_user: dialog return ret = %d ok = %d\n", ret, (ret == GTK_RESPONSE_ACCEPT));
	if (ret == GTK_RESPONSE_ACCEPT) {
		*newuser = gtk_editable_get_chars(GTK_EDITABLE(textuser), 0, -1);
		*newpasswd = gtk_editable_get_chars(GTK_EDITABLE(textpasswd), 0, -1);
		DEBUG_MSG("authen_ask_user: newuser=%s\n",*newuser);
		gtk_widget_destroy(dialog);
		return TRUE;
	}
	gtk_widget_destroy(dialog);
	return FALSE;
}

static gchar *make_key(gchar * proto, gchar * server, int port, gchar * object)
{
	gchar *str = g_strdup_printf("proto=%s server=%s port=%d object=%s", (proto ? proto : ""),
					(server ? server : ""), port, (object ? object : ""));
	DEBUG_MSG("authen_callback: key = %s\n", str);
	return str;
}

static void fill_authen_callback(GnomeVFSModuleCallbackFillAuthenticationIn * in, gsize in_size,
								 GnomeVFSModuleCallbackFillAuthenticationOut * out, gsize out_size,
								 AuthenData * data)
{
	gchar *key;
	AuthenValue *val;

	DEBUG_MSG("fill_authen_callback: uri = %s\n", in->uri);

	key = make_key(in->protocol, in->server, in->port, in->object);
	if ((val = (AuthenValue *) (g_hash_table_lookup(data->hash, key))) != NULL) {
		DEBUG_MSG("fill_authen_callback: key found on authen cache\n");
		out->valid = TRUE;
		out->username = g_strdup(val->user);
		out->password = g_strdup(val->passwd);
		DEBUG_MSG("fill_authen_callback: user=%s len(passwd)=%d\n", out->username, strlen(out->password));
	}
	g_free(key);
}

static void full_authen_callback(GnomeVFSModuleCallbackFullAuthenticationIn * in, gsize in_size,
								 GnomeVFSModuleCallbackFullAuthenticationOut * out, gsize out_size,
								 AuthenData * data)
{

/*	gchar *key; */

	DEBUG_MSG("full_authen_callback: uri = %s flags = 0x%x\n", in->uri, in->flags);

/*	key = make_key(in->protocol, in->server, in->port, in->object);
	g_hash_table_remove(data->hash, key);
	g_free(key);*/

	if (authen_ask_user(in->uri, in->object, in->default_user, &(out->username), &(out->password))) {
/*		g_hash_table_replace(data->hash, key, authen_value_new(out->username, out->password));*/
		out->domain = in->default_domain;
		out->save_password = FALSE;
		out->abort_auth = FALSE;
		out->keyring = NULL;
	} else
		out->abort_auth = TRUE;
	DEBUG_MSG("full_authen_callback: user = %s len(passwd)=%d\n", out->username, strlen(out->password));
}

static void save_authen_callback(GnomeVFSModuleCallbackSaveAuthenticationIn * in, gsize in_size,
								 GnomeVFSModuleCallbackSaveAuthenticationOut * out, gsize out_size,
								 AuthenData * data)
{
	gchar *key;
	AuthenValue *val;

	DEBUG_MSG("save_authen_callback: uri = %s keyring = %s\n", in->uri, in->keyring);

	key = make_key(in->protocol, in->server, in->port, in->object);
	val = authen_value_new(in->username, in->password);
	g_hash_table_replace(data->hash, key, val);
}

/*
 * set_authen_callbacks:
 * 
 * register gnome-vfs callbacks for user authentication handling
 */

void set_authen_callbacks(void)
{

	AuthenData *data = g_new0(AuthenData,1);
	data->hash =
		g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) authen_value_free);
/*	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
										  (GnomeVFSModuleCallback) fill_authen_callback, data,
										  NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
										  (GnomeVFSModuleCallback) full_authen_callback, data,
										  NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
										  (GnomeVFSModuleCallback) save_authen_callback, data,
										  NULL);*/
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
					(GnomeVFSModuleCallback) fill_authen_callback, 
					data,
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
					(GnomeVFSModuleCallback) full_authen_callback, 
					data,
					NULL);
	gnome_vfs_module_callback_push (GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					(GnomeVFSModuleCallback) save_authen_callback, 
					data,
					NULL);
	DEBUG_MSG("set_authen_callbacks: callbacks registered\n");

}

#endif /*  */
#endif							/* HAVE_ATLEAST_GNOMEVFS_2_6 */
