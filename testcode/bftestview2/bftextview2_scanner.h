/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SCANNER_H_
#define _BFTEXTVIEW2_SCANNER_H_

#include "bftextview2.h"
Tfoundstack *get_stackcache_next(BluefishTextView * bt2, GSequenceIter ** siter);
Tfoundstack *get_stackcache_at_position(BluefishTextView * bt2, GtkTextIter *position, GSequenceIter ** retsiter);
guint16 get_context_and_startposition(BluefishTextView * bt2, GtkTextIter *position);
gboolean bftextview2_run_scanner(BluefishTextView * bt2);
void scan_for_prefix_start(BluefishTextView *btv, guint16 contextnum, GtkTextIter *start, GtkTextIter *cursor);
void scan_for_autocomp_prefix(BluefishTextView *btv,GtkTextIter *mstart,GtkTextIter *cursorpos,guint16 *contextnum);
void cleanup_scanner(BluefishTextView *btv);
#endif
