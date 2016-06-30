#ifndef __AHOCORASICKUNICODE_H__
#define __AHOCORASICKUNICODE_H__

#include <stdio.h>
#include <glib.h>

/**
 * 複数パターンを定数時間でスキャン可能な文字列検索アルゴリズム
 */

struct UnicodeAhoCorasickMatcher;
typedef struct UnicodeAhoCorasickMatcher UnicodeAhoCorasickMatcher;
struct UnicodeAhoCorasickPatternsIter;
typedef struct UnicodeAhoCorasickPatternsIter UnicodeAhoCorasickPatternsIter;

#ifdef __cplusplus
extern "C" {
#endif

#define AHOCORASICKUNICODE_ERROR (g_quark_from_static_string("ahocorasickunicode-error-quark"))

typedef enum {
    AHOCORASICKUNICODE_ERROR_TOO_LONG_PATTERN,
} AhoCorasickUnicodeError;

extern UnicodeAhoCorasickMatcher *UnicodeAhoCorasickMatcher_new(gsize max_pattern_len);
extern void UnicodeAhoCorasickMatcher_free(UnicodeAhoCorasickMatcher *self);
extern gboolean UnicodeAhoCorasickMatcher_addKeywordAsUTF8(UnicodeAhoCorasickMatcher *self, const gchar *pattern, glong pattern_len, GError **error);
extern gboolean UnicodeAhoCorasickMatcher_addKeywordAsUTF16(UnicodeAhoCorasickMatcher *self, const gunichar2 *pattern, gsize pattern_len, GError **error);
extern gboolean UnicodeAhoCorasickMatcher_scanUTF8String(UnicodeAhoCorasickMatcher *self, const gchar *text, glong textlen, UnicodeAhoCorasickPatternsIter **iter, GError **error);
extern void UnicodeAhoCorasickMatcher_scanUTF16String(UnicodeAhoCorasickMatcher *self, const gunichar2 *text, gsize textlen, UnicodeAhoCorasickPatternsIter **iter);
#ifdef DEBUG
extern void UnicodeAhoCorasickMatcher_pprintAutomaton(UnicodeAhoCorasickMatcher *self, FILE *ostream);
#endif

extern void UnicodeAhoCorasickPatternsIter_free(UnicodeAhoCorasickPatternsIter *self);
extern gconstpointer UnicodeAhoCorasickPatternsIter_next(UnicodeAhoCorasickPatternsIter *self);

#ifdef __cplusplus
}
#endif

#endif // __AHOCORASICKUNICODE_H__

