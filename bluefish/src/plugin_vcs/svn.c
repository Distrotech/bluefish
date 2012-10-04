

#include "vcs.h"
#include "vcs_gui.h"
#include "../external_commands.h"
#include "../outputbox.h"

static void
svn_commit_real_lcb(const gchar *output, gpointer bfwin, gpointer data)
{
	Tvcssession *vs=data;
	GRegex *greg;
	GMatchInfo *match_info;
	gchar *tmp, *linestart;

	if (!output || output[0] == '\0')
		return;
	g_print("svn_commit_real_lcb, output='%s'\n", output);
	greg = g_regex_new("(Adding|Sending)[ ]+(.+)", G_REGEX_CASELESS, 0, NULL);
	linestart = tmp = output;
	while (*tmp != '\0') {
		if (*tmp == '\n' || *tmp == '\r') {
			if (tmp == linestart+1) {
				linestart++;
			} else {
				gchar *curi=NULL;
				*tmp = '\0';
				/* now we have a line, extract the curi */
				g_print("linestart=%s\n",linestart);
				if (g_regex_match(greg, linestart, 0, &match_info)) {
					gchar *file = g_match_info_fetch(match_info, 2);
					curi = g_strconcat(vs->basedir, "/", file, NULL);
					g_print("file=%s, curi=%s\n",file,curi);
					g_free(file);
				}
				outputbox_add_line(BFWIN(bfwin), curi, -1, linestart);
				g_free(curi);
				g_match_info_free(match_info);
				linestart = tmp+1;
			}
		}
		tmp++;
	}
	g_regex_unref(greg);
}

void
svn_commit_real(Tbfwin *bfwin, Tvcssession *vs, GList *files, const gchar *message)
{
	GString *str;
	GList *tmplist;

	str = g_string_new("cd ");
	g_string_append_printf(str, "%s && svn commit -m '", vs->basedir);
	g_string_append(str, message);
	g_string_append(str, "' ");
	for(tmplist=files;tmplist;tmplist=tmplist->next) {
		g_string_append(str, tmplist->data);
		g_string_append(str, " ");
	}
	g_string_append(str, "|");
	g_print("command='%s'\n",str->str);
	custom_command(bfwin, str->str, svn_commit_real_lcb, vs);
	g_string_free(str, TRUE);
}

static GFile *
parent_with_dotsvn(GFile *cururi)
{
	GFile *parent, *dotsvn, *tmp, *returi=NULL;
	gboolean exists;
	tmp = cururi;
	g_object_ref(tmp);
	while (tmp) {
		parent = g_file_get_parent(tmp);
		g_object_unref(tmp);
		if (!parent)
			return returi;
		dotsvn = g_file_get_child(parent, ".svn");
		g_print("try %s\n",g_file_get_path(dotsvn));
		exists = g_file_query_exists(dotsvn, NULL);
		g_object_unref(dotsvn);
		if (!exists) {
			return returi;
		} else {
			returi = parent;
			g_object_ref(returi);
		}
		tmp = parent;
	}
	return returi;
}

gboolean
svn_autoconfigure(Tvcssession *vs, GFile *cururi)
{
	GFile *baseuri;
	if (!g_file_is_native(cururi)) {
		return FALSE;
	}
	baseuri = parent_with_dotsvn(cururi);
	if (baseuri) {
		vs->basedir = g_file_get_path(baseuri);
		g_object_unref(baseuri);
		return TRUE;
	}
	return FALSE;
}


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

static void
svn_update_lcb(const gchar *output, gpointer bfwin, gpointer data)
{
	Tvcssession *vs=data;
	GRegex *greg;
	GMatchInfo *match_info;
	gchar *tmp, *linestart;

	if (!output || output[0] == '\0')
		return;
	g_print("svn_update_lcb, output='%s'\n",output);
	greg = g_regex_new("(A|U)[ ]+(.+)", G_REGEX_CASELESS, 0, NULL);
	linestart = tmp = output;
	while (*tmp != '\0') {
		if (*tmp == '\n' || *tmp == '\r') {
			if (tmp == linestart+1) {
				g_print("advance linestart one byte\n");
				linestart++;
			} else {
				gchar *curi=NULL;
				*tmp = '\0';
				/* now we have a line, extract the curi */
				g_print("linestart=%s\n",linestart);
				if (g_regex_match(greg, linestart, 0, &match_info)) {
					gchar *file = g_match_info_fetch(match_info, 2);
					curi = g_strconcat(vs->basedir, "/", file, NULL);
					g_print("file=%s, curi=%s\n",file,curi);
					g_free(file);
				}
				g_print("send to outputbox, curi=%s, linestart=%s\n",curi,linestart);
				outputbox_add_line(BFWIN(bfwin), curi, -1, linestart);
				g_free(curi);
				g_match_info_free(match_info);
				linestart = tmp+1;
			}
		}
		tmp++;
	}
	g_regex_unref(greg);
}

void
svn_update(Tbfwin *bfwin, const gchar *basedir, gpointer data)
{
	gchar *command;
	g_print("svn_update, basedir=%s\n",basedir);
	if (!basedir)
		return;
	
	command = g_strdup_printf("cd %s && svn up|", basedir);
	g_print("svn_update, command=%s\n",command);
	custom_command(bfwin, command, svn_update_lcb, data);
	g_free(command);
}
