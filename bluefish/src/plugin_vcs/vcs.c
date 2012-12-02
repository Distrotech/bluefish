
#include "../bluefish.h"
#include <gmodule.h>
#include "../plugins.h"
#include "../rcfile.h"
#include "vcs.h"
#include "vcs_gui.h"
#include "svn.h"

Tvcs vcs_v;

void
vcs_commit_real(Tbfwin *bfwin, Tvcssession *vs, GList *files, const gchar *message)
{
	svn_commit_real(bfwin, vs, files, message);
}

void
vcs_autoconfigure(Tvcssession *vs, GFile *cururi)
{
	svn_autoconfigure(vs, cururi);

}

Tvcssession *
vcsession_get(Tsessionvars * session)
{
	Tvcssession *vs = g_hash_table_lookup(vcs_v.lookup, session);
	if (!vs) {
		vs = g_slice_new0(Tvcssession);
		g_hash_table_insert(vcs_v.lookup, session, vs);
	}
	return vs;
}

void
vcs_status(Tbfwin *bfwin, Tvcssession *vs, gpointer data)
{
	if (vs->basedir == NULL || vs->basedir[0] == '\0') {
		vcs_autoconfigure(vs, bfwin->current_document->uri);
	}
	/*if (g_strcmp0(vs->type, "svn")==0) {*/
		svn_status(bfwin, vs->basedir, data);
	/*}*/
}

static void
vcs_update(GtkAction *action, gpointer user_data)
{
	Tbfwin *bfwin = user_data;
	Tvcssession *vs;
	vs = vcsession_get(bfwin->session);
	if (vs->basedir == NULL || vs->basedir[0] == '\0') {
		vcs_autoconfigure(vs, bfwin->current_document->uri);
	}
	bfwin->session->outputb_show_all_output = TRUE;
	svn_update(bfwin, vs->basedir, vs);
}

static void
vcs_commit(GtkAction *action, gpointer user_data)
{
	Tbfwin *bfwin = user_data;
	Tvcssession *vs;

	vs = vcsession_get(bfwin->session);
	vcs_commit_dialog(bfwin, vs);
	bfwin->session->outputb_show_all_output = TRUE;
	g_print("vcs_commit\n");
}

static void
vcs_session_free(gpointer data)
{
	g_slice_free(Tvcssession, data);
}

static void
vcs_init(void)
{
#ifdef ENABLE_NLS
	DEBUG_MSG("vcs_init, gettext domain-name=%s\n", PACKAGE "_plugin_vcs");
	bindtextdomain(PACKAGE "_plugin_vcs", LOCALEDIR);
	bind_textdomain_codeset(PACKAGE "_plugin_vcs", "UTF-8");
#endif							/* ENABLE_NLS */
	vcs_v.lookup = g_hash_table_new_full(NULL /* == g_direct_hash() */ ,
											  NULL /* == g_direct_equal() */ ,
											  NULL, vcs_session_free);
}

static const gchar *vcs_plugin_ui =
"<ui>"
	"<menubar name='MainMenu'>"
		"<menu action='ToolsMenu'>"
			"<menu action='ToolsVCS'>"
				"<menuitem action='VCSUpdate'/>"
				"<menuitem action='VCSCommit'/>"
			"</menu>"
		"</menu>"
	"</menubar>"
"</ui>";

static void
vcs_initgui(Tbfwin * bfwin)
{
	GtkActionGroup *action_group;
	GError *error = NULL;

	static const GtkActionEntry vcs_actions[] = {
		{"ToolsVCS", NULL, N_("Version Contro_l"), NULL,NULL, NULL},
		{"VCSUpdate", NULL, N_("Update"), NULL, NULL, G_CALLBACK(vcs_update)},
		{"VCSCommit", NULL, N_("Commit"), NULL, NULL, G_CALLBACK(vcs_commit)}
	};

	action_group = gtk_action_group_new("vcsActions");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE "_plugin_vcs");
	gtk_action_group_add_actions(action_group, vcs_actions, G_N_ELEMENTS(vcs_actions), bfwin);
	gtk_ui_manager_insert_action_group(bfwin->uimanager, action_group, 0);
	g_object_unref(action_group);

	gtk_ui_manager_add_ui_from_string(bfwin->uimanager, vcs_plugin_ui, -1, &error);
	if (error != NULL) {
		g_warning("building vcs plugin menu failed: %s", error->message);
		g_error_free(error);
	}
}

/*static void
vcs_enforce_session(Tbfwin * bfwin)
{
}
*/
static void
vcs_cleanup(void)
{
	g_hash_table_unref(vcs_v.lookup);
}

/*static void
vcs_cleanup_gui(Tbfwin * bfwin)
{
}
*/
/*static GHashTable *
vcs_register_globses_config(GHashTable * configlist)
{
	return configlist;
}
*/
static GHashTable *
vcs_register_session_config(GHashTable * configlist, Tsessionvars * session)
{
	Tvcssession *vs;
	vs = vcsession_get(session);
	configlist = make_config_list_item(configlist, &vs->type, 's', "vcs.type:", 0);
	configlist = make_config_list_item(configlist, &vs->basedir, 's', "vcs.basedir:", 0);
	return configlist;
}

static void
vcs_session_cleanup(Tsessionvars * session)
{
	g_hash_table_remove(vcs_v.lookup, session);
}

static TBluefishPlugin bfplugin = {
	"Version Control",
	BFPLUGIN_VERSION,
	GTK_MAJOR_VERSION,
	sizeof(Tdocument),
	sizeof(Tsessionvars),
	sizeof(Tglobalsession),
	sizeof(Tbfwin),
	sizeof(Tproject),
	sizeof(Tmain),
	sizeof(Tproperties),
	BFPLUGIN_PRIORITY_DEFAULT,
	1,
	NULL,						/* private */
	vcs_init,				/* init */
	vcs_initgui,
	NULL /*vcs_enforce_session*/,
	vcs_cleanup,
	NULL /*vcs_cleanup_gui*/,
	NULL /*vcs_register_globses_config*/,
	vcs_register_session_config,
	vcs_session_cleanup,
	NULL,						/* binary compatibility */
	NULL,
	NULL
};

G_MODULE_EXPORT TBluefishPlugin *
getplugin(void)
{
	return &bfplugin;
}
