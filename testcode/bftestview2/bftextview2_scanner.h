/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SCANNER_H_
#define _BFTEXTVIEW2_SCANNER_H_

#include "bftextview2.h"

Tcontext *get_context_at_position(BluefishTextView * bt2, GtkTextIter *position);
gboolean bftextview2_run_scanner(BluefishTextView * bt2);

#endif
