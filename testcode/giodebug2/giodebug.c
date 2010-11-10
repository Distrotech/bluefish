/*
compile with
gcc -Wall -g -O2 `pkg-config --cflags "glib-2.0 gio-2.0"`   -c -o giodebug.o giodebug.c
gcc  `pkg-config --libs "glib-2.0 gio-2.0"`  -o giodebug giodebug.o
*/
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

static void replace_contents_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	GFile *file=user_data;
	gchar *new_etag=NULL;
	GError *error=NULL;
	g_print("replace_contents_async_lcb, started\n");
	if (g_file_replace_contents_finish(file,res,&new_etag,&error)) {
		g_print("stored the data, new etag=%s\n",new_etag);
	} else if (error) {
		g_print("got error %d: %s\n",error->code,error->message);
		g_error_free(error);
	}
	g_object_unref(file);
	g_print("replace_contents_async_lcb, done, you might as well hit <ctrl><c> now\n");
}

static void load_contents_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
	gchar *contents=NULL, *etag_out=NULL;
	gsize length=0;
	GError *error=NULL;
	GFile *file=user_data;
	
	g_print("load_contents_async_lcb, started\n");
	if (g_file_load_contents_finish(file,res,&contents,&length,&etag_out,&error)) {
		g_print("%s",contents);
	} else if (error) {
		g_print("got error %d: %s\n",error->code,error->message);
		g_error_free(error);
	}
	g_free(contents);
	g_free(etag_out);
	g_object_unref(file);
	g_print("load_contents_async_lcb, done, you might as well hit <ctrl><c> now\n");
}

static void query_info_async_lcb(GObject *source_object,GAsyncResult *res,gpointer data) {
	GFileInfo *finfo;
	GFile *file=data;
	GError *gerror=NULL;
	
	finfo = g_file_query_info_finish(file,res,&gerror);
	if (!finfo) {
		g_print("query_info_async_lcb, error: %s\n",gerror->message);
		g_error_free(gerror);
	} 
	g_print("query_info_async_lcb, etag=%s\n",g_file_info_get_etag(finfo));

}

static gboolean timeout_check_changes_lcb(gpointer data) {
	GFile *file=data;
	g_print("check_changes_async_lcb, started\n");
	g_file_query_info_async(file,G_FILE_ATTRIBUTE_ETAG_VALUE,G_FILE_QUERY_INFO_NONE
				,G_PRIORITY_DEFAULT,NULL,query_info_async_lcb,file);
	g_print("check_changes_async_lcb, done\n");
	return TRUE;
}

int main(int argc, char **argv) {
	GMainLoop *mainloop;
	GFile *file;
	g_type_init();
	if (argc <= 2) {
		g_print("pass 'load <URL>' or 'write <URL>' as parameter. write will OVERWRITE !!!\n");
		exit(0);
	}
	
	mainloop = g_main_loop_new(NULL,TRUE);
	g_print("parsing %s\n",argv[2]);
	file = g_file_new_for_commandline_arg(argv[2]);
	
	if (strcmp(argv[1],"write")==0) {
		char *buffer = g_strdup("Hello world\n");
		/* this creates a memor leak */
		g_file_replace_contents_async(file,buffer,strlen(buffer),NULL,FALSE,0,NULL,replace_contents_async_lcb,file);
	} else {
		g_file_load_contents_async(file,NULL,load_contents_async_lcb,file);
	}
	g_timeout_add(80000, timeout_check_changes_lcb, file);
	g_main_loop_run(mainloop);
	return 0;
}
