#ifndef __BOYERMOORE_H__
#define __BOYERMOORE_H__

#include <glib.h>

struct BoyerMooreMatcher;
typedef struct BoyerMooreMatcher BoyerMooreMatcher;

extern BoyerMooreMatcher * BoyerMooreMatcher_new(const gchar *pat);
extern void BoyerMooreMatcher_free(BoyerMooreMatcher *self);
extern void BoyerMooreMatcher_updatePattern(BoyerMooreMatcher *self, const gchar *pat);
extern gboolean BoyerMooreMatcher_scan(BoyerMooreMatcher *self, const gchar *string, gboolean verbose);

#endif // __BOYERMOORE_H__
