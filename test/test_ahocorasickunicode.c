#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/ahocorasickunicode.h"

void test0() {
  UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(16);
  static const char *patterns[] = {"abcd", "abc", "bcd", "ab", "bc", "cd", "a", "b", "c", "d",
                                   "xbcd", "axcd", "abxd", "abcx", "xbc", "axc", "abx",
                                   NULL};
  for (const char **patterns_iter = patterns; NULL != *patterns_iter; ++patterns_iter) {
    assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, *patterns_iter, -1L, NULL));
  }
  UnicodeAhoCorasickMatcher_pprintAutomaton(matcher, stdout);
  UnicodeAhoCorasickPatternsIter *iter = NULL;
  assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, "abcd", -1L, &iter, NULL));
  assert(NULL != iter);
  assert(0 == strcmp("a", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("ab", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("b", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("abc", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("bc", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("c", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("abcd", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("bcd", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("cd", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("d", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(NULL == UnicodeAhoCorasickPatternsIter_next(iter));
  UnicodeAhoCorasickPatternsIter_free(iter);
  UnicodeAhoCorasickMatcher_free(matcher);
}

void test1() {
  UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(16);
  static const char *patterns[] = {"あいうえ", "あいう", "いうえ", "あい", "いう", "うえ", "あ", "い", "う", "え",
                                   "んいうえ", "あんうえ", "あいんえ", "あいうん", "んいう", "あんう", "あいん",
                                   NULL};
  for (const char **patterns_iter = patterns; NULL != *patterns_iter; ++patterns_iter) {
    assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, *patterns_iter, -1L, NULL));
  }
  UnicodeAhoCorasickMatcher_pprintAutomaton(matcher, stdout);
  UnicodeAhoCorasickPatternsIter *iter = NULL;
  assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, "あいうえ", -1L, &iter, NULL));
  assert(NULL != iter);
  assert(0 == strcmp("あ", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("あい", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("い", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("あいう", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("いう", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("う", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("あいうえ", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("いうえ", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("うえ", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(0 == strcmp("え", UnicodeAhoCorasickPatternsIter_next(iter)));
  assert(NULL == UnicodeAhoCorasickPatternsIter_next(iter));
  UnicodeAhoCorasickPatternsIter_free(iter);
  UnicodeAhoCorasickMatcher_free(matcher);
}

int main(int argc, char *argv[]) {
  test0();
  test1();
  return 0;
}

