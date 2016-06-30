// 各文字を UTF-16 としてみるためスキップが利きやすいが、
// 前処理結果にハッシュテーブルを使う

#ifndef __COMMENTZWALTERUNICODE_H__
#define __COMMENTZWALTERUNICODE_H__

#include <stdio.h>
#include <glib.h>

struct UnicodeCommentzWalterMatcher;
typedef struct UnicodeCommentzWalterMatcher UnicodeCommentzWalterMatcher;

#ifdef __cplusplus
extern "C" {
#endif

extern UnicodeCommentzWalterMatcher *UnicodeCommentzWalterMatcher_new(gsize max_keyword_length);
extern void UnicodeCommentzWalterMatcher_free(UnicodeCommentzWalterMatcher *self);
extern void UnicodeCommentzWalterMatcher_compile(UnicodeCommentzWalterMatcher *self);
extern gboolean UnicodeCommentzWalterMatcher_addKeywordAsUTF8(UnicodeCommentzWalterMatcher *self, const gchar *keyword, glong length, GError **error);
extern void UnicodeCommentzWalterMatcher_addKeywordAsUTF16(UnicodeCommentzWalterMatcher* self, const gunichar2 *keyword, gsize length);
extern gboolean UnicodeCommentzWalterMatcher_scanUTF8String(UnicodeCommentzWalterMatcher *self, const gchar *document, glong length, gconstpointer *output, GError **error);
extern void UnicodeCommentzWalterMatcher_scanUTF16String(UnicodeCommentzWalterMatcher *self, const gunichar2 *document, gsize length, gconstpointer *output);
#ifdef DEBUG
extern void UnicodeCommentzWalterMatcher_pprintTrie(UnicodeCommentzWalterMatcher *self, FILE *ostream);
#endif

#ifdef __cplusplus
}
#endif

#endif // __COMMENTZWALTERUNICODE_H__


