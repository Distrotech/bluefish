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
	pixmap_abbr abbr.png \
	pixmap_acronym acronym.png \
	pixmap_anchor anchor.png \
	pixmap_bluefish_icon1 bluefish_icon1.png \
	pixmap_body body.png \
	pixmap_bold bold.png \
	pixmap_bookmarks bookmarks.png \
	pixmap_brbreak brbreak.png \
	pixmap_breakall breakall.png \
	pixmap_center center.png \
	pixmap_cite cite.png \
	pixmap_close close.png \
	pixmap_code code.png \
	pixmap_color_select color_select.png \
	pixmap_comment comment.png \
	pixmap_context context.png \
	pixmap_css_small css_small.png \
	pixmap_cssdiv cssdiv.png \
	pixmap_cssnewstyle cssnewstyle.png \
	pixmap_cssspan cssspan.png \
	pixmap_cssstyle cssstyle.png \
	pixmap_dfn dfn.png \
	pixmap_email email.png \
	pixmap_embed embed.png \
	pixmap_filetree filetree.png \
	pixmap_font font.png \
	pixmap_font_base font_base.png \
	pixmap_fonth1 fonth1.png \
	pixmap_fonth2 fonth2.png \
	pixmap_fonth3 fonth3.png \
	pixmap_fonth4 fonth4.png \
	pixmap_fonth5 fonth5.png \
	pixmap_fonth6 fonth6.png \
	pixmap_fontm1 fontm1.png \
	pixmap_fontp1 fontp1.png \
	pixmap_fontpre fontpre.png \
	pixmap_fontsub fontsub.png \
	pixmap_fontsuper fontsuper.png \
	pixmap_form form.png \
	pixmap_form_check form_check.png \
	pixmap_form_hidden form_hidden.png \
	pixmap_form_option form_option.png \
	pixmap_form_radio form_radio.png \
	pixmap_form_select form_select.png \
	pixmap_form_submit form_submit.png \
	pixmap_form_text form_text.png \
	pixmap_form_textarea form_textarea.png \
	pixmap_form_optiongroup form_optiongroup.png \
	pixmap_frame frame.png \
	pixmap_frame2 frame2.png \
	pixmap_frame_base frame_base.png \
	pixmap_frame_no frame_no.png \
	pixmap_frameset frameset.png \
	pixmap_frameset2 frameset2.png \
	pixmap_framewhiz framewhiz.png \
	pixmap_fref fref.png \
	pixmap_frdialog frdialog.png \
	pixmap_frinfo frinfo.png \
	pixmap_frsearch frsearch.png \
	pixmap_headings headings.png \
	pixmap_hrule hrule.png \
	pixmap_image image.png \
	pixmap_indent indent.png \
	pixmap_italic italic.png \
	pixmap_kbd kbd.png \
	pixmap_linkstylesheet linkstylesheet.png \
	pixmap_list list.png \
	pixmap_list_dd list_dd.png \
	pixmap_list_dl list_dl.png \
	pixmap_list_dt list_dt.png \
	pixmap_list_li list_li.png \
	pixmap_list_ol list_ol.png \
	pixmap_list_ul list_ul.png \
	pixmap_meta meta.png \
	pixmap_multithumbs multithumbs.png \
	pixmap_nbsp nbsp.png \
	pixmap_new new.png \
	pixmap_nothing nothing.png \
	pixmap_open_small open_small.png \
	pixmap_paragraph paragraph.png \
	pixmap_php3 php3.png \
	pixmap_prefseditor prefs-editor.png \
	pixmap_prefsfiletypes prefs-filetypes.png \
	pixmap_prefsinterface prefs-interface.png \
	pixmap_prefsexternal prefs-external.png \
	pixmap_prefshtml prefs-html.png \
	pixmap_prefsoutputparser prefs-outputparser.png \
	pixmap_prefsfiles prefs-files.png \
	pixmap_prefsimages prefs-images.png \
 	pixmap_prefssyntax prefs-syntax.png \
	pixmap_quick_start quick_start.png \
	pixmap_right right.png \
	pixmap_samp samp.png \
	pixmap_table table.png \
	pixmap_table2 table2.png \
	pixmap_table_capt table_capt.png \
	pixmap_table_td table_td.png \
	pixmap_table_td2 table_td2.png \
	pixmap_table_th table_th.png \
	pixmap_table_th2 table_th2.png \
	pixmap_table_tr table_tr.png \
	pixmap_table_tr2 table_tr2.png \
	pixmap_tablewhiz tablewhiz.png \
	pixmap_thumbnail thumbnail.png \
	pixmap_underline underline.png \
	pixmap_unindent unindent.png \
	pixmap_var var.png \
	pixmap_view_in_browser view_in_browser.png
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
	pixmap_close_small close_small.png \
	pixmap_edit_tag edit_tag.png

all:
	gdk-pixbuf-csource --raw --extern --build-list $(TOOLBAR) > toolbar_icons.c
	gdk-pixbuf-csource --raw --extern --build-list $(GENERAL) > general_icons.c




