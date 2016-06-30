#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "commentzwalter.h"

typedef struct CommentzWalterTrie CommentzWalterTrie;
struct CommentzWalterTrie {
  CommentzWalterTrie *childs[0x100];
  gint shift1;
  gint shift2;
  gchar *word;
  gsize wordlen;
  gconstpointer output;
};

struct CommentzWalterMatcher {
  gsize max_keyword_length;
  gchar *wordbuf;
  CommentzWalterTrie *trie;
  gsize wmin;
  guint chars[0x100];
  gboolean compiled;
};

static void CommentzWalterTrie_free(gpointer self);

static CommentzWalterTrie *
CommentzWalterTrie_new()
{
  CommentzWalterTrie *self = (CommentzWalterTrie *) g_malloc0(sizeof(CommentzWalterTrie));
  memset(self->childs, 0, sizeof(self->childs));
  return self;
}

static void
CommentzWalterTrie_free(gpointer data)
{
  if (NULL != data) {
      CommentzWalterTrie *self = (CommentzWalterTrie *) data;
      CommentzWalterTrie **childs_iter = self->childs;
      CommentzWalterTrie **childs_end = self->childs + G_N_ELEMENTS(self->childs);
      for (; childs_iter != childs_end; ++childs_iter) {
        CommentzWalterTrie_free(*childs_iter);
      }
      g_free(self->word);
      g_free(self);
  }
}

static void
CommentzWalterTrie_addKeyword(CommentzWalterTrie *self, const gchar *keyword, glong keyword_length, gchar *wordbuf)
{
  // 既に存在するノードをスキップする
  CommentzWalterTrie *current_node = self;
  const gchar *keyword_iter = keyword + keyword_length - 1;
  gchar *wordbuf_iter = wordbuf;
  for (; keyword <= keyword_iter; --keyword_iter) {
    CommentzWalterTrie *child_node = current_node->childs[(guchar) *keyword_iter];
    if (NULL == child_node) {
      break;
    }
    *wordbuf_iter = *keyword_iter;
    ++wordbuf_iter;
    current_node = child_node;
  }
  // keyword を表現するために必要なノードを追加する
  for (; keyword <= keyword_iter; --keyword_iter) {
    CommentzWalterTrie *new_node = CommentzWalterTrie_new();
    current_node->childs[(guchar) *keyword_iter] = new_node;
    *wordbuf_iter = *keyword_iter;
    ++wordbuf_iter;
    new_node->wordlen = wordbuf_iter - wordbuf;
    new_node->word = g_memdup(wordbuf, sizeof(gchar) * new_node->wordlen);
    current_node = new_node;
  }
  // output を設定する
  current_node->output = keyword;
}

static void
CommentzWalterTrie_updateShifts(CommentzWalterTrie *self, const CommentzWalterTrie *another, gint rel_depth)
{
  if (0 < rel_depth &&
      0 == memcmp(self->word, another->word + rel_depth, sizeof(gchar) * self->wordlen)) {
    self->shift1 = MIN(self->shift1, rel_depth);
    if (NULL != another->output) {
      self->shift2 = MIN(self->shift2, rel_depth);
    }
  }
  CommentzWalterTrie **childs_iter = (CommentzWalterTrie **) another->childs;
  CommentzWalterTrie **childs_end = childs_iter + G_N_ELEMENTS(another->childs);
  const CommentzWalterTrie *child_node = *childs_iter;
  for (; childs_iter != childs_end; ++childs_iter, child_node = *childs_iter) {
    if (NULL != child_node) {
      CommentzWalterTrie_updateShifts(self, child_node, rel_depth + 1);
    }
  }
}

static void
CommentzWalterTrie_compile(CommentzWalterTrie *self, const CommentzWalterTrie *root_node, gint wmin, gint father_shift2)
{
  if (root_node == self) {
    self->shift1 = 1;
    self->shift2 = wmin;
  } else {
    self->shift1 = wmin;
    self->shift2 = father_shift2;
    CommentzWalterTrie_updateShifts(self, root_node, -(self->wordlen));
  }
  CommentzWalterTrie **childs_iter = self->childs;
  CommentzWalterTrie **childs_end = self->childs + G_N_ELEMENTS(self->childs);
  CommentzWalterTrie *child_node = *childs_iter;
  for (; childs_iter != childs_end; ++childs_iter, child_node = *childs_iter) {
    if (NULL != child_node) {
      CommentzWalterTrie_compile(child_node, root_node, wmin, self->shift2);
    }
  }
}

static void
CommentzWalterTrie_calcMinDepthForChar(CommentzWalterTrie *self, guint *min_depths, guint limit_depth)
{
  if (limit_depth <= self->wordlen + 1) {
    return;
  }
  CommentzWalterTrie **childs_iter = self->childs;
  CommentzWalterTrie **childs_end = self->childs + G_N_ELEMENTS(self->childs);
  CommentzWalterTrie *child_node = *childs_iter;
  guint *min_depth = min_depths;
  for (; childs_iter != childs_end; ++childs_iter, child_node = *childs_iter, ++min_depth) {
    if (NULL != child_node) {
      if (*min_depth > self->wordlen + 1) {
        *min_depth = self->wordlen + 1;
      }
      CommentzWalterTrie_calcMinDepthForChar(child_node, min_depths, limit_depth);
    }
  }
}

#ifdef DEBUG

static void
CommentzWalterTrie_pprint(CommentzWalterTrie *self, guchar label, int depth, FILE *ostream)
{
  for (int i = 0; i < depth; ++i) {
    fprintf(ostream, "  ");
  }
  fprintf(ostream, "\"%c\": <%p> shift1=%d, shift2=%d, word=%s, wordlen=%ld, output=%p\n", label, self, self->shift1, self->shift2, self->word, (long) self->wordlen, self->output);
  CommentzWalterTrie **childs_iter = self->childs;
  CommentzWalterTrie **childs_end = self->childs + G_N_ELEMENTS(self->childs);
  CommentzWalterTrie *child_node = *childs_iter;
  guchar child_label = 0;
  for (; childs_iter != childs_end; ++childs_iter, child_node = *childs_iter, ++child_label) {
    if (NULL != child_node) {
      CommentzWalterTrie_pprint(child_node, child_label, depth + 1, ostream);
    }
  }
}

#endif // DEBUG

CommentzWalterMatcher *
CommentzWalterMatcher_new(gsize max_keyword_length)
{
  CommentzWalterMatcher *self = (CommentzWalterMatcher *) g_malloc0(sizeof(CommentzWalterMatcher));
  self->max_keyword_length = max_keyword_length;
  self->wordbuf = (gchar *) g_malloc_n(max_keyword_length, sizeof(gchar));
  self->trie = CommentzWalterTrie_new();
  self->wmin = max_keyword_length;
  self->compiled = FALSE;
  return self;
}

void
CommentzWalterMatcher_free(CommentzWalterMatcher *self)
{
  CommentzWalterTrie_free(self->trie);
  g_free(self->wordbuf);
  g_free(self);
}

void
CommentzWalterMatcher_addKeyword(CommentzWalterMatcher *self, const gchar *keyword, glong length)
{
  if (0L > length) {
      length = strlen(keyword);
  }
  CommentzWalterTrie_addKeyword(self->trie, keyword, length, self->wordbuf);
  self->compiled = FALSE;
  if (self->wmin > length) {
    self->wmin = length;
  }
}

void
CommentzWalterMatcher_compile(CommentzWalterMatcher *self)
{
  if (!self->compiled) {
    CommentzWalterTrie_compile(self->trie, self->trie, self->wmin, 0);
    guint *chars_iter = self->chars;
    guint *const chars_end = self->chars + G_N_ELEMENTS(self->chars);
    for (; chars_end != chars_iter; ++chars_iter) {
      *chars_iter = self->wmin + 1;
    }
    CommentzWalterTrie_calcMinDepthForChar(self->trie, self->chars, self->wmin);
    self->compiled = TRUE;
  }
}

void
CommentzWalterMatcher_scan(CommentzWalterMatcher *self, const gchar *document, glong length, gconstpointer *output)
{
  CommentzWalterMatcher_compile(self);

  if (0L > length) {
      length = strlen(document);
  }
  const gchar *document_start_iter = document + self->wmin - 1;
  const gchar *const document_end = document + length;
  g_assert(NULL != output);
  while (TRUE) {
    const CommentzWalterTrie *current_node = self->trie;
    const gchar *document_iter = document_start_iter;
    guchar label = 0;
    while (TRUE) {
      label = (guchar) *document_iter;
      const CommentzWalterTrie *next_node = current_node->childs[label];
      if (NULL == next_node) {
        break;
      }
      current_node = next_node;
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
CommentzWalterMatcher_pprintTrie(CommentzWalterMatcher *self, FILE *ostream)
{
  CommentzWalterMatcher_compile(self);
  CommentzWalterTrie_pprint(self->trie, ' ', 0, ostream);
}

#endif // DEBUG
