#include <assert.h>
#include <stdio.h>

#include "../src/commentzwalterunicode.h"

void test0() {
  UnicodeCommentzWalterMatcher *matcher = UnicodeCommentzWalterMatcher_new(64);
  static const char *keywords[] = {"cacbaa", "acb", "aba", "acbab", "ccbab", NULL};
  for (const char **keywords_iter = keywords; NULL != *keywords_iter; ++keywords_iter) {
    assert(UnicodeCommentzWalterMatcher_addKeywordAsUTF8(matcher, *keywords_iter, -1L, NULL));
  }
  UnicodeCommentzWalterMatcher_pprintTrie(matcher, stdout);
  gconstpointer output = NULL;
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "acb", -1L, &output, NULL));
  assert(keywords[1] == output);
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "aba", -1L, &output, NULL));
  assert(keywords[2] == output);
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "ccbab", -1L, &output, NULL));
  assert(keywords[4] == output);
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "ecbabbcacbaa", -1L, &output, NULL));
  assert(keywords[1] == output);
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "ecbabbccbab", -1L, &output, NULL));
  assert(keywords[4] == output);
  assert(UnicodeCommentzWalterMatcher_scanUTF8String(matcher, "ecbabbccbaa", -1L, &output, NULL));
  assert(NULL == output);
  UnicodeCommentzWalterMatcher_free(matcher);
}

int main(int argc, char *argv[]) {
  test0();
  return 0;
}
