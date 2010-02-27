/* Bluefish HTML Editor
 * bftextview2_spell.c
 *
 * Copyright (C) 2009-2010 Olivier Sessink
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
/*#define SPELL_PROFILING*/

/* for the design docs see bftextview2.h */

#include "bluefish.h"

#ifdef HAVE_LIBENCHANT
#ifdef HAVE_ENCHANT_ENCHANT_H
#include <enchant/enchant.h>
#else /* HAVE_ENCHANT_ENCHANT_H */
#include <enchant.h>
#endif /* HAVE_ENCHANT_ENCHANT_H */
#include <string.h> /*strlen*/

#include "bftextview2_scanner.h"
#include "bftextview2_langmgr.h"
#include "bftextview2_spell.h"
#include "document.h"

/*#undef DBG_SPELL
#define DBG_SPELL g_print*/

#define MAX_CONTINUOUS_SPELLCHECK_INTERVAL 0.1 /* float in seconds */

static EnchantBroker *eb;
static guint loops_per_timer=1000;

static gboolean bftextview2_find_region2spellcheck(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	/* first find a region that needs a spellcheck */
	gtk_text_buffer_get_start_iter(buffer, start);
	if (!gtk_text_iter_begins_tag(start,btv->needspellcheck) ) {
		DBG_SPELL("iter %d does not begins tag needspellcheck %p, needscanning(%p)=%d\n",gtk_text_iter_get_offset(start),btv->needspellcheck,btv->needscanning,gtk_text_iter_begins_tag(start,btv->needscanning));
		if (!gtk_text_iter_forward_to_tag_toggle(start,btv->needspellcheck)) {
			/* nothing to spellcheck */
			DBG_SPELL("tag needspellcheck is never started\n");
			return FALSE;
		}
	}
	/* find the end of the region */
	*end = *start;
	gtk_text_iter_forward_char(end);
	if (!gtk_text_iter_ends_tag(end,btv->needspellcheck)) {
		if (!gtk_text_iter_forward_to_tag_toggle(end,btv->needspellcheck)) {
			DBG_MSG("BUG: we should never get here\n");
			return FALSE;
		}
	}
	return TRUE;
}

static void dicts_load_first_lcb(const char * const lang_tag,const char * const provider_name,const char * const provider_desc,const char * const provider_file,void *data) {
	Tbfwin *bfwin=data;
	if (!bfwin->ed && lang_tag) {
		bfwin->ed = (void *)enchant_broker_request_dict(eb, lang_tag);
		DBG_SPELL("loaded first available dictionary %s at %p\n", lang_tag, bfwin->ed);
		if (bfwin->ed) {
			g_free(bfwin->session->spell_lang);
			bfwin->session->spell_lang = g_strdup(lang_tag);
		}
	}
}

void unload_spell_dictionary(Tbfwin *bfwin) {
	if (bfwin->ed) {
#ifdef HAVE_LIBENCHANT_1_4
		DBG_SPELL("unload_spell_dictionary, bfwin=%p, ed=%p\n",bfwin,bfwin->ed);
		enchant_broker_free_dict(eb, (EnchantDict *)bfwin->ed);
#else
		/* enchant < 1.4.0 does not do refcounting, so we have to check ourselves if we are the last window that
		is using this dictionary */
		GList *tmplist;
		gboolean in_use=FALSE;
		for (tmplist=g_list_first(main_v->bfwinlist);tmplist;tmplist=g_list_next(tmplist)) {
			if (tmplist->data != bfwin && BFWIN(tmplist->data)->ed == bfwin->ed) {
				in_use=TRUE;
				DBG_SPELL("keep dictionary %p, it is in use by bfwin %p\n",bfwin->ed, tmplist->data);
				break;
			}
		}
		if (!in_use) {
			DBG_SPELL("free dictionary %p, it is not in use\n",bfwin->ed);
			enchant_broker_free_dict(eb, (EnchantDict *)bfwin->ed);
		}
#endif
	}
}

static gboolean load_dictionary(Tbfwin *bfwin) {
	unload_spell_dictionary(bfwin);
	if (bfwin->session->spell_lang && bfwin->session->spell_lang[0]!='\0' && enchant_broker_dict_exists(eb,bfwin->session->spell_lang)) {
	    DBG_SPELL("load_dictionary called for bfwin %p which has session->spell_lang=%s\n",bfwin, bfwin->session->spell_lang);
		bfwin->ed = (void *)enchant_broker_request_dict(eb, bfwin->session->spell_lang);
		DBG_SPELL("loaded dictionary %s at %p\n", bfwin->session->spell_lang, bfwin->ed);
		return (bfwin->ed != NULL);
	} else {
		bfwin->ed = NULL;
		/* load the first existing enchant dict */
		enchant_broker_list_dicts(eb,dicts_load_first_lcb,bfwin);
		return (bfwin->ed != NULL);
	}
}

static void spellcheck_word(BluefishTextView * btv, GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end) {
	gchar *tocheck;
	
	tocheck = gtk_text_buffer_get_text(buffer, start,end, FALSE);
	DBG_SPELL("spellcheck_word, check word %s in dictionary %p\n",tocheck,BFWIN(DOCUMENT(btv->doc)->bfwin)->ed);
	if (enchant_dict_check((EnchantDict *)BFWIN(DOCUMENT(btv->doc)->bfwin)->ed, tocheck, strlen(tocheck)) != 0) {
		DBG_SPELL("'%s' *not* spelled correctly!\n", tocheck);
		gtk_text_buffer_apply_tag_by_name(buffer, "_spellerror_", start, end);
	} else {
		DBG_SPELL("'%s' spelled correctly!\n", tocheck);
	}
	g_free(tocheck);
}

/* handle apostrophe in word gracefully */ 
static inline gboolean text_iter_forward_real_word_end(GtkTextIter *it) {
	GtkTextIter iter;
	if (!gtk_text_iter_forward_word_end(it))
		return FALSE;
	if (gtk_text_iter_get_char(it) != '\'')
		return TRUE;
	iter = *it;
	if (gtk_text_iter_forward_char(&iter)) {
		if (g_unichar_isalpha(gtk_text_iter_get_char(&iter))) {
			return (gtk_text_iter_forward_word_end(it));
		}
	}
	return TRUE;
}

#ifdef HAVE_LIBENCHANT_OLD
static gint OLD_foundstack_needspellcheck(BluefishTextView * btv, Tfoundstack *fstack) {
	guint16 contextnum;
	if (g_queue_get_length(fstack->contextstack))
		contextnum = ((Tfoundcontext *)g_queue_peek_head(fstack->contextstack))->context;
	else
		contextnum = 1;
	DBG_SPELL("context %d has spellcheck=%d\n",contextnum, g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).needspellcheck);
	return g_array_index(btv->bflang->st->contexts,Tcontext, contextnum).needspellcheck;
}

/*static inline gboolean OLD_text_iter_backward_real_word_start(GtkTextIter *i) {
	GtkTextIter iter;
	if (!gtk_text_iter_backward_word_start(i))
		return FALSE;
	iter = *i;
	if (gtk_text_iter_backward_char(&iter) && gtk_text_iter_get_char(&iter) == '\'' 
					&& gtk_text_iter_backward_char(&iter) && g_unichar_isalpha(gtk_text_iter_get_char(&iter)))
		return gtk_text_iter_backward_word_start(i);
	return TRUE;
}
*/

gboolean OLD_bftextview2_run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	Tfoundstack *fstack;
	GSequenceIter *siter=NULL;
	GTimer *timer;
	gint loop=0, loops_per_timer=100;
	guint eo_offset;
	gboolean cont=TRUE;
	if (!BFWIN(DOCUMENT(btv->doc)->bfwin)->session->spell_enable)
		return FALSE;
	
	if (!BFWIN(DOCUMENT(btv->doc)->bfwin)->ed && !load_dictionary(BFWIN(DOCUMENT(btv->doc)->bfwin))) {
		DBG_SPELL("bftextview2_run_spellcheck, no dictionary.. return..\n");
		return FALSE;
	}

	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo)) {
		DBG_SPELL("bftextview2_run_spellcheck, no region to spellcheck found... return FALSE\n");
		return FALSE;
	}
	DBG_SPELL("bftextview2_run_spellcheck, loop1 from %d to %d\n",gtk_text_iter_get_offset(&so),gtk_text_iter_get_offset(&eo));
	timer = g_timer_new();
	iter=so;
	eo_offset = gtk_text_iter_get_offset(&eo);
	do {
		GtkTextIter eo2=eo;
		gboolean cont2=TRUE;
		if (btv->bflang->st) {
			/**** find the start within this region of a context that needs spellcheck */
			fstack = get_stackcache_at_position(btv, &iter, &siter);
			if (fstack || !g_array_index(btv->bflang->st->contexts,Tcontext, 1).needspellcheck) {
				if(!fstack) {
					fstack = get_stackcache_first(btv, &siter);
					if (fstack->charoffset > eo_offset) {
						/* nothing to do ! */
						DBG_SPELL("bftextview2_run_spellcheck, next starting fstack beyond eo (1), we're finished\n");
						cont=FALSE;
						cont2=FALSE;
						iter=eo;
					}
				}
				while (fstack && fstack->charoffset < eo_offset && !foundstack_needspellcheck(btv,fstack)) {
					fstack = get_stackcache_next(btv, &siter);
				}
				
				if (fstack && fstack->charoffset < eo_offset) {
					/* advance the iter to the charoffset of fstack, but keep at the same place if the iter is already further */
					if (fstack->charoffset > gtk_text_iter_get_offset(&iter)) {
						gtk_text_iter_set_offset(&iter, fstack->charoffset);
						DBG_SPELL("bftextview2_run_spellcheck, advance iter to %d\n",gtk_text_iter_get_offset(&iter));
					}
				} else {
					DBG_SPELL("bftextview2_run_spellcheck, next starting fstack beyond eo (2), we're finished\n");
					cont=FALSE;
					cont2=FALSE;
					iter=eo;
				}
			} else { /* no fstack and the default context needs spellcheck */
				DBG_SPELL("bftextview2_run_spellcheck, default context, keep iter at %d\n",gtk_text_iter_get_offset(&iter));
				fstack = get_stackcache_first(btv, &siter);
			}
	
			/***** now find the end of this context to spellcheck */
			if (cont && cont2) {
				while(fstack && fstack->charoffset < eo_offset && foundstack_needspellcheck(btv,fstack)) {
					fstack = get_stackcache_next(btv, &siter);
				}
				if (fstack && fstack->charoffset < eo_offset) {
					/* set eo2 to the end of the context(s) that needs spellcheck */
					gtk_text_iter_set_offset(&eo2, fstack->charoffset);
					DBG_SPELL("bftextview2_run_spellcheck, set eo2 to %d\n",gtk_text_iter_get_offset(&eo2));
				} else {
					/* no next, set end iter to eo */
					eo2=eo;
					DBG_SPELL("bftextview2_run_spellcheck, set eo2 to eo at %d\n",gtk_text_iter_get_offset(&eo2));
				}
			}
		} else { /* no scantable */
			eo2=eo;
		}
		DBG_SPELL("bftextview2_run_spellcheck, loop2 from %d to %d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&eo2));
		while (cont2 && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL)) { /* loop from iter to eo2 */
			loop++;
			if (gtk_text_iter_forward_word_end(&iter) && gtk_text_iter_compare(&iter, &eo2) <= 0) {
				GtkTextIter wordstart;
				gtk_text_iter_backward_word_start(&iter);

				wordstart=iter;
				/* move to the next word as long as the we don't hit eo2 */
				if (text_iter_forward_real_word_end(&iter)) {
					/* check word */
					spellcheck_word(btv, buffer, &wordstart, &iter);
				}
			} else {
				DBG_SPELL("bftextview2_run_spellcheck, no word start within region\n");
				iter=eo2;
				cont2=FALSE;
			}
			if (cont2 && gtk_text_iter_compare(&iter, &eo2) >= 0)
				cont2 = FALSE;
		}

		if (gtk_text_iter_compare(&iter, &eo) >= 0) /* TODO: check the order of the compare items */
			cont = FALSE;
			
	} while (cont && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL));
	DBG_SPELL("bftextview2_run_spellcheck, remove needspellcheck from start %d to iter at %d\n",gtk_text_iter_get_offset(&so),gtk_text_iter_get_offset(&iter));
	gtk_text_buffer_remove_tag(buffer, btv->needspellcheck, &so , &iter);

	g_timer_destroy(timer);
	
	return TRUE;
}

#else

static gboolean has_tags(GSList *tags, GtkTextTag **tagarr) {
	GSList *tmpslist = tags;
	while (tmpslist) {
		gint i=0;
		while (tagarr[i]) {
			if (tagarr[i] == tmpslist->data) {
				/*g_print("has_tags, return TRUE for tag %p in tagarray[%d] %p\n",tagarr[i],i,tagarr);*/
				return TRUE;
			}
			i++;
		}
		tmpslist=tmpslist->next;
	}
	return FALSE;
}

static gboolean get_next_region(BluefishTextView * btv, GtkTextIter *so, GtkTextIter *eo) {
	gboolean fso=FALSE,feo=FALSE;
	GtkTextIter iter=*so;
	GSList *tmpslist;
	/* search start */
	DBG_SPELL("get_next_region, search from %d to %d\n",gtk_text_iter_get_offset(so),gtk_text_iter_get_offset(eo));
	do {
		tmpslist = gtk_text_iter_get_tags(&iter);
		if (has_tags(tmpslist, langmgr_need_spellcheck_tags())) {
			/* yes we need to scan */
			DBG_SPELL("found tag to scan at %d\n",gtk_text_iter_get_offset(&iter));
			fso=TRUE;
		} else if (has_tags(tmpslist, langmgr_no_spellcheck_tags())) {
			/* do not scan */
			DBG_SPELL("found tag not to scan, skip at %d\n",gtk_text_iter_get_offset(&iter));
		} else {
			/* scan depending on the settings of the language if it needs spell checking in default area's */
			if (btv->bflang->default_spellcheck) {
				DBG_SPELL("no tags: scan at %d\n",gtk_text_iter_get_offset(&iter));
				fso=TRUE;
			}
		}
		g_slist_free(tmpslist);
	} while (fso==FALSE && gtk_text_iter_forward_to_tag_toggle(&iter, NULL) && !gtk_text_iter_is_end(&iter));
	if (fso) {
		*so = iter;
		
		/* search end */
		while (feo==FALSE && gtk_text_iter_forward_to_tag_toggle(&iter, NULL) && !gtk_text_iter_is_end(&iter)) {
			tmpslist = gtk_text_iter_get_tags(&iter);
			DBG_SPELL("got %d tags at %d\n",g_slist_length(tmpslist),gtk_text_iter_get_offset(&iter));
			if (has_tags(tmpslist, langmgr_need_spellcheck_tags())) {
				/* yes we need to scan */
				DBG_SPELL("found tag to scan, continue scanning at %d\n",gtk_text_iter_get_offset(&iter));
			} else if (has_tags(tmpslist, langmgr_no_spellcheck_tags())) {
				/* do not scan */
				DBG_SPELL("found tag not to scan at %d\n",gtk_text_iter_get_offset(&iter));
				feo=TRUE;
			} else {
				/* scan depending on the settings of the language if it needs scanning in default area's */
				if (!btv->bflang->default_spellcheck) {
					DBG_SPELL("no tags, no scan at %d\n",gtk_text_iter_get_offset(&iter));
					feo=TRUE;
				}
			}
			g_slist_free(tmpslist);
		}
		if (feo) {
			*eo = iter;
			return TRUE;
		}
	}
	DBG_SPELL("get_next_region, return FALSE\n");
	return FALSE;
}

gboolean bftextview2_run_spellcheck(BluefishTextView * btv) {
	GtkTextIter so,eo,iter;
	GtkTextBuffer *buffer;
	GTimer *timer;
	gint loop=0;
	gboolean cont=TRUE;
#ifdef SPELL_PROFILING
	guint profile_words=0;
#endif	
	
	if (!btv->spell_check)
		return FALSE;
	
	if (!BFWIN(DOCUMENT(btv->doc)->bfwin)->ed && !load_dictionary(BFWIN(DOCUMENT(btv->doc)->bfwin))) {
		DBG_SPELL("bftextview2_run_spellcheck, no dictionary.. return..\n");
		return FALSE;
	}
	
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(btv));
	if (!bftextview2_find_region2spellcheck(btv, buffer, &so, &eo)) {
		DBG_SPELL("bftextview2_run_spellcheck, no region to spellcheck found... return FALSE\n");
		return FALSE;
	}
	DBG_SPELL("bftextview2_run_spellcheck, in bfwin=%p, bfwin->ed=%p loop1 from %d to %d\n",DOCUMENT(btv->doc)->bfwin,BFWIN(DOCUMENT(btv->doc)->bfwin)->ed,gtk_text_iter_get_offset(&so),gtk_text_iter_get_offset(&eo));
	timer = g_timer_new();
	iter=so;
	do {
		GtkTextIter eo2=eo;
		gboolean cont2=TRUE;
		if (btv->bflang->st) {
			cont2 = get_next_region(btv, &iter, &eo2);
			if (!cont2) {
				iter=eo;
			}
		} else { /* no scantable */
			eo2=eo;
		}
		DBG_SPELL("bftextview2_run_spellcheck, loop2 from %d to %d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&eo2));
		while (cont2 && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL)) { /* loop from iter to eo2 */
			loop++;
			DBG_SPELL("iter at %d, now forward to word end\n",gtk_text_iter_get_offset(&iter));
			if (gtk_text_iter_forward_word_end(&iter) && gtk_text_iter_compare(&iter, &eo2) <= 0) {
				GtkTextIter wordstart=iter;
				gtk_text_iter_backward_word_start(&wordstart);
				DBG_SPELL("iter at %d, backward wordstart at %d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&wordstart));
				
				/* handle apostrophe gracefully */
				if (gtk_text_iter_get_char(&iter) == '\'') {
					DBG_SPELL("handle apostrophe, forward to next word end\n");
					if (gtk_text_iter_forward_char(&iter)) {
						if (g_unichar_isalpha(gtk_text_iter_get_char(&iter))) {
							gtk_text_iter_forward_word_end(&iter);
						}
					}
				}
				/* check word */
#ifdef SPELL_PROFILING
				profile_words++;
#endif
				spellcheck_word(btv, buffer, &wordstart, &iter);
			} else {
				DBG_SPELL("bftextview2_run_spellcheck, no word end within region\n");
				iter=eo2;
				cont2=FALSE;
			}
			DBG_SPELL("iter=%d, eo=%d, eo2=%d\n",gtk_text_iter_get_offset(&iter),gtk_text_iter_get_offset(&eo),gtk_text_iter_get_offset(&eo2));
			if (cont2 && gtk_text_iter_compare(&iter, &eo2) >= 0)
				cont2 = FALSE;
		}

		if (gtk_text_iter_compare(&iter, &eo) >= 0) {
			DBG_SPELL("bftextview2_run_spellcheck, iter (%d) equals eo, finished this area for spell checking\n", gtk_text_iter_get_offset(&iter));
			cont = FALSE;
		}
			
	} while (cont && (loop%loops_per_timer!=0 || g_timer_elapsed(timer,NULL)<MAX_CONTINUOUS_SPELLCHECK_INTERVAL));
	if (cont) {
		loops_per_timer = MAX(loop/10,100);
	}
#ifdef SPELL_PROFILING
	g_print("timing for this %d ms spell run: %d words\n",(gint)(1000.0*g_timer_elapsed(timer,NULL)), profile_words);
#endif
	DBG_SPELL("bftextview2_run_spellcheck, remove needspellcheck from start %d to iter at %d\n",gtk_text_iter_get_offset(&so),gtk_text_iter_get_offset(&iter));
	gtk_text_buffer_remove_tag(buffer, btv->needspellcheck, &so , &iter);

	g_timer_destroy(timer);
	
	return (!gtk_text_iter_is_end(&iter));
}



#endif

void bftextview2_spell_init(void) {
	eb = enchant_broker_init();
	if (!eb) {
		g_warning("could not initialize spell checking engine\n");
		return;
	}
}

void bftextview2_spell_cleanup(void) {
	enchant_broker_free(eb);
	eb=NULL;
}

static void recheck_document(Tdocument *doc) {
	GtkTextIter start,end;
	gtk_text_buffer_get_bounds(doc->buffer,&start,&end);
	gtk_text_buffer_apply_tag(doc->buffer, BLUEFISH_TEXT_VIEW(doc->view)->needspellcheck, &start, &end);
}

static void recheck_bfwin(Tbfwin *bfwin) {
	GList *tmplist;
	for (tmplist=g_list_first(bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
		recheck_document(DOCUMENT(tmplist->data));
	}
	bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
}


static gboolean get_misspelled_word_at_bevent(BluefishTextView *btv, GtkTextIter *wordstart, GtkTextIter *wordend) {
	GtkTextTag *misspelled;
	
	misspelled = gtk_text_tag_table_lookup(langmgr_get_tagtable(), "_spellerror_");
	gtk_text_buffer_get_iter_at_offset(GTK_TEXT_VIEW(btv)->buffer,wordstart,main_v->bevent_charoffset);
	if (gtk_text_iter_has_tag(wordstart, misspelled)) {
		*wordend=*wordstart;
		if (gtk_text_iter_backward_to_tag_toggle(wordstart,misspelled) && gtk_text_iter_forward_to_tag_toggle(wordend,misspelled))
			return TRUE;
	}
	return FALSE;
}

static void bftextview2_add_word_backend(BluefishTextView *btv, Tbfwin *bfwin, gboolean to_dict) {
	GtkTextIter so,eo;
	gchar *word;
	if (!get_misspelled_word_at_bevent(btv, &so,&eo)) 
		return;
	
	word = gtk_text_buffer_get_text(GTK_TEXT_VIEW(btv)->buffer,&so,&eo,FALSE);
	if (to_dict) {
#ifdef HAVE_LIBENCHANT_1_4
		enchant_dict_add((EnchantDict *)bfwin->ed, word,strlen(word));
#else
		enchant_dict_add_to_pwl((EnchantDict *)bfwin->ed, word,strlen(word));
#endif
	} else {
		enchant_dict_add_to_session((EnchantDict *)bfwin->ed, word,strlen(word));
	}
	recheck_bfwin(bfwin);
}

static void bftexview2_add_word_to_dict(GtkWidget *widget, gpointer data) {
	Tdocument*doc = data;
	bftextview2_add_word_backend(BLUEFISH_TEXT_VIEW(doc->view), BFWIN(doc->bfwin), TRUE);
}

static void bftexview2_add_word_to_ses(GtkWidget *widget, gpointer data) {
	Tdocument*doc = data;
	bftextview2_add_word_backend(BLUEFISH_TEXT_VIEW(doc->view), BFWIN(doc->bfwin), FALSE);
}

static void bftextview2_suggestion_menu_lcb(GtkWidget *widget, gpointer data) {
	Tdocument *doc=data;
	GtkTextIter wordstart,wordend;
	if (main_v->bevent_doc != doc)
		return;
	DBG_SPELL("chosen %s\n",gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child)));
	if (get_misspelled_word_at_bevent(BLUEFISH_TEXT_VIEW(doc->view), &wordstart, &wordend)) {
		gint start,end;
		/* no need to remove the tag because the text with this tag is deleted by the replace
		gtk_text_buffer_remove_tag_by_name(doc->buffer, "_spellerror_", &wordstart, &wordend);*/
		start = gtk_text_iter_get_offset(&wordstart);
		end = gtk_text_iter_get_offset(&wordend);
		doc_replace_text(doc, gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child)), start, end);
	}
}

static void bftextview2_preferences_menu_lcb(GtkWidget *widget, gpointer data) {
	Tbfwin *bfwin=data;
	if (GTK_CHECK_MENU_ITEM(widget)->active) {
		if (bfwin->session->spell_lang)
			g_free(bfwin->session->spell_lang);
		bfwin->session->spell_lang = g_strdup(gtk_label_get_text(GTK_LABEL(GTK_BIN(widget)->child)));
		if (load_dictionary(bfwin)) {
			GList *tmplist;			
			/* now mark all documents in this window with 'need_spellcheck'*/
			for (tmplist=g_list_first(bfwin->documentlist);tmplist;tmplist=g_list_next(tmplist)) {
				Tdocument *doc = DOCUMENT(tmplist->data);
				GtkTextIter start,end;
				gtk_text_buffer_get_bounds(doc->buffer,&start,&end);
				gtk_text_buffer_apply_tag(doc->buffer, BLUEFISH_TEXT_VIEW(doc->view)->needspellcheck, &start, &end);
			}
			bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
		}
	}
}

typedef struct {
	Tbfwin *bfwin;
	GtkWidget *menu;
	GSList *group;
} Tdictlist;

static void list_dicts_lcb(const char * const lang_tag,const char * const provider_name,const char * const provider_desc,const char * const provider_file,void *data) {
	Tdictlist *dl=data;
	GtkWidget *menuitem;
	/*DBG_SPELL("lang_tag=%s, provider_name=%s, provider_desc=%s, provider_file=%s\n",lang_tag,provider_name,provider_desc,provider_file);*/
	menuitem = gtk_radio_menu_item_new_with_label(dl->group, lang_tag);
	if (!dl->group)
		dl->group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menuitem));
	if (g_strcmp0(dl->bfwin->session->spell_lang, lang_tag)==0) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(menuitem, "activate", G_CALLBACK(bftextview2_preferences_menu_lcb), dl->bfwin);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(dl->menu), GTK_WIDGET(menuitem));
}

void bftextview2_populate_suggestions_popup(GtkMenu *menu, Tdocument *doc) {
	GtkTextIter wordstart,wordend;
	Tdictlist dl;
	GtkWidget *menuitem, *submenu;
	
	if (main_v->bevent_doc != doc)
		return;
	
	if (!BLUEFISH_TEXT_VIEW(doc->view)->spell_check)
		return;
	
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));

	menuitem = gtk_image_menu_item_new_with_label(_("Spell check language"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_SPELL_CHECK, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
	
	submenu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), submenu);
	
	dl.bfwin = doc->bfwin;
	dl.menu = submenu;
	dl.group=NULL;
	enchant_broker_list_dicts(eb, list_dicts_lcb, &dl);
	
	if (!BFWIN(doc->bfwin)->ed)
		return;
	
	if (get_misspelled_word_at_bevent(BLUEFISH_TEXT_VIEW(doc->view), &wordstart, &wordend)) {
		gchar *word, **suggestions;
		size_t n_suggs;

		menuitem = gtk_image_menu_item_new_with_label(_("Add to dictionary"));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
		g_signal_connect(menuitem, "activate", G_CALLBACK(bftexview2_add_word_to_dict), doc);
		menuitem = gtk_image_menu_item_new_with_label(_("Ignore spelling"));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
		g_signal_connect(menuitem, "activate", G_CALLBACK(bftexview2_add_word_to_ses), doc);

		word = gtk_text_buffer_get_text(GTK_TEXT_VIEW(doc->view)->buffer, &wordstart,&wordend,FALSE);
		DBG_SPELL("list alternatives for %s\n", word);
		suggestions = enchant_dict_suggest((EnchantDict *)BFWIN(doc->bfwin)->ed, word,strlen(word), &n_suggs);
		if (suggestions) {
			GtkWidget *menuitem;
			gint i;
			gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(gtk_menu_item_new()));
			for (i=0; i<n_suggs; i++) {
				menuitem = gtk_image_menu_item_new_with_label(suggestions[i]);
				g_signal_connect(menuitem, "activate", G_CALLBACK(bftextview2_suggestion_menu_lcb), doc);
				/*gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU));*/
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
			}
			
			enchant_dict_free_string_list((EnchantDict *)BFWIN(doc->bfwin)->ed, suggestions);
		}
		g_free(word);
	}
}

/*
static void bftextview2_preferences_menu_enable_lcb(GtkWidget *widget, gpointer data) {
	Tbfwin *bfwin=data;
	bfwin->session->spell_enable = GTK_CHECK_MENU_ITEM(widget)->active;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bfwin->toolbar_spell),bfwin->session->spell_enable);
}

void bftextview2_populate_preferences_popup(GtkMenu *menu, Tdocument *doc) {
	GtkWidget *menuitem, *submenu;
	Tdictlist dl;
	

	menuitem = gtk_check_menu_item_new_with_label(_("Enable spell check"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), BFWIN(doc->bfwin)->session->spell_enable);
	g_signal_connect(menuitem, "activate", G_CALLBACK(bftextview2_preferences_menu_enable_lcb), doc->bfwin);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));
}*/

/*void bftextview2_gui_toggle_spell_check(GtkWidget *widget, gpointer data) {
	Tbfwin *bfwin=data;
	bfwin->session->spell_enable = GTK_TOGGLE_BUTTON(widget)->active;
	if (bfwin->current_document && bfwin->current_document->view) {
		/ * the signal is also emitted when the toggle button gets it's initial value during the building of the window * /
		bluefish_text_view_rescan(BLUEFISH_TEXT_VIEW(bfwin->current_document->view));
	}
}*/

#endif /*HAVE_LIBENCHANT*/