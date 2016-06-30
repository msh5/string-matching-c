#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "boyermooreunicode.h"

#define CHANNEL_READ_COUNT (4096)

struct UnicodeBoyerMooreMatcher {
    gunichar2 *pattern;
    gsize patternlen;
    guint16 bctable[0x10000]; /* bad character ruleに基づくシフト量テーブル */
    guint16 *gstable;         /* good suffix ruleに基づくシフト量テーブル */
    gchar *channelbuf;
    gunichar2 *textque;
};

/**
 * パターン文字列は UTF-16 に変換されていることを想定している
 */
UnicodeBoyerMooreMatcher *
UnicodeBoyerMooreMatcher_new(const gunichar2 *pattern, gsize patternlen)
{
    UnicodeBoyerMooreMatcher *self = (UnicodeBoyerMooreMatcher *) g_malloc(sizeof(UnicodeBoyerMooreMatcher));
    memset(self, 0, sizeof(UnicodeBoyerMooreMatcher));
    /* 指定されたパターン文字列をディープコピーする */
    self->pattern = (gunichar2 *) g_malloc(sizeof(gunichar2) * patternlen);
    memcpy(self->pattern, pattern, sizeof(gunichar2) * patternlen);
    self->patternlen = patternlen;
    memset(self->bctable, 0, sizeof(guint16) * 0x10000);
    self->gstable = (guint16 *) g_malloc(sizeof(guint16) * patternlen);
    memset(self->gstable, 0, sizeof(guint16) * patternlen);
    /* テキストチャネルの検査でしか必要ないバッファなので遅延確保することにする */
    self->channelbuf = NULL;
    self->textque = (gunichar2 *) g_malloc(sizeof(gunichar2) * patternlen);

    /* bad character ruleに基づいて、シフト量を計算する */
    g_assert(patternlen <= G_MAXUINT16);
    for (guint i = 0; i < 0x10000; ++i) {
        self->bctable[i] = patternlen;
    }
    for (guint i = 0; i < patternlen; ++i) {
        self->bctable[pattern[i]] = patternlen - i - 1;
    }

    /* good suffix ruleに基づいて、シフト量を計算する */
    self->gstable[patternlen - 1] = 1;
    int matchidx = patternlen - 1;
    for (int i = patternlen - 2; i >= 0; --i) {
        gint suffixlen = patternlen - (i + 1);
        if (suffixlen > 0 && memcmp(pattern, pattern + i + 1, sizeof(gunichar2) * suffixlen) == 0) {
            matchidx = i;
        }
        self->gstable[i] = patternlen + matchidx - i;

        for (int j = i; j >= 0; --j) {
            if (memcmp(pattern + j + 1, pattern + i + 1, sizeof(gunichar2) * suffixlen)
                != 0) {
                continue;
            }
            if (pattern[i] == pattern[j]) {
                continue;
            }
            self->gstable[i] = patternlen - j - 1;
            break;
        }
    }

    return self;
}

void
UnicodeBoyerMooreMatcher_free(UnicodeBoyerMooreMatcher *self)
{
    if (self == NULL) {
        return;
    }
    g_free(self->textque);
    g_free(self->channelbuf);
    g_free(self->gstable);
    g_free(self->pattern);
    g_free(self);
}

const gunichar2 *
UnicodeBoyerMooreMatcher_getPattern(UnicodeBoyerMooreMatcher *self)
{
    return self->pattern;
}

gsize
UnicodeBoyerMooreMatcher_getPatternLength(UnicodeBoyerMooreMatcher *self)
{
    return self->patternlen;
}

static void
UnicodeBoyerMooreMatcher_scanUTF8TextImpl(UnicodeBoyerMooreMatcher *self, const gchar *text,
                                                gsize textlen, gboolean *match)
{
    g_assert(NULL != match);

    /* テキスト長が0の場合は不一致とする */
    if (textlen == 0) {
        *match = FALSE;
        return;
    }
    const gunichar2 *p = self->pattern;
    const gchar *t = text;
    while (TRUE) {
        /* キューの文字列と完全一致するか確認する */
        gint i = self->patternlen - 1;
        const gunichar2 *pp = p + i;
        const gchar *tt = t;
        for (int i=0; i<self->patternlen - 1; ++i) {
            tt = g_utf8_next_char(tt);
        }
        gunichar2 tchar;
        for (; pp >= p; --pp) {
            tchar = g_utf8_get_char(tt);
            if (*pp != tchar) {
                break;
            }
            tt = g_utf8_prev_char(tt);
        }
        if (pp < p) {
            *match = TRUE;
            return;
        }

        /* シフト量を取得する */
        gint bcshift = self->bctable[tchar];
        gint gsshift = self->gstable[pp - p];
        gint shift = (pp - p) - self->patternlen + 1 + MAX(bcshift, gsshift);
        if (shift <= 0) {
            shift = 1;
        }

        /* キューの文字列を入れ替える */
        for (int i=0; i<shift; ++i) {
            t = g_utf8_next_char(t);
            if ('\0' == *t) {
                *match = FALSE;
                return;
            }
        }
    }
}

/**
 * パターン文字列が UTF-8 テキストに含まれるかを検査する
 * 検査結果を match に代入する
 */
gboolean
UnicodeBoyerMooreMatcher_scanUTF8String(UnicodeBoyerMooreMatcher *self, const gchar *text,
                                             gsize textlen, gboolean *match, GError **error)
{
    g_assert(NULL != match);
    UnicodeBoyerMooreMatcher_scanUTF8TextImpl(self, text, textlen, match);
    return TRUE;
}

/**
 * パターン文字列がチャネル内の UTF-8 テキストに含まれるかを検査する
 * 検査結果を match に代入する
 */
gboolean
UnicodeBoyerMooreMatcher_scanUTF8Channel(UnicodeBoyerMooreMatcher *self, GIOChannel *text,
                                               gboolean *match, GError **error)
{
    g_assert(NULL != match);

    if (self->channelbuf == NULL) {
        self->channelbuf = (gchar *) g_malloc(sizeof(gchar) * CHANNEL_READ_COUNT);
    }
    memset(self->textque, 0xFF, sizeof(gunichar2) * self->patternlen);

    gchar *bufhead = self->channelbuf;
    gchar *buftail = self->channelbuf + CHANNEL_READ_COUNT;
    gchar *readtail = bufhead;

    while (TRUE) {
        /* チャネル内のテキストをバッファに読み出す */
        gsize bytes_read = 0;
        GError *read_error = NULL;
        gboolean read_stat =
            g_io_channel_read_chars(text, readtail, buftail - readtail, &bytes_read, &read_error);
        if (G_IO_STATUS_ERROR == read_stat) {
            g_propagate_error(error, read_error);
            return FALSE;
        }
        if (G_IO_STATUS_NORMAL != read_stat || 0 == bytes_read) {
            /* もしバッファにバイト列が残っていたとしても、
             * 追加のバイト列なしでは UTF-8 として無効なバイト列なので破棄してよい */
            break;
        }
        readtail += bytes_read;

        /* bufhead の全バイトが有効なら TRUE を返すが、
         * 中途半端な切れ方をしている可能性があるので、返値は参考にしない */
        const gchar *validtail = bufhead;
        (void) g_utf8_validate(bufhead, readtail - bufhead, &validtail);
        if (bufhead >= validtail) {
            /* 有効な文字が1文字も含まれていない */
            break;
        }

        /* チャネルから取り出した範囲で、検索文字列を照合する
         * なお、今回取り出した範囲と次回取り出す範囲にまたがる部分を比較するために、
         * textque はクリアしてはいけない */
        UnicodeBoyerMooreMatcher_scanUTF8TextImpl(self, bufhead, validtail - bufhead, match);
        if (*match) {
            break;
        }

        /* バッファの状態とアドレス情報を再設定 */
        ptrdiff_t leftlen = readtail - validtail;
        g_assert(leftlen >= 0);
        if (leftlen != 0) {
            g_memmove(bufhead, validtail, leftlen);
        }
        readtail = bufhead + leftlen;
    }

    return TRUE;
}

static void
UnicodeBoyerMooreMatcher_scanUTF16StringImpl(UnicodeBoyerMooreMatcher *self, const gunichar2 *text,
                                                   gsize textlen, gboolean *match)
{
    g_assert(NULL != match);

    /* テキスト長が0の場合は不一致とする */
    if (textlen == 0) {
        *match = FALSE;
        return;
    }

    const gunichar2 *p = self->pattern;
    const gunichar2 *t = text;
    const gunichar2 *tend = text + textlen;
    while (TRUE) {
        /* キューの文字列と完全一致するか確認する */
        gint i = self->patternlen - 1;
        const gunichar2 *pp = p + i;
        const gunichar2 *tt = t + i;
        for (; pp >= p; --pp, --tt) {
            if (*pp != *tt) {
                break;
            }
        }
        if (pp < p) {
            *match = TRUE;
            return;
        }

        /* シフト量を取得する */
        gint bcshift = self->bctable[*tt];
        gint gsshift = self->gstable[pp - p];
        gint shift = (pp - p) - self->patternlen + 1 + MAX(bcshift, gsshift);
        if (shift <= 0) {
            shift = 1;
        }

        /* キューの文字列を入れ替える */
        t += shift;
        if (t >= tend) {
            *match = FALSE;
            return;
        }
    }
}

/**
 * パターン文字列が UTF-16 テキストに含まれるかを検査する
 * 検査結果を match に代入する
 */
void
UnicodeBoyerMooreMatcher_scanUTF16String(UnicodeBoyerMooreMatcher *self, const gunichar2 *text,
                                              gsize textlen, gboolean *match)
{
    g_assert(NULL != match);

    memset(self->textque, 0xFF, sizeof(gunichar2) * self->patternlen);
    UnicodeBoyerMooreMatcher_scanUTF16StringImpl(self, text, textlen, match);
}

/**
 * パターン文字列がチャネル内の UTF-16 テキストに含まれるかを検査する
 * 検査結果を match に代入する
 */
gboolean
UnicodeBoyerMooreMatcher_scanUTF16Channel(UnicodeBoyerMooreMatcher *self, GIOChannel *text,
                                                gboolean *match, GError **error)
{
    g_assert(NULL != match);

    if (NULL == self->channelbuf) {
        self->channelbuf = (gchar *) g_malloc(sizeof(gchar) * CHANNEL_READ_COUNT);
    }
    memset(self->textque, 0xFF, sizeof(gunichar2) * self->patternlen);

    while (TRUE) {
        /* チャネル内のテキストをバッファに読み出す */
        G_STATIC_ASSERT(0 == CHANNEL_READ_COUNT % 2);
        gsize valid_size = 0;
        GError *read_error = NULL;
        gboolean read_stat =
            g_io_channel_read_chars(text, self->channelbuf, CHANNEL_READ_COUNT, &valid_size,
                                    &read_error);
        if (G_IO_STATUS_ERROR == read_stat) {
            g_propagate_error(error, read_error);
            return FALSE;
        }
        if (G_IO_STATUS_NORMAL != read_stat || 0 == valid_size) {
            break;
        }

        /* チャネルから取り出した範囲で、検索文字列を照合する
         * なお、今回取り出した範囲と次回取り出す範囲にまたがる部分を比較するために、
         * textque はクリアしてはいけない */
        UnicodeBoyerMooreMatcher_scanUTF16StringImpl(self, (const gunichar2 *) self->channelbuf,
                                                    valid_size / 2, match);
        if (*match) {
            break;
        }
    }

    return TRUE;
}
