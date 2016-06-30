#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "commentzwalterunicode.h"

typedef struct UnicodeCommentzWalterTrie {
  GHashTable *childs; // 要素は UnicodeCommentzWalterTrie
  gint shift1;
  gint shift2;
  gunichar2 *word;
  gsize wordlen;
  gconstpointer output;
} UnicodeCommentzWalterTrie;

struct UnicodeCommentzWalterMatcher {
  gsize max_keyword_length;
  gunichar2 *wordbuf;
  UnicodeCommentzWalterTrie *trie;
  gsize wmin;
  guint chars[0x10000];
  gboolean compiled;
};

static void UnicodeCommentzWalterTrie_free(gpointer self);

static UnicodeCommentzWalterTrie *
UnicodeCommentzWalterTrie_new()
{
  UnicodeCommentzWalterTrie *self = (UnicodeCommentzWalterTrie *) g_malloc0(sizeof(UnicodeCommentzWalterTrie));
  self->childs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, UnicodeCommentzWalterTrie_free);
  return self;
}

static void
UnicodeCommentzWalterTrie_free(gpointer data)
{
  UnicodeCommentzWalterTrie *self = (UnicodeCommentzWalterTrie *) data;
  g_hash_table_destroy(self->childs);
  g_free(self->word);
  g_free(self);
}

static void
UnicodeCommentzWalterTrie_addKeyword(UnicodeCommentzWalterTrie *self, const gunichar2 *keyword, glong keyword_length, gconstpointer output, gunichar2 *wordbuf)
{
  // 既に存在するノードをスキップする
  UnicodeCommentzWalterTrie *current_node = self;
  const gunichar2 *keyword_iter = keyword + keyword_length - 1;
  gunichar2 *wordbuf_iter = wordbuf;
  for (; keyword <= keyword_iter; --keyword_iter) {
    gpointer child_node = g_hash_table_lookup(current_node->childs, GINT_TO_POINTER(*keyword_iter));
    if (NULL == child_node) {
      break;
    }
    *wordbuf_iter = *keyword_iter;
    ++wordbuf_iter;
    current_node = (UnicodeCommentzWalterTrie *) child_node;
  }
  // keyword を表現するために必要なノードを追加する
  for (; keyword <= keyword_iter; --keyword_iter) {
    UnicodeCommentzWalterTrie *new_node = UnicodeCommentzWalterTrie_new();
    g_assert(g_hash_table_insert(current_node->childs, GINT_TO_POINTER(*keyword_iter), new_node));
    *wordbuf_iter = *keyword_iter;
    ++wordbuf_iter;
    new_node->wordlen = wordbuf_iter - wordbuf;
    new_node->word = g_memdup(wordbuf, sizeof(gunichar2) * new_node->wordlen);
    current_node = new_node;
  }
  // output を設定する
  current_node->output = output;
}

static void
UnicodeCommentzWalterTrie_updateShifts(UnicodeCommentzWalterTrie *self, const UnicodeCommentzWalterTrie *another, gint rel_depth)
{
  if (0 < rel_depth &&
      0 == memcmp(self->word, another->word + rel_depth, sizeof(gunichar2) * self->wordlen)) {
    self->shift1 = MIN(self->shift1, rel_depth);
    if (NULL != another->output) {
      self->shift2 = MIN(self->shift2, rel_depth);
    }
  }
    GHashTableIter childs_iter;
    g_hash_table_iter_init(&childs_iter, another->childs);
    gpointer child_node = NULL;
    while (g_hash_table_iter_next(&childs_iter, NULL, &child_node)) {
      UnicodeCommentzWalterTrie_updateShifts(self, child_node, rel_depth + 1);
    }
}

static void
UnicodeCommentzWalterTrie_compile(UnicodeCommentzWalterTrie *self, const UnicodeCommentzWalterTrie *root_node, gint wmin, gint father_shift2)
{
  if (root_node == self) {
    self->shift1 = 1;
    self->shift2 = wmin;
  } else {
    self->shift1 = wmin;
    self->shift2 = father_shift2;
    UnicodeCommentzWalterTrie_updateShifts(self, root_node, -(self->wordlen));
  }
  GHashTableIter childs_iter;
  g_hash_table_iter_init(&childs_iter, self->childs);
  gpointer child_node;
  while (g_hash_table_iter_next(&childs_iter, NULL, &child_node)) {
    UnicodeCommentzWalterTrie_compile((UnicodeCommentzWalterTrie *) child_node, root_node, wmin, self->shift2);
  }
}

static void
UnicodeCommentzWalterTrie_calcMinDepthForChar(UnicodeCommentzWalterTrie *self, guint *min_depths, guint limit_depth)
{
  if (limit_depth <= self->wordlen + 1) {
    return;
  }
  GHashTableIter childs_iter;
  g_hash_table_iter_init(&childs_iter, self->childs);
  gpointer child_label;
  gpointer child_node;
  while (g_hash_table_iter_next(&childs_iter, &child_label, &child_node)) {
    guint *min_depth = min_depths + GPOINTER_TO_INT(child_label);
    if (*min_depth > self->wordlen + 1) {
      *min_depth = self->wordlen + 1;
    }
    UnicodeCommentzWalterTrie_calcMinDepthForChar((UnicodeCommentzWalterTrie *) child_node, min_depths, limit_depth);
  }
}

#ifdef DEBUG

static void
UnicodeCommentzWalterTrie_pprint(UnicodeCommentzWalterTrie *self, gunichar2 label, int depth, FILE *ostream)
{
  for (int i = 0; i < depth; ++i) {
    fprintf(ostream, "  ");
  }
  gchar label_as_utf8[6] = {0};
  gint length = g_unichar_to_utf8(label, label_as_utf8);
  if (0 == depth) {
    length = 0;
  }
  gchar *word_as_u8 = NULL;
  if (NULL != self->word) {
    word_as_u8 = g_utf16_to_utf8(self->word, self->wordlen, NULL, NULL, NULL);
  }
  fprintf(ostream, "\"%.*s\": <%p> shift1=%d, shift2=%d, word=%s, wordlen=%ld, output=%p\n", length, label_as_utf8, self, self->shift1, self->shift2, word_as_u8, (long) self->wordlen, self->output);
  g_free(word_as_u8);

  GHashTableIter childs_iter;
  g_hash_table_iter_init(&childs_iter, self->childs);
  gpointer child_label = NULL;
  gpointer child_node = NULL;
  while (g_hash_table_iter_next(&childs_iter, &child_label, &child_node)) {
    UnicodeCommentzWalterTrie_pprint((UnicodeCommentzWalterTrie *) child_node, GPOINTER_TO_INT(child_label), depth + 1, ostream);
  }
}

#endif // DEBUG

UnicodeCommentzWalterMatcher *
UnicodeCommentzWalterMatcher_new(gsize max_keyword_length)
{
  UnicodeCommentzWalterMatcher *self = (UnicodeCommentzWalterMatcher *) g_malloc0(sizeof(UnicodeCommentzWalterMatcher));
  self->max_keyword_length = max_keyword_length;
  self->wordbuf = (gunichar2 *) g_malloc_n(max_keyword_length, sizeof(gunichar2));
  self->trie = UnicodeCommentzWalterTrie_new();
  self->wmin = max_keyword_length;
  self->compiled = FALSE;
  return self;
}

void
UnicodeCommentzWalterMatcher_free(UnicodeCommentzWalterMatcher *self)
{
  UnicodeCommentzWalterTrie_free(self->trie);
  g_free(self->wordbuf);
  g_free(self);
}

gboolean
UnicodeCommentzWalterMatcher_addKeywordAsUTF8(UnicodeCommentzWalterMatcher *self, const gchar *keyword, glong length, GError **error)
{
  glong length_as_u16 = 0L;
  GError *conv_error = NULL;
  gunichar2 *keyword_as_u16 = g_utf8_to_utf16(keyword, length, NULL, &length_as_u16, &conv_error);
  if (keyword_as_u16 == NULL) {
    g_propagate_error(error, conv_error);
    return FALSE;
  }
  g_assert(0L < length_as_u16);
  UnicodeCommentzWalterTrie_addKeyword(self->trie, keyword_as_u16, length_as_u16, keyword, self->wordbuf);
  g_free(keyword_as_u16);
  self->compiled = FALSE;
  if (self->wmin > length_as_u16) {
    self->wmin = length_as_u16;
  }
  return TRUE;
}

void
UnicodeCommentzWalterMatcher_addKeywordAsUTF16(UnicodeCommentzWalterMatcher *self, const gunichar2 *keyword, gsize length)
{
  UnicodeCommentzWalterTrie_addKeyword(self->trie, keyword, length, keyword, self->wordbuf);
  self->compiled = FALSE;
  if (self->wmin > length) {
    self->wmin = length;
  }
}

void
UnicodeCommentzWalterMatcher_compile(UnicodeCommentzWalterMatcher *self)
{
  if (!self->compiled) {
    UnicodeCommentzWalterTrie_compile(self->trie, self->trie, self->wmin, 0);
    guint *chars_iter = self->chars;
    guint *const chars_end = self->chars + G_N_ELEMENTS(self->chars);
    for (; chars_end != chars_iter; ++chars_iter) {
      *chars_iter = self->wmin + 1;
    }
    UnicodeCommentzWalterTrie_calcMinDepthForChar(self->trie, self->chars, self->wmin);
    self->compiled = TRUE;
  }
}

gboolean
UnicodeCommentzWalterMatcher_scanUTF8String(UnicodeCommentzWalterMatcher *self, const gchar *document, glong length, gconstpointer *output, GError **error)
{
  glong length_as_u16 = 0L;
  GError *conv_error = NULL;
  gunichar2 *document_as_u16 = g_utf8_to_utf16(document, length, NULL, &length_as_u16, &conv_error);
  if (NULL == document_as_u16) {
    g_propagate_error(error, conv_error);
    return FALSE;
  }
  g_assert(0L < length_as_u16);
  UnicodeCommentzWalterMatcher_scanUTF16String(self, document_as_u16, length_as_u16, output);
  g_free(document_as_u16);
  return TRUE;
}

void
UnicodeCommentzWalterMatcher_scanUTF16String(UnicodeCommentzWalterMatcher *self, const gunichar2 *document, gsize length, gconstpointer *output)
{
  UnicodeCommentzWalterMatcher_compile(self);

  const gunichar2 *document_start_iter = document + self->wmin - 1;
  const gunichar2 *const document_end = document + length;
  g_assert(NULL != output);
  while (TRUE) {
    const UnicodeCommentzWalterTrie *current_node = self->trie;
    const gunichar2 *document_iter = document_start_iter;
    gunichar2 label = 0;
    while (TRUE) {
      label = *document_iter;
      gpointer next_node = g_hash_table_lookup(current_node->childs, GINT_TO_POINTER(label));
      if (NULL == next_node) {
        break;
      }
      current_node = (const UnicodeCommentzWalterTrie *) next_node;
      if (NULL != current_node->output) {
        *output = current_node->output;
        return;
      }
      if (document == document_iter) {
        // XXX
        break;
      }
      --document_iter;
    }
    gint shift = MAX(current_node->shift1, self->chars[label] - (document_start_iter - document_iter) - 1);
    document_start_iter += MIN(shift, current_node->shift2);
    if (document_end <= document_start_iter) {
      *output = NULL;
      return;
    }
  }
}

#ifdef DEBUG

void
UnicodeCommentzWalterMatcher_pprintTrie(UnicodeCommentzWalterMatcher *self, FILE *ostream)
{
  UnicodeCommentzWalterMatcher_compile(self);
  UnicodeCommentzWalterTrie_pprint(self->trie, ' ', 0, ostream);
}

#endif // DEBUG

