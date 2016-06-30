// UTF-8 文字列が使えるが、バイトデータ的に偏りが出るとスキップが利きづらい

#ifndef __COMMENTZWALTER_H__
#define __COMMENTZWALTER_H__

#include <stdio.h>
#include <glib.h>

struct CommentzWalterMatcher;
typedef struct CommentzWalterMatcher CommentzWalterMatcher;

#ifdef __cplusplus
extern "C" {
#endif

extern CommentzWalterMatcher *CommentzWalterMatcher_new(gsize max_keyword_length);
extern void CommentzWalterMatcher_free(CommentzWalterMatcher *self);
extern void CommentzWalterMatcher_addKeyword(CommentzWalterMatcher *self, const gchar *keyword, glong length);
extern void CommentzWalterMatcher_compile(CommentzWalterMatcher *self);
extern void CommentzWalterMatcher_scan(CommentzWalterMatcher *self, const gchar *document, glong length, gconstpointer *output);

#ifdef DEBUG
extern void CommentzWalterMatcher_pprintTrie(CommentzWalterMatcher *self, FILE *ostream);
#endif

#ifdef __cplusplus
}
#endif

#endif // __COMMENTZWALTER_H__
