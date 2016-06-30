#include <assert.h>
#include <stdio.h>

#include "../src/ahocorasickunicode.h"

void test0() {
  UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(16);
  static const char *patterns[] = {"ab", "cd", "bc", "de", NULL};
  for (const char **patterns_iter = patterns; NULL != *patterns_iter; ++patterns_iter) {
    assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, *patterns_iter, -1L, NULL));
  }
  UnicodeAhoCorasickMatcher_pprintAutomaton(matcher, stdout);
  UnicodeAhoCorasickPatternsIter *iter = NULL;
  assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, "abcde", -1L, &iter, NULL));
  assert(NULL != iter);
  assert(patterns[0] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[2] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[1] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[3] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(NULL == UnicodeAhoCorasickPatternsIter_next(iter));
  UnicodeAhoCorasickPatternsIter_free(iter);
  UnicodeAhoCorasickMatcher_free(matcher);
}

void test1() {
  UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(16);
  static const char *patterns[] = {"abcd", "cd", "bcd", "d", NULL};
  for (const char **patterns_iter = patterns; NULL != *patterns_iter; ++patterns_iter) {
    assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, *patterns_iter, -1L, NULL));
  }
  UnicodeAhoCorasickMatcher_pprintAutomaton(matcher, stdout);
  UnicodeAhoCorasickPatternsIter *iter = NULL;
  assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, "abcd", -1L, &iter, NULL));
  assert(NULL != iter);
  assert(patterns[0] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[2] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[1] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[3] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(NULL == UnicodeAhoCorasickPatternsIter_next(iter));
  UnicodeAhoCorasickPatternsIter_free(iter);
  UnicodeAhoCorasickMatcher_free(matcher);
}

void test2() {
  UnicodeAhoCorasickMatcher *matcher = UnicodeAhoCorasickMatcher_new(16);
  static const char *patterns[] = {"あいう", "うえお", "いうえ", NULL};
  for (const char **patterns_iter = patterns; NULL != *patterns_iter; ++patterns_iter) {
    assert(UnicodeAhoCorasickMatcher_addKeywordAsUTF8(matcher, *patterns_iter, -1L, NULL));
  }
  UnicodeAhoCorasickMatcher_pprintAutomaton(matcher, stdout);
  UnicodeAhoCorasickPatternsIter *iter = NULL;
  assert(UnicodeAhoCorasickMatcher_scanUTF8String(matcher, "あいうえお", -1L, &iter, NULL));
  assert(NULL != iter);
  assert(patterns[0] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[2] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(patterns[1] == UnicodeAhoCorasickPatternsIter_next(iter));
  assert(NULL == UnicodeAhoCorasickPatternsIter_next(iter));
  UnicodeAhoCorasickPatternsIter_free(iter);
  UnicodeAhoCorasickMatcher_free(matcher);
}

int main(int argc, char *argv[]) {
  test0();
  test1();
  test2();
  return 0;
}

