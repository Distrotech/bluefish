
AUTOMAKE_OPTIONS = std-options

SUBDIRS = pixmaps\
	plugin_about\
	plugin_entities\
	plugin_htmlbar\
	plugin_infbrowser\
	plugin_snippets\
	plugin_charmap\
	plugin_zencoding

#	plugin_xmltools
# 	plugin_vcs

EXTRA_DIST = bluefish_rc.rc.in.in resource.h

CLEANFILES = bluefish_rc.rc

DISTCLEANFILES = bluefish_rc.rc.in

MAINTAINERCLEANFILES = Makefile.in config.h.in

DEFS += -DLOCALEDIR=\"$(localedir)\" -DPKGDATADIR=\"$(pkgdatadir)\" -DPKGLIBDIR=\"$(pkglibdir)\"

if OS_WIN32
AM_LDFLAGS = -export-all-symbols
DEFS += -DEXE_EXPORT_SYMBOLS
LDADD = -lws2_32
LDADD += bluefish_rc.o win32utils.o
else
AM_LDFLAGS = -export-dynamic
endif

bin_PROGRAMS = bluefish


bluefish_SOURCES = \
	async_queue.c \
	async_queue.h \
	bf_lib.c \
	bf_lib.h \
	blocksync.c \
	blocksync.h \
	bluefish.c \
	bluefish.h \
	bftextview2.c \
	bftextview2.h \
	bftextview2_private.h \
	bftextview2_langmgr.c \
	bftextview2_langmgr.h \
	bftextview2_autocomp.c \
	bftextview2_autocomp.h \
	bftextview2_identifier.c \
	bftextview2_identifier.h \
	bftextview2_markregion.c \
	bftextview2_markregion.h \
	bftextview2_patcompile.c \
	bftextview2_patcompile.h \
	bftextview2_scanner.c \
	bftextview2_scanner.h \
	bftextview2_spell.c \
	bftextview2_spell.h \
	bfwin.h \
	bfwin.c \
	bfwin_uimanager.h \
	bfwin_uimanager.c \
	bookmark.c \
	bookmark.h \
	dialog_utils.c \
	dialog_utils.h \
	document.c \
	doc_comments.h \
	doc_comments.c \
	doc_text_tools.h \
	doc_text_tools.c \
	doc_extern_tools.h \
	doc_extern_tools.c \
	document.h \
	encodings_dialog.c \
	encodings_dialog.h \
	external_commands.c \
	external_commands.h \
	file_treemodel.c \
	file_treemodel.h \
	filebrowser2.c \
	filebrowser2.h \
	file.c \
	file.h \
	file_autosave.c \
	file_autosave.h \
	file_dialogs.c \
	file_dialogs.h \
	filefilter.c \
	filefilter.h \
	gtk_easy.c \
	gtk_easy.h \
	ipc_bf2bf.c \
	ipc_bf2bf.h \
	languages.c \
	languages.h \
	msg_queue.c \
	msg_queue.h \
	outputbox.c \
	outputbox.h \
	pixmap.c \
	pixmap.h \
	plugins.c \
	plugins.h \
	preferences.c \
	preferences.h \
	print.c \
	print.h \
	project.c \
	project.h \
	rcfile.c \
	rcfile.h \
	snr3.c \
	snr3.h \
	snr3_files.c \
	snr3_files.h \
	stringlist.c \
	stringlist.h \
	undo_redo.c \
	undo_redo.h \
	xml_entity.c \
	xml_entity.h

bluefish_rc.rc: bluefish_rc.rc.in
	$(SED) -e "s#SRCDIR#$(top_srcdir)/win32/pixmaps#g" $< > $@

.rc.o:
	$(CC) -E -xc -DRC_INVOKED -I$(srcdir) $< | $(WINDRES) -o $@
