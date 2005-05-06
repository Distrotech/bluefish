/* Bluefish HTML Editor
 * authen2.c - register authentication callbacks if libgnomeui is not available
 *
 * Copyright (C) 2005 Olivier Sessink
 *
 * inspired by code in libgnomeui and the previous dialog written by Salvador Fandino
 * (which was buggy with async gnome_vfs calls)
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
#define DEBUG

#include "config.h"

#ifndef HAVE_ATLEAST_GNOMEUI_2_6

#include <gtk/gtk.h>
#include <string.h>
#include "bluefish.h"

/* #include "authen2.h" */
#include "gtk_easy.h"
typedef struct {
	GHashTable *hash;
} Tauthen;

typedef struct {
	gchar *username;
	gchar *password;
	gchar *domain;
} Tauthenval;

static void authen_value_free(Tauthenval *val) {
	g_free(val->username);
	g_free(val->password);
	g_free(val->domain);
	g_free(val);
}

static Tauthenval *authen_value_new(const gchar *username, const gchar *password, const gchar *domain) {
	Tauthenval *val = g_new0(Tauthenval, 1);
	val->username = g_strdup(username);
	if (password) val->password = g_strdup(password);
	if (domain) val->domain = g_strdup(domain);
	return val;
}

static gchar *authen_key_new(gchar *proto, gchar *server, gint port, gchar *object, gchar *authtype, gchar *username, gchar *domain) {
	gchar *key;
	if (port) {
		key = g_strdup_printf(":%s:%s:%d:%s:%s:%s:%s:", (proto ? proto : ""),(server ? server : ""), port, (object ? object : ""),(authtype?authtype:""),(username?username:""),(domain?domain:""));
	} else {
		key = g_strdup_printf(":%s:%s::%s:%s:%s:%s:", (proto ? proto : ""),(server ? server : ""), (object ? object : ""),(authtype?authtype:""),(username?username:""),(domain?domain:""));
	}
	DEBUG_MSG("authen_key_new, created key=%s\n",key);
	return key;
}

static void authen_insert(Tauthen *auth, gchar *password, gchar *proto, gchar *server, gint port, gchar *object, gchar *authtype, gchar *username, gchar *domain) {
	gchar *key;
	Tauthenval *val;
	val = authen_value_new(username, password, domain);
	key = authen_key_new(proto, server, port, object, authtype, username, domain);
	g_hash_table_insert(auth->hash, key, val);

	val = authen_value_new(username, password, domain);
	key = authen_key_new(proto, server, port, object, authtype, NULL, NULL);
	g_hash_table_insert(auth->hash, key, val);

	val = authen_value_new(username, password, domain);
	key = authen_key_new(NULL, server, port, NULL, NULL, username, domain);
	g_hash_table_insert(auth->hash, key, val);

	val = authen_value_new(username, password, domain);
	key = authen_key_new(NULL, server, 0, NULL, NULL, NULL, NULL);
	g_hash_table_insert(auth->hash, key, val);
}

static Tauthenval *authenval_lookup(Tauthen *auth, gchar *proto, gchar *server, gint port, gchar *object, gchar *authtype, gchar *username, gchar *domain) {
	gchar *key;
	Tauthenval *val;
	key = authen_key_new(proto, server, port, object, authtype, username, domain);
	val = g_hash_table_lookup(auth->hash, key);
	g_free(key);
	if (!val) {
		key = authen_key_new(proto, server, port, object, authtype, NULL, NULL);
		val = g_hash_table_lookup(auth->hash, key);
		g_free(key);
	}
	if (!val) {
		key = authen_key_new(proto, server, port, NULL, NULL, username, domain);
		val = g_hash_table_lookup(auth->hash, key);
		g_free(key);
	}
	if (!val) {
		key = authen_key_new(NULL, server, 0, NULL, NULL, username, domain);
		val = g_hash_table_lookup(auth->hash, key);
		g_free(key);
	}
	if (!val) {
		key = authen_key_new(NULL, server, 0, NULL, NULL, NULL, NULL);
		val = g_hash_table_lookup(auth->hash, key);
		g_free(key);
	}
	return val;
}

typedef struct {
	Tauthen *auth;
	GtkWidget *dialog;
	GtkWidget *username;
	GtkWidget *password;
	GtkWidget *domain;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
	gpointer in;
	gpointer out;
} Tauthdialog;

typedef struct {
	Tauthen *auth;
	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
	gpointer in;
	gpointer out;
} Tauthfill;

/*static void password_dialog_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tauthdialog *ad = data;
	if (response == GTK_RESPONSE_ACCEPT) {
		
	
	}
	ad->response(ad->response_data);
}*/

static Tauthdialog *password_dialog(Tauthen *auth, gchar *location, gchar *default_user, gchar *default_domain) {
	Tauthdialog *ad;
	gchar *tmp;

	ad = g_new0(Tauthdialog,1);
	ad->auth = auth;
	ad->dialog = gtk_dialog_new_with_buttons(_("Authentication required"), NULL,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(ad->dialog), GTK_RESPONSE_ACCEPT);
	gtk_window_set_position(GTK_WINDOW(ad->dialog), GTK_WIN_POS_CENTER);
	
	tmp = g_strconcat(_("You must log in to access "), location, NULL);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ad->dialog)->vbox), gtk_label_new(tmp), TRUE, TRUE, 5);
	g_free(tmp);
	if (default_user) {
		ad->username = boxed_entry_with_text(default_user, 20, GTK_DIALOG(ad->dialog)->vbox);
	}
	
	ad->password = boxed_entry_with_text("", 20, GTK_DIALOG(ad->dialog)->vbox);
	
	if (default_domain) {
		ad->domain = boxed_entry_with_text(default_domain, 20, GTK_DIALOG(ad->dialog)->vbox);
	}
	
	gtk_entry_set_visibility(GTK_ENTRY(ad->password), FALSE);

/*	g_signal_connect(dialog, "response", G_CALLBACK(password_dialog_response_lcb), bfwin);*/
	gtk_widget_show_all(ad->dialog);
	return ad;
}

static void vfs_async_authentication_lcb(gconstpointer in, size_t in_size, 
				   gpointer out, size_t out_size, 
				   gpointer user_data,
				   GnomeVFSModuleCallbackResponse response,
				   gpointer response_data) {
	DEBUG_MSG("vfs_async_authentication_lcb, called\n");
	response(response_data);
}
static void vfs_authentication_lcb(gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data) {
	DEBUG_MSG("vfs_authentication_lcb, called\n");
}

static void full_authentication_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tauthdialog *ad = data;
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real = ad->in;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real = ad->out;

	if (response == GTK_RESPONSE_ACCEPT) {
		out_real->password = gtk_editable_get_chars(GTK_EDITABLE(ad->password), 0, -1);
		if (ad->username) out_real->username = gtk_editable_get_chars(GTK_EDITABLE(ad->username), 0, -1);
		if (ad->domain) out_real->domain = gtk_editable_get_chars(GTK_EDITABLE(ad->domain), 0, -1);
		authen_insert(ad->auth, out_real->password, in_real->protocol, in_real->server, in_real->port, in_real->object, in_real->authtype, out_real->username, out_real->domain);
	}
	
	gtk_widget_destroy(ad->dialog);
	DEBUG_MSG("full_authentication_response_lcb, calling response(response_data)\n");
	ad->response(ad->response_data);
	
	g_free(ad);
}

static void vfs_async_full_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	Tauthdialog *ad;
	Tauthen *auth = user_data;
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real;

	DEBUG_MSG("vfs_async_full_authentication_lcb, called\n");

	in_real = (GnomeVFSModuleCallbackFullAuthenticationIn *)in;
	out_real = (GnomeVFSModuleCallbackFullAuthenticationOut *)out;
	ad = password_dialog(auth, in_real->uri, in_real->default_user ? in_real->default_user : in_real->username, in_real->domain);
	ad->in = in_real;
	ad->out = out_real;
	ad->response = response;
	ad->response_data = response_data;
	g_signal_connect(ad->dialog, "response", G_CALLBACK(full_authentication_response_lcb), ad);
}

static void vfs_full_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data) {
	DEBUG_MSG("vfs_full_authentication_lcb, called\n");
}

/*static void fill_authentication_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tauthdialog *ad = data;
	GnomeVFSModuleCallbackFillAuthenticationIn *in_real = ad->in;
	GnomeVFSModuleCallbackFillAuthenticationOut *out_real = ad->out;

	if (response == GTK_RESPONSE_ACCEPT) {
		gchar *key;
		out_real->password = gtk_editable_get_chars(GTK_EDITABLE(ad->password), 0, -1);
		if (in_real->username) out_real->username = g_strdup(in_real->username);
		if (in_real->domain) out_real->domain = g_strdup(in_real->domain);
		out_real->valid = TRUE;
		key = authen_key_new(NULL, in_real->server, in_real->port, in_real->object);
		g_hash_table_replace(ad->auth->hash, key, authen_value_new(out_real->username, out_real->password, out_real->domain));
	} else {
		out_real->valid = FALSE;
	}
	
	gtk_widget_destroy(ad->dialog);
	DEBUG_MSG("fill_authentication_response_lcb, valid=%d, calling response(response_data)\n",out_real->valid);
	ad->response(ad->response_data);
	
	g_free(ad);
}*/

static gboolean fill_authentication_idle_lcb(gpointer user_data) {
	Tauthfill *af = user_data;
	GnomeVFSModuleCallbackFillAuthenticationIn *in_real = (GnomeVFSModuleCallbackFillAuthenticationIn *)af->in;
	GnomeVFSModuleCallbackFillAuthenticationOut *out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)af->out;
	Tauthenval *val;

	val = authenval_lookup(af->auth, in_real->protocol, in_real->server, in_real->port, in_real->object, in_real->authtype, in_real->username, in_real->domain);
	if (val) {
		DEBUG_MSG("fill_authentication_idle_lcb, found val %p\n",val);
		out_real->password = g_strdup(val->password);
		if (in_real->username) out_real->username = g_strdup(val->username);
		if (in_real->domain) out_real->domain = g_strdup(val->domain);
		out_real->valid = TRUE;
	} else {
		out_real->valid = FALSE;
	}
	DEBUG_MSG("fill_authentication_idle_lcb, valid=%d, calling response()\n",out_real->valid);
	af->response(af->response_data);
	g_free(af);
	return FALSE;
}

static void vfs_async_fill_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	Tauthfill *af;
	
	af = g_new0(Tauthfill,1);
	af->auth = user_data;
	af->in = (gpointer)in;
	af->out = (gpointer)out;
	af->response = response;
	af->response_data = response_data;
	/* for some reason, I get a hanging bluefish if I call 'response' directly */
	g_idle_add_full(G_PRIORITY_HIGH, fill_authentication_idle_lcb, af, NULL);
	
/*	Tauthen *auth = user_data;
	gchar *key;
	Tauthdialog *ad;
	Tauthenval *val;
	GnomeVFSModuleCallbackFillAuthenticationIn *in_real = (GnomeVFSModuleCallbackFillAuthenticationIn *)in;
	GnomeVFSModuleCallbackFillAuthenticationOut *out_real = (GnomeVFSModuleCallbackFillAuthenticationOut *)out;

	DEBUG_MSG("vfs_async_fill_authentication_lcb, called, auth=%p\n", auth);
	key = authen_key_new("", in_real->server, in_real->port, in_real->object);
	DEBUG_MSG("vfs_async_fill_authentication_lcb, key=%s, will look in hash %p\n",key,auth->hash);
	val = (Tauthenval *)g_hash_table_lookup(auth->hash, key);
	g_free(key);
	if (val) {
		DEBUG_MSG("vfs_async_fill_authentication_lcb, found val %p\n",val);
		out_real->password = g_strdup(val->password);
		if (in_real->username) out_real->username = g_strdup(val->username);
		if (in_real->domain) out_real->domain = g_strdup(val->domain);
		out_real->valid = TRUE;
		DEBUG_MSG("vfs_async_fill_authentication_lcb, found the key, calling response()\n");
		response(response_data);
	} else {
		ad = password_dialog(auth, in_real->uri, NULL, NULL);
		ad->in = in_real;
		ad->out = out_real;
		ad->response = response;
		ad->response_data = response_data;
		g_signal_connect(ad->dialog, "response", G_CALLBACK(fill_authentication_response_lcb), ad);
	}*/
}

static void vfs_fill_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data) {
	DEBUG_MSG("vfs_fill_authentication_lcb, called\n");
}
static void vfs_async_save_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	DEBUG_MSG("vfs_async_save_authentication_lcb, called\n");
	response(response_data);
}

static void vfs_save_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data) {
	DEBUG_MSG("vfs_save_authentication_lcb, called\n");
}

static void vfs_async_question_lcb(gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data,
			     GnomeVFSModuleCallbackResponse response,
			     gpointer response_data) {
	DEBUG_MSG("vfs_async_question_lcb, called\n");
	response(response_data);
}

static void vfs_question_lcb(gconstpointer in, size_t in_size, 
		       gpointer out, size_t out_size, 
		       gpointer user_data) {
	DEBUG_MSG("vfs_question_lcb, called\n");
}

void authen_init(void) {
	Tauthen * auth;

	auth = g_new(Tauthen,1);
	auth->hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify) authen_value_free);
	DEBUG_MSG("authen_init, auth=%p, hash=%p\n",auth,auth->hash);

	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
				vfs_async_authentication_lcb, 
				auth,NULL);
	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
				vfs_async_authentication_lcb, 
				auth,NULL);
	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
				vfs_async_fill_authentication_lcb, 
				auth,NULL);
	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
				vfs_async_full_authentication_lcb, 
				auth,NULL);
	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
				vfs_async_save_authentication_lcb, 
				auth,NULL);
	gnome_vfs_async_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_QUESTION,
				vfs_async_question_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_AUTHENTICATION,
				vfs_authentication_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_HTTP_PROXY_AUTHENTICATION, 
				vfs_authentication_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
				vfs_fill_authentication_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
				vfs_full_authentication_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
				vfs_save_authentication_lcb, 
				auth,NULL);
	gnome_vfs_module_callback_set_default(GNOME_VFS_MODULE_CALLBACK_QUESTION,
				vfs_question_lcb, 
				auth,NULL);
}
#endif /* ifndef HAVE_ATLEAST_GNOMEUI_2_6 */
