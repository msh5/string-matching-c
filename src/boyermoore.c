#include <stdio.h>
#include <string.h>
#include <glib.h>

typedef struct BoyerMooreMatcher {
    const gchar *pat;
    guint16 patlen;
    guint16 delta1[G_MAXUINT8];
    guint16 *delta2;
} BoyerMooreMatcher;

static void
BoyerMooreMatcher_buildShiftLengthTable(BoyerMooreMatcher *self) {
    // bad character rule に基づいて計算
    for (int i = 0; i < G_MAXUINT8; ++i) {
        self->delta1[i] = self->patlen;
    }
    for (int i = 0; i < self->patlen; ++i) {
        self->delta1[(guint16) self->pat[i]] = self->patlen - i - 1;
    }

    // good suffix rule に基づいて計算
    self->delta2[self->patlen - 1] = 1;
    int matchidx = self->patlen - 1;
    for (int i = self->patlen - 2; i >= 0; --i) {
        // プレフィクスとサフィックスの一致を確認
        if ((self->patlen - (i + 1) > 0) &&
            strncmp(self->pat, self->pat + i + 1, self->patlen - (i + 1)) == 0) {
            matchidx = i;
        }
        self->delta2[i] = self->patlen + matchidx - i;

        for (int j = i; j >= 0; --j) {
            // 部分文字列とサフィックスの一致を確認
            if (strncmp(self->pat + i + 1, self->pat + j + 1, self->patlen - (i + 1)) != 0) {
                continue;
            }
            // サフィックスの手前の文字との不一致を確認
            if (self->pat[i] == self->pat[j]) {
                continue;
            }
            self->delta2[i] = self->patlen - j - 1;
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
    memset(self->delta1, 0, sizeof(self->delta1));
    self->delta2 = g_malloc(sizeof(guint16) * self->patlen);
    memset(self->delta2, 0, sizeof(sizeof(guint16) * self->patlen));
    BoyerMooreMatcher_buildShiftLengthTable(self);
    return self;
}

void
BoyerMooreMatcher_free(BoyerMooreMatcher *self) {
    if (self == NULL) {
        return;
    }
    g_free(self->delta2);
    g_free(self);
}

void
BoyerMooreMatcher_updatePattern(BoyerMooreMatcher *self, const gchar *pat) {
    self->pat = pat;
    self->patlen = strlen(pat);
    memset(self->delta1, 0, sizeof(self->delta1));
    self->delta2 = g_realloc(self->delta2, sizeof(guint16) * self->patlen);
    memset(self->delta2, 0, sizeof(sizeof(guint16) * self->patlen));
    BoyerMooreMatcher_buildShiftLengthTable(self);
}

gboolean
BoyerMooreMatcher_scan(BoyerMooreMatcher *self, const gchar *string, gboolean verbose) {
    if (verbose) {
        printf("== START SEARCHING ==\n");
        printf("pat: %s\n", self->pat);
        printf("string: %s\n\n", string);
    }

    int totalshift = 0;
    int patlen = self->patlen;
    int stringlen = strlen(string);
    int count = 0;
    long shiftall = 0L;
    while (TRUE) {
        if (totalshift + patlen > stringlen) {
            break;
        }

        if (verbose) {
            printf("== TEST ==\n");
            printf("%s\n", string);
            for (int i = 0; i <totalshift; ++i) {
                printf(" ");
            }
            printf("%s\n\n", self->pat);
        }

        /* check matching */
        gboolean match = TRUE;
        int j;
        for (j = patlen - 1; j >= 0; --j) {
            ++count;
            if (string[totalshift + j] != self->pat[j]) {
                match = FALSE;
                break;
            }
        }
        if (match) {
            if (verbose) {
                printf("== FINISH SEARCHING ==\n");
                printf("result: MATCH (shift: %d)\n\n", totalshift);
            }
            return TRUE;
        } else {
            guint16 key = *(string + totalshift + j);
            int shift = j + 1 - patlen + MAX(self->delta1[key], self->delta2[j]);
            shift = MAX(shift, 1);
            shiftall += shift;
            totalshift += shift;
        }
    }
    //printf("count: %d, shiftall: %ld\n", count, shiftall);

    if (verbose) {
        printf("== FINISH SEARCHING ==\n");
        printf("result: NOT MATCH \n\n");
    }
    return FALSE;
}
