#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "sunday.h"

struct SundayMatcher {
    gchar *pattern;
    gsize patternlen;
    gsize shifts[0x100];
};

SundayMatcher *
SundayMatcher_new(const gchar *pattern, glong patternlen)
{
    SundayMatcher *self = (SundayMatcher *) g_malloc0(sizeof(SundayMatcher));
    SundayMatcher_reinit(self, pattern, patternlen);
    return self;
}

void
SundayMatcher_free(SundayMatcher *self)
{
    g_free(self->pattern);
    g_free(self);
}

void
SundayMatcher_reinit(SundayMatcher *self, const gchar *pattern, glong patternlen)
{
    if (0L <= patternlen) {
        self->patternlen = patternlen;
    } else {
        self->patternlen = strlen(pattern);
    }
    g_free(self->pattern);
    self->pattern = g_strndup(pattern, self->patternlen);

    // bad character rule に基づくシフト表を作成する
    gsize *shifts_iter = self->shifts;
    gsize *shifts_end = shifts_iter + G_N_ELEMENTS(self->shifts);
    for (; shifts_end != shifts_iter; ++shifts_iter) {
        // シフト量の最大長はパターン長+1
        *shifts_iter = self->patternlen + 1;
    }
    const gchar *pattern_iter = self->pattern;
    const gchar *pattern_end = pattern_iter + self->patternlen;
    gsize shift = self->patternlen;
    for (; pattern_end != pattern_iter; ++pattern_iter, --shift) {
        self->shifts[(guchar) (*pattern_iter)] = shift;
    }
}

gboolean
SundayMatcher_scan(SundayMatcher *self, const gchar *text, gsize textlen)
{
    const gchar *textend = text + textlen;
    const gchar *text_iter = text + self->patternlen - 1;
  continue_scanning:;
    if (textend <= text_iter) {
        return FALSE;
    }
    const gchar *subtext_iter = text_iter;
    const gchar *pattern_end = self->pattern - 1;
    const gchar *pattern_iter = pattern_end + self->patternlen;
    for (; pattern_end != pattern_iter; --subtext_iter, --pattern_iter) {
        if (*subtext_iter != *pattern_iter) {
            // 比較開始位置のひとつ後ろの文字を使ってシフト量を求める
            if (textend == subtext_iter + 1) {
                return FALSE;
            }
            gchar subtext_char = *(subtext_iter + 1);
            text_iter += self->shifts[(guchar) subtext_char];
            goto continue_scanning;
        }
    }
    return TRUE;
}
