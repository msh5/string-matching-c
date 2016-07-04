#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <glib.h>

#include "../src/ahocorasickunicode.h"
#include "../src/boyermoore.h"
#include "../src/boyermooreunicode.h"
#include "../src/commentzwalter.h"
#include "../src/commentzwalterunicode.h"
#include "../src/naiveunicode.h"
#include "../src/sunday.h"

// 大文字小文字は区別しない、正規化しない
// サンプルデータは UTF-8 である必要がある

#define DOCUMENT_SIZE (1024 * 1024)
#define MAX_KEYWORD_LENGTH (1024)
#define KEYWORD_SIZE (64)
#define KEYWORD_ALLOC_SIZE (KEYWORD_SIZE + 6)

typedef void (*bench_impl_func)(const char *, const char *, size_t, size_t, double *, long *);

struct bench_entry_t {
    const char *label;
    bench_impl_func bench_impl;
};

static void bench_ac_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_cw(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_cw_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_bm(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_bm_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_sunday(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_naive_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);
static void bench_naive_unicode_with_simd(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits);

static struct bench_entry_t bench_entries[] = {
        {"Aho-Corasick   ", bench_ac_unicode},
        {"Commentz-Walter", bench_cw_unicode},
        {"Boyer-Moore    ", bench_bm},
        {"Sunday         ", bench_sunday},
        {"naive          ", bench_naive_unicode},
        {NULL, NULL},
};

static void
bench_ac_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(MAX_KEYWORD_LENGTH);
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    struct timeval tv_before;
    g_assert(0 == gettimeofday(&tv_before, NULL));
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE) {
        g_assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, keyword, -1L, NULL));
    }
    struct timeval tv_after;
    g_assert(0 == gettimeofday(&tv_after, NULL));
    if (NULL != build_time) {
        *build_time += tv_after.tv_sec - tv_before.tv_sec;
        *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
    }
    for (int j=0; j<n_scanning; ++j) {
        UnicodeAhoCorasickPatternsIter *iter = NULL;
        g_assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, document, -1L, &iter, NULL));
        if (NULL != UnicodeAhoCorasickPatternsIter_next(iter) && NULL != n_hits) {
            ++(*n_hits);
        }
        UnicodeAhoCorasickPatternsIter_free(iter);
    }
    UnicodeAhoCorasickMatcher_free(matcher);
}

static void
bench_cw(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    CommentzWalterMatcher *matcher = CommentzWalterMatcher_new(MAX_KEYWORD_LENGTH);
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    struct timeval tv_before;
    g_assert(0 == gettimeofday(&tv_before, NULL));
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE) {
        CommentzWalterMatcher_addKeyword(matcher, keyword, -1L);
    }
    CommentzWalterMatcher_compile(matcher);
    struct timeval tv_after;
    g_assert(0 == gettimeofday(&tv_after, NULL));
    if (NULL != build_time) {
        *build_time += tv_after.tv_sec - tv_before.tv_sec;
        *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
    }
    for (int j=0; j<n_scanning; ++j) {
        gconstpointer output = NULL;
        CommentzWalterMatcher_scan(matcher, document, -1L, &output);
        if (NULL != output && NULL != n_hits) {
            ++(*n_hits);
        }
    }
    CommentzWalterMatcher_free(matcher);
}

static void
bench_cw_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    UnicodeCommentzWalterMatcher *matcher = UnicodeCommentzWalterMatcher_new(MAX_KEYWORD_LENGTH);
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    struct timeval tv_before;
    g_assert(0 == gettimeofday(&tv_before, NULL));
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE) {
        g_assert(UnicodeCommentzWalterMatcher_addKeywordAsUTF8(matcher, keyword, -1L, NULL));
    }
    UnicodeCommentzWalterMatcher_compile(matcher);
    struct timeval tv_after;
    g_assert(0 == gettimeofday(&tv_after, NULL));
    if (NULL != build_time) {
        *build_time += tv_after.tv_sec - tv_before.tv_sec;
        *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
    }
    for (int j=0; j<n_scanning; ++j) {
        gconstpointer output = NULL;
        g_assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, document, -1L, &output, NULL));
        if (NULL != output && NULL != n_hits) {
            ++(*n_hits);
        }
    }
    UnicodeCommentzWalterMatcher_free(matcher);
}

static void
bench_bm(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    struct timeval tv_before;
    struct timeval tv_after;
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    BoyerMooreMatcher *matcher = BoyerMooreMatcher_new("dummy");
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE ) {
        g_assert(0 == gettimeofday(&tv_before, NULL));
        BoyerMooreMatcher_updatePattern(matcher, keyword);
        g_assert(0 == gettimeofday(&tv_after, NULL));
        if (NULL != build_time) {
            *build_time += tv_after.tv_sec - tv_before.tv_sec;
            *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
        }
        for (int j=0; j<n_scanning; ++j) {
            if (BoyerMooreMatcher_scan(matcher, document, FALSE)) {
                if (NULL != n_hits) {
                    ++(*n_hits);
                }
            }
        }
    }
    BoyerMooreMatcher_free(matcher);
}

static void
bench_bm_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    struct timeval tv_before;
    struct timeval tv_after;
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE) {
        g_assert(0 == gettimeofday(&tv_before, NULL));
        glong length_as_u16 = 0L;
        gunichar2 *keyword_as_u16 = g_utf8_to_utf16(keyword, -1L, NULL, &length_as_u16, NULL);
        g_assert(NULL != keyword_as_u16);
        UnicodeBoyerMooreMatcher *matcher = UnicodeBoyerMooreMatcher_new(keyword_as_u16, length_as_u16);
        g_assert(0 == gettimeofday(&tv_after, NULL));
        if (NULL != build_time) {
            *build_time += tv_after.tv_sec - tv_before.tv_sec;
            *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
        }
        for (size_t j=0; j<n_scanning; ++j) {
            gboolean matched;
            g_assert(UnicodeBoyerMooreMatcher_scanUTF8String(matcher, document, -1L, &matched, NULL));
            if (matched && NULL != n_hits) {
                ++(*n_hits);
            }
        }
        UnicodeBoyerMooreMatcher_free(matcher);
        g_free(keyword_as_u16);
    }
}

static void
bench_sunday(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    struct timeval tv_before;
    struct timeval tv_after;
    gsize documentlen = strlen(document);
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    SundayMatcher *matcher = SundayMatcher_new("dummy", -1L);
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE ) {
        g_assert(0 == gettimeofday(&tv_before, NULL));
        SundayMatcher_reinit(matcher, keyword, -1L);
        g_assert(0 == gettimeofday(&tv_after, NULL));
        if (NULL != build_time) {
            *build_time += tv_after.tv_sec - tv_before.tv_sec;
            *build_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
        }
        for (int j=0; j<n_scanning; ++j) {
            if (SundayMatcher_scan(matcher, document, documentlen)) {
                if (NULL != n_hits) {
                    ++(*n_hits);
                }
            }
        }
    }
    SundayMatcher_free(matcher);
}

static void
bench_naive_unicode(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    for (; keyword_end != keyword; keyword += KEYWORD_ALLOC_SIZE) {
        for (size_t i=0; i<n_scanning; ++i) {
            if (UnicodeNaiveMatcher_scan(keyword, document)) {
                if (NULL != n_hits) {
                    ++(*n_hits);
                }
            }
        }
    }
}

static void
bench_naive_unicode_with_simd(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, double *build_time, long *n_hits)
{
    const char *keyword = keywords;
    const char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
    for (; keyword < keyword_end; keyword += KEYWORD_ALLOC_SIZE) {
        for (size_t i=0; i<n_scanning; ++i) {
            if (UnicodeNaiveMatcher_scanWithSIMD(keyword, document)) {
                if (NULL != n_hits) {
                    ++(*n_hits);
                }
            }
        }
    }
}

static size_t
rand_utf8_text(size_t size, char *outbuf)
{
    size_t real_size = 0;
    while (real_size < size) {
        gunichar rand_char;
        do {
            rand_char = rand() + rand();
            rand_char %= 0x2fa1f;
        } while (!g_unichar_validate(rand_char));
        real_size += g_unichar_to_utf8(rand_char, outbuf + real_size);
    }
    outbuf[real_size] = '\0';
    g_assert(g_utf8_validate(outbuf, -1, NULL));
    return real_size;
}

static void
measure(const char *document, const char *keywords, size_t n_keywords, size_t n_scanning, const char *label, bench_impl_func bench_impl, double *build_time, double *total_time, long *n_hits)
{
    struct timeval tv_before;
    g_assert(0 == gettimeofday(&tv_before, NULL));
    bench_impl(document, keywords, n_keywords, n_scanning, build_time, n_hits);
    struct timeval tv_after;
    g_assert(0 == gettimeofday(&tv_after, NULL));
    if (NULL != total_time) {
        *total_time += tv_after.tv_sec - tv_before.tv_sec;
        *total_time += (tv_after.tv_usec -tv_before.tv_usec) * 0.000001;
    }
    //printf("[%s]:\ttime=%lf,\tn_hits=%ld\n", label, elapsed_sec, n_hits);
}

int
main(int argc, char *argv[])
{
    g_assert(4 == argc);
    size_t n_tests = (size_t) atoi(argv[1]);
    size_t n_keywords = (size_t) atoi(argv[2]);
    size_t n_scanning = (size_t) atoi(argv[3]);
    srand(time(NULL));
    double build_times[G_N_ELEMENTS(bench_entries)];
    double total_times[G_N_ELEMENTS(bench_entries)];
    for (int i=0; i<G_N_ELEMENTS(bench_entries); ++i) {
        build_times[i] = 0.0;
        total_times[i] = 0.0;
    }
    char *document = (char *) malloc(sizeof(char) * (DOCUMENT_SIZE + 6));
    char *keywords = (char *) malloc(sizeof(char) * KEYWORD_ALLOC_SIZE * n_keywords);
    for (int i=0; i<n_tests; ++i) {
        rand_utf8_text(DOCUMENT_SIZE, document);
        char *keyword = keywords;
        char *keyword_end = keywords + KEYWORD_ALLOC_SIZE * n_keywords;
        for (; keyword < keyword_end; keyword += KEYWORD_ALLOC_SIZE) {
            rand_utf8_text(KEYWORD_SIZE, keyword);
        }
        struct bench_entry_t *bench_entry = bench_entries;
        double *build_time = build_times;
        double *total_time = total_times;
        for (; bench_entry->label != NULL; ++bench_entry, ++build_time, ++total_time) {
            measure(document, keywords, n_keywords, n_scanning, bench_entry->label, bench_entry->bench_impl, build_time, total_time, NULL);
        }
    }
    free(keywords);
    free(document);
    struct bench_entry_t *bench_entry = bench_entries;
    double *build_time = build_times;
    double *total_time = total_times;
    for (; bench_entry->label != NULL; ++bench_entry, ++build_time, ++total_time) {
        double build_time_average = *build_time / (double) n_tests;
        double total_time_average = *total_time / (double) n_tests;
        double search_time_average = total_time_average - build_time_average;
        printf("[%s]:\t%lf,\t%lf,\t%lf\n", bench_entry->label, build_time_average, search_time_average, total_time_average);
    }
    return 0;
}
