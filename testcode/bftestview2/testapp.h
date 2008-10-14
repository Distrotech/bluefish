#ifndef _TESTAPP_H_
#define _TESTAPP_H_
#include "bftextview2.h"

extern gboolean autoindent;
extern gboolean reduced_scan_triggers;
extern gboolean delay_full_scan;
extern gboolean smart_cursor;
extern gint autocomp_popup_mode;

void testapp_rescan_bflang(Tbflang *bflang);
#endif
