

#include "vcs.h"
#include "vcs_gui.h"
#include "external_commands.h"

static Tvcsstatus
vcsstatus_from_svnstatus(gchar svnstatus)
{
	switch(svnstatus) {
		case 'M':
			return vcsstatus_modified;
		case 'C':
			return vcsstatus_conflict;
	}
	return vcsstatus_normal;
}

static void
svn_status_lcb(const gchar *output, gpointer bfwin, gpointer data)
{
	gchar *status;
	gchar *filename;
	GList *list=NULL;
	Tvcsfilestatus *fs;
	
	if (!output || output[0] == '\0')
		return;
	
	g_print("svn_status_lcb, output='%s'\n",output);
	
	status = strtok((gchar *)output, " \n\t");
	while (status && (
			status[0] == 'U' || 
			status[0] == 'G' || 
			status[0] == 'C' ||
			status[0] == 'M' ||
			status[0] == '?'
			)) {
		fs = g_slice_new(Tvcsfilestatus);
		fs->status = vcsstatus_from_svnstatus(status[0]);
		filename = strtok(NULL, " \n\t");
		fs->filename = g_strdup(filename);
		list = g_list_prepend(list, fs);
		status = strtok(NULL, " \n\t");
	}
	vcs_gui_update_status(data, list);
}


void
svn_status(Tbfwin *bfwin, const gchar *basedir, gpointer data)
{
	gchar *command = g_strdup_printf("cd %s && svn st|",basedir);
	g_print("start %s\n",command);
	custom_command(bfwin, command, svn_status_lcb, data);
	g_free(command);
}
