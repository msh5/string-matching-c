#ifndef __BOYERMOOREUNICODE_H__
#define __BOYERMOOREUNICODE_H__

#include <glib.h>

typedef struct UnicodeBoyerMooreMatcher UnicodeBoyerMooreMatcher;

extern UnicodeBoyerMooreMatcher *UnicodeBoyerMooreMatcher_new(const gunichar2 *pattern, gsize patternlen);
extern void UnicodeBoyerMooreMatcher_free(UnicodeBoyerMooreMatcher *self);
extern const gunichar2 *UnicodeBoyerMooreMatcher_getPattern(UnicodeBoyerMooreMatcher *self);
extern gsize UnicodeBoyerMooreMatcher_getPatternLength(UnicodeBoyerMooreMatcher *self);
extern gboolean UnicodeBoyerMooreMatcher_scanUTF8String(UnicodeBoyerMooreMatcher *self,
                                                              const gchar *text, gsize textlen,
                                                              gboolean *match, GError **error);
extern gboolean UnicodeBoyerMooreMatcher_scanUTF8Channel(UnicodeBoyerMooreMatcher *self,
                                                                GIOChannel *text, gboolean *match,
                                                                GError **error);
extern void UnicodeBoyerMooreMatcher_scanUTF16String(UnicodeBoyerMooreMatcher *self,
                                                            const gunichar2 *text,
                                                            gsize textlen, gboolean *match);
extern gboolean UnicodeBoyerMooreMatcher_scanUTF16Channel(UnicodeBoyerMooreMatcher *self, GIOChannel *text,
                                                                 gboolean *match, GError **error);

#endif // __BOYERMOOREUNICODE_H__
