/* Bluefish HTML Editor
 * authen2.c - register authentication callbacks if libgnomeui is not available
 *
 * Copyright (C) 2005 Olivier Sessink
 *
 * inspired by code in libgnomeui 
 * and the previous dialog written by Salvador Fandino
 * (which was buggy with async gnome_vfs calls)
 *
 * there is hardly any documentation on this, you should read 
 * libgnomevfs/gnome-vfs-standard-callbacks.h 
 * libgnomeui/gnome-authentication-manager.c http://cvs.gnome.org/viewcvs/libgnomeui/libgnomeui/gnome-authentication-manager.c
 * libgnomeui/gnome-password-dialog.c http://cvs.gnome.org/viewcvs/libgnomeui/libgnomeui/gnome-password-dialog.c
 * gnome-keyring/gnome-keyring.c http://cvs.gnome.org/viewcvs/gnome-keyring/gnome-keyring.c
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

static void add_row(GtkWidget *table, int row, const char *label_text, GtkWidget *entry) {
	GtkWidget *label;

	label = gtk_label_new_with_mnemonic(label_text);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
			0, 1,row, row + 1,
			GTK_FILL,(GTK_FILL|GTK_EXPAND),0, 0);
	gtk_table_attach (GTK_TABLE (table), entry,
			1, 2,row, row + 1,
			(GTK_FILL|GTK_EXPAND),(GTK_FILL|GTK_EXPAND),0, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
}

static Tauthdialog *password_dialog(Tauthen *auth, gboolean proxy_authentication, gchar *location, gchar *default_user, gchar *default_domain) {
	Tauthdialog *ad;
	gchar *tmp;
	GtkWidget *table, *label;
	gint row;
	DEBUG_MSG("password_dialog, started for location %s\n",location);
	ad = g_new0(Tauthdialog,1);
	ad->auth = auth;
	/* keep the messages for consistency the same as in libgnomeui */
	ad->dialog = gtk_dialog_new_with_buttons(_("Authentication required"), NULL,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(ad->dialog), GTK_RESPONSE_ACCEPT);
	if (proxy_authentication) {
		GtkWidget *label = gtk_label_new(_("Your HTTP Proxy requires you to log in."));
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ad->dialog)->vbox), label, TRUE,TRUE,5);
	}
	gtk_dialog_set_has_separator (GTK_DIALOG(ad->dialog), FALSE);
	gtk_window_set_position(GTK_WINDOW(ad->dialog), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(ad->dialog), 10);
	
	tmp = g_strconcat(_("You must log in to access "), location, NULL);
	label = gtk_label_new(tmp);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(ad->dialog)->vbox), label, TRUE, TRUE, 5);
	g_free(tmp);
	
	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE (table), 12);
	gtk_table_set_row_spacings(GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(ad->dialog)->vbox), table, TRUE, TRUE, 5);
	row = 0;
	if (default_user) {
		ad->username = entry_with_text(default_user, 20);
		add_row(table, row++, _("_Username:"), ad->username);
	}
	
	if (default_domain) {
		ad->domain = entry_with_text(default_domain, 20);
		add_row(table, row++, _("_Domain:"), ad->domain);
	}
	ad->password = entry_with_text("", 20);
	add_row(table, row++, _("_Pasword:"), ad->password);
	gtk_entry_set_visibility(GTK_ENTRY(ad->password), FALSE);

	gtk_widget_show_all(ad->dialog);
	return ad;
}

static void authentication_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tauthdialog *ad = data;
	GnomeVFSModuleCallbackAuthenticationOut *out_real = ad->out;

	if (response == GTK_RESPONSE_ACCEPT) {
		out_real->password = gtk_editable_get_chars(GTK_EDITABLE(ad->password), 0, -1);
		if (ad->username) out_real->username = gtk_editable_get_chars(GTK_EDITABLE(ad->username), 0, -1);
	}
	
	gtk_widget_destroy(ad->dialog);
	if (ad->response) {
		DEBUG_MSG("authentication_response_lcb, calling response(response_data)\n");
		ad->response(ad->response_data);
	}
	g_free(ad);
}

static void vfs_async_authentication_lcb(gconstpointer in, size_t in_size, 
				   gpointer out, size_t out_size, 
				   gpointer user_data,
				   GnomeVFSModuleCallbackResponse response,
				   gpointer response_data) {
	Tauthdialog *ad;
	Tauthen *auth = user_data;
	GnomeVFSModuleCallbackAuthenticationIn *in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	GnomeVFSModuleCallbackAuthenticationOut *out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;

	DEBUG_MSG("vfs_async_authentication_lcb, called\n");
	ad = password_dialog(auth, TRUE, in_real->uri, 
			"", NULL);
	ad->in = in_real;
	ad->out = out_real;
	ad->response = response;
	ad->response_data = response_data;
	g_signal_connect(ad->dialog, "response", G_CALLBACK(authentication_response_lcb), ad);

}
static void vfs_authentication_lcb(gconstpointer in, size_t in_size, 
			     gpointer out, size_t out_size, 
			     gpointer user_data) {
	/* seems to be for http-proxy authentication */
	Tauthdialog *ad;
	Tauthen *auth = user_data;
	GnomeVFSModuleCallbackAuthenticationIn *in_real = (GnomeVFSModuleCallbackAuthenticationIn *)in;
	GnomeVFSModuleCallbackAuthenticationOut *out_real = (GnomeVFSModuleCallbackAuthenticationOut *)out;
	gint ret;
	GDK_THREADS_ENTER ();
	DEBUG_MSG("vfs_authentication_lcb, called\n");
	ad = password_dialog(auth, TRUE, in_real->uri, 
			"", NULL);
	ad->in = in_real;
	ad->out = out_real;
	ad->response = NULL;
	ad->response_data = NULL;
	ret = gtk_dialog_run(GTK_DIALOG(ad->dialog));
	authentication_response_lcb(GTK_DIALOG(ad->dialog),ret,ad);
	GDK_THREADS_LEAVE ();
}

static void full_authentication_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tauthdialog *ad = data;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real = ad->out;

	if (response == GTK_RESPONSE_ACCEPT) {
		out_real->abort_auth = FALSE;
		out_real->save_password = TRUE;
		out_real->password = gtk_editable_get_chars(GTK_EDITABLE(ad->password), 0, -1);
		if (ad->username) out_real->username = gtk_editable_get_chars(GTK_EDITABLE(ad->username), 0, -1);
		if (ad->domain) out_real->domain = gtk_editable_get_chars(GTK_EDITABLE(ad->domain), 0, -1);
	} else {
		out_real->abort_auth = TRUE;
	}
	
	gtk_widget_destroy(ad->dialog);
	if (ad->response) {
		DEBUG_MSG("full_authentication_response_lcb, calling response(response_data), abort=%d\n",out_real->abort_auth);
		ad->response(ad->response_data);
	}
	g_free(ad);
}

static void vfs_async_full_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	Tauthdialog *ad;
	Tauthen *auth = user_data;
	gchar *username=NULL;
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real = (GnomeVFSModuleCallbackFullAuthenticationIn *)in;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real = (GnomeVFSModuleCallbackFullAuthenticationOut *)out;

	DEBUG_MSG("vfs_async_full_authentication_lcb, called, flags=%d\n",in_real->flags);
	if (in_real->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME) {
		if (in_real->default_user) username = in_real->default_user;
		else if (in_real->username) username = in_real->username;
		else username = "";
	}
	ad = password_dialog(auth, FALSE, in_real->uri, 
			username,
			(in_real->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN) ? (in_real->domain ? in_real->domain : "" ): NULL);
	ad->in = in_real;
	ad->out = out_real;
	ad->response = response;
	ad->response_data = response_data;
	g_signal_connect(ad->dialog, "response", G_CALLBACK(full_authentication_response_lcb), ad);
}

static void vfs_full_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, gpointer user_data) {
	Tauthdialog *ad;
	Tauthen *auth = user_data;
	GnomeVFSModuleCallbackFullAuthenticationIn *in_real = (GnomeVFSModuleCallbackFullAuthenticationIn *)in;
	GnomeVFSModuleCallbackFullAuthenticationOut *out_real = (GnomeVFSModuleCallbackFullAuthenticationOut *)out;
	gint ret;

	DEBUG_MSG("vfs_full_authentication_lcb, called\n");
	ad = password_dialog(auth, FALSE, in_real->uri, 
			(in_real->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME) ? (in_real->default_user ? in_real->default_user : in_real->username) : NULL, 
			(in_real->flags & GNOME_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_DOMAIN) ? (in_real->domain ? in_real->domain : "" ): NULL);
	ad->in = in_real;
	ad->out = out_real;
	ad->response = NULL;
	ad->response_data = NULL;
	ret = gtk_dialog_run(GTK_DIALOG(ad->dialog));
	full_authentication_response_lcb(GTK_DIALOG(ad->dialog),ret,ad);
}

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
	if (af->response) {
		DEBUG_MSG("fill_authentication_idle_lcb, valid=%d, calling response()\n",out_real->valid);
		af->response(af->response_data);
	}
	g_free(af);
	return FALSE;
}

static void vfs_async_fill_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	Tauthfill *af;
	DEBUG_MSG("vfs_async_fill_authentication_lcb, started\n");
	af = g_new0(Tauthfill,1);
	af->auth = user_data;
	af->in = (gpointer)in;
	af->out = (gpointer)out;
	af->response = response;
	af->response_data = response_data;
	/* for some reason, I get a hanging bluefish if I call 'response' directly */
	g_idle_add_full(G_PRIORITY_HIGH, fill_authentication_idle_lcb, af, NULL);
}

static void vfs_fill_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data) {
	DEBUG_MSG("vfs_fill_authentication_lcb, started\n");
	Tauthfill *af;
	af = g_new0(Tauthfill,1);
	af->auth = user_data;
	af->in = (gpointer)in;
	af->out = (gpointer)out;
	af->response = NULL;
	af->response_data = NULL;
	fill_authentication_idle_lcb(af);
}

static gboolean save_authentication_idle_lcb(gpointer user_data) {
	Tauthfill *af = user_data;
	GnomeVFSModuleCallbackSaveAuthenticationIn *in_real = (GnomeVFSModuleCallbackSaveAuthenticationIn *)af->in;
	
	authen_insert(af->auth, in_real->password, in_real->protocol, in_real->server, in_real->port, in_real->object, in_real->authtype, in_real->username, in_real->domain);
	if (af->response) {
		DEBUG_MSG("save_authentication_idle_lcb, calling response()\n");
		/* I can't find in libgnomeui nor in gnome-keyring.c if they call the response anywhere.. */
		af->response(af->response_data);
	}
	g_free(af);
	return FALSE;
}

static void vfs_async_save_authentication_lcb(gconstpointer in, size_t in_size, 
					gpointer out, size_t out_size, 
					gpointer user_data,
					GnomeVFSModuleCallbackResponse response,
					gpointer response_data) {
	Tauthfill *af;
	DEBUG_MSG("vfs_async_save_authentication_lcb, called\n");
	af = g_new0(Tauthfill,1);
	af->auth = user_data;
	af->in = (gpointer)in;
	af->out = (gpointer)out;
	af->response = response;
	af->response_data = response_data;
	/* for some reason, I get a hanging bluefish if I call 'response' directly */
	g_idle_add_full(G_PRIORITY_HIGH, save_authentication_idle_lcb, af, NULL);
}

static void vfs_save_authentication_lcb(gconstpointer in, size_t in_size, 
				  gpointer out, size_t out_size, 
				  gpointer user_data) {
	Tauthfill *af;
	DEBUG_MSG("vfs_save_authentication_lcb, called\n");
	af = g_new0(Tauthfill,1);
	af->auth = user_data;
	af->in = (gpointer)in;
	af->out = (gpointer)out;
	af->response = NULL;
	af->response_data = NULL;
	save_authentication_idle_lcb(af);
}

typedef struct {
	Tauthen *auth;
	GtkWidget *dialog;

	GnomeVFSModuleCallbackResponse response;
	gpointer response_data;
	gpointer in;
	gpointer out;
} Tquesdialog;

static Tquesdialog *create_question_dialog (char *prim_msg, char *sec_msg, char **choices)  {
	Tquesdialog *qd;
	int cnt, len;

	qd = g_new0(Tquesdialog,1);
	qd->dialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "<span weight=\"bold\" size=\"larger\">%s</span>\n\n%s",prim_msg, sec_msg);
	
	if (choices) {
		/* First count the items in the list then 
		 * add the buttons in reverse order */
		for (len=0; choices[len] != NULL; len++); 
		for (cnt=len-1; cnt >= 0; cnt--) {
			/* Maybe we should define some gnome-vfs stockbuttons and 
			 * then replace them here with gtk-stockbuttons */
			gtk_dialog_add_button (GTK_DIALOG(qd->dialog), choices[cnt], cnt);
		}
	}
	return qd;
}

static void question_response_lcb(GtkDialog *dialog,gint response,gpointer data) {
	Tquesdialog *qd = data;
	GnomeVFSModuleCallbackQuestionOut *out_real = qd->out;
	out_real->answer = response;
	gtk_widget_destroy(qd->dialog);
	if (qd->response) {
		qd->response(qd->response_data);
	}
	g_free(qd);
}

static void vfs_async_question_lcb(gconstpointer in, size_t in_size, 
				gpointer out, size_t out_size, 
				gpointer user_data,
				GnomeVFSModuleCallbackResponse response,
				gpointer response_data) {
	GnomeVFSModuleCallbackQuestionIn *in_real = (GnomeVFSModuleCallbackQuestionIn *)in;
	GnomeVFSModuleCallbackQuestionOut *out_real = (GnomeVFSModuleCallbackQuestionOut *)out;
	Tquesdialog *qd = create_question_dialog(in_real->primary_message, in_real->secondary_message, in_real->choices);
	qd->response = response;
	qd->response_data = response_data;
	qd->out = out_real;
	out_real->answer = -1; /* Set a default value */
	g_signal_connect(qd->dialog, "response", G_CALLBACK(question_response_lcb), qd);
}

static void vfs_question_lcb(gconstpointer in, size_t in_size, 
				gpointer out, size_t out_size, 
				gpointer user_data) {
	GnomeVFSModuleCallbackQuestionIn *in_real = (GnomeVFSModuleCallbackQuestionIn *)in;
	GnomeVFSModuleCallbackQuestionOut *out_real = (GnomeVFSModuleCallbackQuestionOut *)out;
	
	Tquesdialog *qd;
	qd = create_question_dialog(in_real->primary_message, in_real->secondary_message, in_real->choices);
	qd->out = out_real;
	out_real->answer = gtk_dialog_run(GTK_DIALOG(qd->dialog));
	question_response_lcb(GTK_DIALOG(qd->dialog),out_real->answer,qd);
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
