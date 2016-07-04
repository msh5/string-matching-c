#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include "boyermoore.h"

/**
 * アルファベット文字列の検索をテストする
 */
static void
testAlphabetTextScan()
{
    static const gchar *text_tbl[] = {
        "abcde", "bcde", "abcd", "abde", "_bcde", "abcd_", "ab_de",
        "xyzabcdefgh", "xyzbcdefgh", "xyzabcdfgh", "xyzabdefgh",
        "abbacbcdabcdeacbd", "abbacbcdabcdabbde",
        NULL,
    };
    static gboolean expected_tbl[] = {
        TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        TRUE, FALSE, FALSE, FALSE,
        TRUE, FALSE,
    };
    BoyerMooreMatcher *matcher = BoyerMooreMatcher_new("abcde");
    const gchar **texts_iter = text_tbl;
    gboolean *expecteds_iter = expected_tbl;
    for (; NULL != *texts_iter; ++texts_iter, ++expecteds_iter) {
        assert(*expecteds_iter == BoyerMooreMatcher_scan(matcher, *texts_iter, FALSE));
    }
    BoyerMooreMatcher_free(matcher);
}

/**
 * 日本語文字列の検索をテストする
 */
static void
testJapaneseTextScan()
{
    static const gchar *text_tbl[] = {
        "インターネット",
        "ンターネット", "インターネッ", "インタネット", "インタアネット",
        "株式会社インターネットイニシアティブ",
        "株式会社ンターネットイニシアティブ", "株式会社インターネッイニシアティブ",
        "株式会社インタネットイニシアティブ", "株式会社インタアネットイニシアティブ",
        "イターネットインーネットインターネットインターットインターネト",
        "イターネットインーネットインタアネットインターットインターネト",
        NULL,
    };
    static int expected_tbl[] = {
        TRUE,
        FALSE, FALSE, FALSE, FALSE,
        TRUE,
        FALSE, FALSE,
        FALSE, FALSE,
        TRUE,
        FALSE
    };
    BoyerMooreMatcher *matcher = BoyerMooreMatcher_new("インターネット");
    const gchar **texts_iter = text_tbl;
    gboolean *expecteds_iter = expected_tbl;
    for (; NULL != *texts_iter; ++texts_iter, ++expecteds_iter) {
        assert(*expecteds_iter == BoyerMooreMatcher_scan(matcher, *texts_iter, FALSE));
    }
    BoyerMooreMatcher_free(matcher);
}

/**
 * 検索文字列およびテキストに非BMP文字が含まれる場合の検索をテストする
 */
static void
testNotBMPTextScan()
{
    static const gchar *text_tbl[] = {
        "𠮟・𠂉・𥻘・𨨩",
        "・𠂉・𥻘・𨨩", "𠮟・𠂉・𥻘・", "𠮟・𠂉𥻘・𨨩", "𠮟・𠂉鎼𥻘・𨨩",
        "驑・䮶・髝・𠮟・𠂉・𥻘・𨨩・䲂・䱿・鰸",
        "驑・䮶・髝・・𠂉・𥻘・𨨩・䲂・䱿・鰸", "驑・䮶・髝・・𠂉・𥻘・・䲂・䱿・鰸",
        "驑・䮶・髝・𠮟・𠂉𥻘・𨨩・䲂・䱿・鰸", "驑・䮶・髝・𠮟・𠂉鎼𥻘・𨨩・䲂・䱿・鰸",
        "𠮟𠂉・𥻘・𨨩𠮟・・𥻘・𨨩𠮟・𠂉・𥻘・𨨩𠮟・𠂉・・𨨩𠮟・𠂉・𥻘𨨩",
        "𠮟𠂉・𥻘・𨨩𠮟・・𥻘・𨨩𠮟・𠂉鎼𥻘・𨨩𠮟・𠂉・・𨨩𠮟・𠂉・𥻘𨨩",
        NULL,
    };
    static int expected_tbl[] = {
        TRUE,
        FALSE, FALSE, FALSE, FALSE,
        TRUE,
        FALSE, FALSE,
        FALSE, FALSE,
        TRUE,
        FALSE
    };
    BoyerMooreMatcher *matcher = BoyerMooreMatcher_new("𠮟・𠂉・𥻘・𨨩");
    const gchar **texts_iter = text_tbl;
    gboolean *expecteds_iter = expected_tbl;
    for (; NULL != *texts_iter; ++texts_iter, ++expecteds_iter) {
        assert(*expecteds_iter == BoyerMooreMatcher_scan(matcher, *texts_iter, FALSE));
    }
    BoyerMooreMatcher_free(matcher);
}

int
main(int argc, char *argv[])
{
    testAlphabetTextScan();
    testJapaneseTextScan();
    testNotBMPTextScan();
    return 0;
}
