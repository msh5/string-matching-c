#include <assert.h>
#include <stdio.h>

#include "../src/commentzwalter.h"

void test0() {
  CommentzWalterMatcher *matcher = CommentzWalterMatcher_new(64);
  static const char *keywords[] = {"cacbaa", "acb", "aba", "acbab", "ccbab", NULL};
  for (const char **keywords_iter = keywords; NULL != *keywords_iter; ++keywords_iter) {
    CommentzWalterMatcher_addKeyword(matcher, *keywords_iter, -1L);
  }
  CommentzWalterMatcher_pprintTrie(matcher, stdout);
  gconstpointer output = NULL;
  CommentzWalterMatcher_scan(matcher, "acb", -1L, &output);
  assert(keywords[1] == output);
  CommentzWalterMatcher_scan(matcher, "aba", -1L, &output);
  assert(keywords[2] == output);
  CommentzWalterMatcher_scan(matcher, "ccbab", -1L, &output);
  assert(keywords[4] == output);
  CommentzWalterMatcher_scan(matcher, "ecbabbcacbaa", -1L, &output);
  assert(keywords[1] == output);
  CommentzWalterMatcher_scan(matcher, "ecbabbccbab", -1L, &output);
  assert(keywords[4] == output);
  CommentzWalterMatcher_scan(matcher, "ecbabbccbaa", -1L, &output);
  assert(NULL == output);
  CommentzWalterMatcher_free(matcher);
}

int main(int argc, char *argv[]) {
  test0();
  return 0;
}
