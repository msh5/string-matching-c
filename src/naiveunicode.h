#ifndef __NAIVEUNICODE_H__
#define __NAIVEUNICODE_H__

#include <glib.h>

extern gboolean UnicodeNaiveMatcher_scan(const gchar *keyword, const gchar *document);
#ifdef __SSE2__
extern gboolean UnicodeNaiveMatcher_scanWithSIMD(const gchar *keyword, const gchar *document);
#endif // __SSE2__

#endif // __NAIVEUNICODE_H__
