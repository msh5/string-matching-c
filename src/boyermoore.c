#include <stdio.h>
#include <string.h>
#include <glib.h>

typedef struct BoyerMooreMatcher {
    const gchar *pat;
    guint16 patlen;
    guint16 bcshifts[G_MAXUINT8];
    guint16 *gsshifts;
} BoyerMooreMatcher;

static void
BoyerMooreMatcher_buildShiftLengthTable(BoyerMooreMatcher *self) {
    // bad character rule に基づいて計算
    for (int i = 0; i < G_N_ELEMENTS(self->bcshifts); ++i) {
        self->bcshifts[i] = self->patlen;
    }
    for (int i = 0; i < self->patlen; ++i) {
        self->bcshifts[(guchar) self->pat[i]] = self->patlen - i - 1;
    }

    // good suffix rule に基づいて計算
    self->gsshifts[self->patlen - 1] = 1;
    int matchidx = self->patlen - 1;
    for (int i = self->patlen - 2; i >= 0; --i) {
        // プレフィクスとサフィックスの一致を確認
        if ((self->patlen - (i + 1) > 0) &&
            strncmp(self->pat, self->pat + i + 1, self->patlen - (i + 1)) == 0) {
            matchidx = i;
        }
        self->gsshifts[i] = self->patlen + matchidx - i;

        for (int j = i; j >= 0; --j) {
            // 部分文字列とサフィックスの一致を確認
            if (strncmp(self->pat + i + 1, self->pat + j + 1, self->patlen - (i + 1)) != 0) {
                continue;
            }
            // サフィックスの手前の文字との不一致を確認
            if (self->pat[i] == self->pat[j]) {
                continue;
            }
            self->gsshifts[i] = self->patlen - j - 1;
            break;
        }
    }
}

BoyerMooreMatcher *
BoyerMooreMatcher_new(const gchar *pat) {
    BoyerMooreMatcher *self = (BoyerMooreMatcher *) g_malloc(sizeof(BoyerMooreMatcher));
    memset(self, 0, sizeof(BoyerMooreMatcher));
    self->pat = pat;
    self->patlen = strlen(pat);
    memset(self->bcshifts, 0, sizeof(self->bcshifts));
    self->gsshifts = g_malloc(sizeof(guint16) * self->patlen);
    memset(self->gsshifts, 0, sizeof(sizeof(guint16) * self->patlen));
    BoyerMooreMatcher_buildShiftLengthTable(self);
    return self;
}

void
BoyerMooreMatcher_free(BoyerMooreMatcher *self) {
    if (self == NULL) {
        return;
    }
    g_free(self->gsshifts);
    g_free(self);
}

void
BoyerMooreMatcher_updatePattern(BoyerMooreMatcher *self, const gchar *pat) {
    self->pat = pat;
    self->patlen = strlen(pat);
    memset(self->bcshifts, 0, sizeof(self->bcshifts));
    self->gsshifts = g_realloc(self->gsshifts, sizeof(guint16) * self->patlen);
    memset(self->gsshifts, 0, sizeof(sizeof(guint16) * self->patlen));
    BoyerMooreMatcher_buildShiftLengthTable(self);
}

gboolean
BoyerMooreMatcher_scan(BoyerMooreMatcher *self, const gchar *string) {
    int totalshift = 0;
    int patlen = self->patlen;
    int stringlen = strlen(string);
    while (TRUE) {
        if (totalshift + patlen > stringlen) {
            break;
        }
        /* check matching */
        gboolean match = TRUE;
        int j;
        for (j = patlen - 1; j >= 0; --j) {
            if (string[totalshift + j] != self->pat[j]) {
                match = FALSE;
                break;
            }
        }
        if (match) {
            return TRUE;
        } else {
            gchar key = *(string + totalshift + j);
            int shift = j + 1 - patlen + MAX(self->bcshifts[(guchar) key], self->gsshifts[j]);
            shift = MAX(shift, 1);
            totalshift += shift;
        }
    }
    return FALSE;
}
