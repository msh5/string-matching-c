#include <string.h>
#include <glib.h>

#include "naiveunicode.h"

static inline gboolean
UnicodeNaiveMatcher_hasPrefix(const char *string, const char *prefix)
{
    const char *s1 = string;
    const char *s2 = prefix;
    while (*s1 != '\0' && *s2 != '\0') {
        if (g_utf8_get_char(s1) != g_utf8_get_char(s2)) {
            return FALSE;
        }
        s1 = g_utf8_next_char(s1);
        s2 = g_utf8_next_char(s2);
    }
    if (*s2 != '\0') {
        return FALSE;
    }
    return TRUE;
}

gboolean
UnicodeNaiveMatcher_scan(const gchar *keyword, const gchar *document)
{
    const char *s1 = document;
    while (*s1 != '\0') {
        if (UnicodeNaiveMatcher_hasPrefix(s1, keyword)) {
            return TRUE;
        }
        s1 = g_utf8_next_char(s1);
    }
    return FALSE;
}

#ifdef __SSE2__

#include <emmintrin.h>
#include <sys/mman.h>

typedef gchar vi128[16] __attribute__((aligned(16)));
static vi128 s1v;
static vi128 s2v;
static vi128 result;
static vi128 zero ={0};

gboolean
UnicodeNaiveMatcher_scanWithSIMD(const gchar *keyword, const gchar *document)
{
    const char *s1 = document;
    const char *s1tbl[G_N_ELEMENTS(s1v)];
    gchar s2u = (gchar) g_utf8_get_char(keyword);
    for (int j=0; j<G_N_ELEMENTS(s2v); ++j) {
        s2v[j] = s2u;
    }
    __m128i s2m = _mm_load_si128((__m128i *) s2v);
    while (1) {
        int j=0;
        for (; j<G_N_ELEMENTS(s1v); ++j) {
            if (*s1 == '\0') {
                break;
            }
            s1tbl[j] = s1;
            s1v[j] = (gchar) g_utf8_get_char(s1);
            s1 = g_utf8_next_char(s1);
        }
        if (*s1 == '\0') {
            for (int k=0; k<j; ++k) {
                if (UnicodeNaiveMatcher_hasPrefix(s1tbl[k], keyword)) {
                    return TRUE;
                }
            }
            break;
        }
        __m128i s1m = _mm_load_si128((__m128i *) s1v);
        s1m = _mm_cmpeq_epi8(s1m, s2m);
        _mm_store_si128((__m128i *) result, s1m);
        if (0 != memcmp(result, zero, sizeof(zero))) {
            for (int k=0; k<G_N_ELEMENTS(result); ++k) {
                if (0 != result[k]) {
                    if (UnicodeNaiveMatcher_hasPrefix(s1tbl[k], keyword)) {
                        return TRUE;
                    }
                }
            }
        }
    }
    return FALSE;
}

#endif // __SSE2__
