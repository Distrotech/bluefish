# this script can help to find files that are actually not in use, run it from the basedir:
#
#for file in inline_images/*.png; do
#	BASE=`echo $file | sed -e 's/.png//' -e 's?inline_images/??'`
#	if ! egrep -q "[\\t ]*\{[0-9]+, ${BASE}\}" src/pixmap.c ; then
#		if grep -q "^[^#]*${BASE} ${BASE}.png" inline_images/Makefile; then
#			echo $BASE
#		fi
#	fi
#done

TOOLBAR = \
	abbr abbr.png \
	acronym acronym.png \
	anchor anchor.png \
	bluefish_icon1 bluefish_icon1.png \
	body body.png \
	bold bold.png \
	bookmarks bookmarks.png \
	brbreak brbreak.png \
	breakall breakall.png \
	center center.png \
	cite cite.png \
	close close.png \
	code code.png \
	color_select color_select.png \
	comment comment.png \
	context context.png \
	css_small css_small.png \
	cssdiv cssdiv.png \
	cssnewstyle cssnewstyle.png \
	cssspan cssspan.png \
	cssstyle cssstyle.png \
	dfn dfn.png \
	email email.png \
	embed embed.png \
	filetree filetree.png \
	font font.png \
	font_base font_base.png \
	fonth1 fonth1.png \
	fonth2 fonth2.png \
	fonth3 fonth3.png \
	fonth4 fonth4.png \
	fonth5 fonth5.png \
	fonth6 fonth6.png \
	fontm1 fontm1.png \
	fontp1 fontp1.png \
	fontpre fontpre.png \
	fontsub fontsub.png \
	fontsuper fontsuper.png \
	form form.png \
	form_check form_check.png \
	form_hidden form_hidden.png \
	form_option form_option.png \
	form_radio form_radio.png \
	form_select form_select.png \
	form_submit form_submit.png \
	form_text form_text.png \
	form_textarea form_textarea.png \
	form_optiongroup form_optiongroup.png \
	frame frame.png \
	frame2 frame2.png \
	frame_base frame_base.png \
	frame_no frame_no.png \
	frameset frameset.png \
	frameset2 frameset2.png \
	framewhiz framewhiz.png \
	fref fref.png \
	frdialog frdialog.png \
	frinfo frinfo.png \
	frsearch frsearch.png \
	headings headings.png \
	hrule hrule.png \
	image image.png \
	indent indent.png \
	italic italic.png \
	kbd kbd.png \
	linkstylesheet linkstylesheet.png \
	list list.png \
	list_dd list_dd.png \
	list_dl list_dl.png \
	list_dt list_dt.png \
	list_li list_li.png \
	list_ol list_ol.png \
	list_ul list_ul.png \
	meta meta.png \
	multithumbs multithumbs.png \
	nbsp nbsp.png \
	new new.png \
	nothing nothing.png \
	open_small open_small.png \
	paragraph paragraph.png \
	php3 php3.png \
	prefseditor prefs-editor.png \
	prefsfiletypes prefs-filetypes.png \
	prefsinterface prefs-interface.png \
	prefsexternal prefs-external.png \
	prefshtml prefs-html.png \
	prefsoutputparser prefs-outputparser.png \
	prefsfiles prefs-files.png \
	prefsimages prefs-images.png \
 	prefssyntax prefs-syntax.png \
	quick_start quick_start.png \
	right right.png \
	samp samp.png \
	table table.png \
	table2 table2.png \
	table_capt table_capt.png \
	table_td table_td.png \
	table_td2 table_td2.png \
	table_th table_th.png \
	table_th2 table_th2.png \
	table_tr table_tr.png \
	table_tr2 table_tr2.png \
	tablewhiz tablewhiz.png \
	thumbnail thumbnail.png \
	underline underline.png \
	unindent unindent.png \
	var var.png \
	view_in_browser view_in_browser.png
#	cancel cancel.png \
#	copy copy.png \
#	cssclass cssclass.png \
#	cut cut.png \
#	dtd dtd.png \
#	help help.png \
#	image_map image_map.png \
#	javascript_mouseover javascript_mouseover.png \
#	javascript_netscape javascript_netscape.png \
#	javascript_preload javascript_preload.png \
#	jscript jscript.png \
#	linkjscript linkjscript.png \
#	list_menu list_menu.png \
#	netscape netscape.png \
#	no no.png \
#	ok ok.png \
#	open open.png \
#	paste paste.png \
#	preferences preferences.png \
#	print print.png \
#	redo redo.png \
#	rxml rxml.png \
#	ssi ssi.png \
#	save save.png \
#	save_as save_as.png \
#	search search.png \
#	search_replace search_replace.png \
#	spellcheck spellcheck.png \
#	strikeout strikeout.png \
#	undo undo.png \
#	wml_a wml_a.png \
#	wml_access wml_access.png \
#	wml_bold wml_bold.png \
#	wml_break wml_break.png \
#	wml_card wml_card.png \
#	wml_do wml_do.png \
#	wml_go wml_go.png \
#	wml_italic wml_italic.png \
#	wml_nbsp wml_nbsp.png \
#	wml_noop wml_noop.png \
#	wml_paragraph wml_paragraph.png \
#	wml_postfield wml_postfield.png \
#	wml_prev wml_prev.png \
#	wml_refresh wml_refresh.png \
#	wml_standard_document wml_standard_document.png \
#	wml_timer wml_timer.png \
#	wml_var wml_var.png

GENERAL = \
	close_small close_small.png \
	edit_tag edit_tag.png

all:
	gdk-pixbuf-csource --raw --static --build-list $(TOOLBAR) > toolbar_icons.c
	gdk-pixbuf-csource --raw --extern --build-list $(GENERAL) > general_icons.c




