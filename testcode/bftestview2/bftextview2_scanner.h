/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SCANNER_H_
#define _BFTEXTVIEW2_SCANNER_H_

#include "bftextview2.h"
Tfoundstack *get_stackcache_next(BluefishTextView * bt2, GSequenceIter ** siter);
Tfoundstack *get_stackcache_at_position(BluefishTextView * bt2, GtkTextIter *position, GSequenceIter ** retsiter);
Tcontext *get_context_at_position(BluefishTextView * bt2, GtkTextIter *position);
gboolean bftextview2_run_scanner(BluefishTextView * bt2);

#endif
