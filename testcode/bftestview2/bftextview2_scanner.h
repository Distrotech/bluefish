/* for the design docs see bftextview2.h */
#ifndef _BFTEXTVIEW2_SCANNER_H_
#define _BFTEXTVIEW2_SCANNER_H_

Tcontext *get_context_at_position(Tbftextview2 * bt2, GtkTextIter *position);
gboolean bftextview2_run_scanner(Tbftextview2 * bt2);

#endif
