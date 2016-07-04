#ifndef __SUNDAY_H__
#define __SUNDAY_H__

#include <glib.h>

struct SundayMatcher;
typedef struct SundayMatcher SundayMatcher;

extern SundayMatcher * SundayMatcher_new(const gchar *pattern, glong patternlen);
extern void SundayMatcher_free(SundayMatcher *self);
extern void SundayMatcher_reinit(SundayMatcher *self, const gchar *pattern, glong patternlen);
extern gboolean SundayMatcher_scan(SundayMatcher *self, const gchar *text, gsize textlen);

#endif // __SUNDAY_H__
