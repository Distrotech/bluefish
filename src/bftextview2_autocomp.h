#ifndef _BFTEXTVIEW2_AUTOCOMP_H_
#define _BFTEXTVIEW2_AUTOCOMP_H_

#include "bftextview2.h"
gboolean acwin_check_keypress(BluefishTextView *btv, GdkEventKey *event);
void autocomp_run(BluefishTextView *btv, gboolean user_requested);

#endif /* _BFTEXTVIEW2_AUTOCOMP_H_ */
