#include <stdio.h>
#include <string.h>
#include <glib.h>

#include "ahocorasickunicode.h"

// 論文において "goto function" と記されているものを GHashTable の入れ子として表現していて、
// GHashTable のキー値がステートマシンにおける遷移条件に相当する
// また、"failure function", "output function" は fail_state, output メンバとして表現している

struct UnicodeAhoCorasickState;
typedef struct UnicodeAhoCorasickState UnicodeAhoCorasickState;

struct UnicodeAhoCorasickState {
  GHashTable *next_states; // 要素は UnicodeAhoCorasickState
  const UnicodeAhoCorasickState *fail_state;
  gconstpointer output;
};

struct UnicodeAhoCorasickMatcher {
  gsize max_pattern_len;
  UnicodeAhoCorasickState *start_state;
  gboolean need_update;
  gunichar2 *conds_buf;
};

struct UnicodeAhoCorasickPatternsIter {
  const UnicodeAhoCorasickState *start_state;
  const UnicodeAhoCorasickState *current_state;
  const UnicodeAhoCorasickState *current_fail_state;
  const gunichar2 *text_iter;
  const gunichar2 *text_end;
  gunichar2 *text_allocated;
};

static void UnicodeAhoCorasickState_free(gpointer self);

static UnicodeAhoCorasickState *
UnicodeAhoCorasickState_new()
{
  UnicodeAhoCorasickState *self = (UnicodeAhoCorasickState *) g_malloc0(sizeof(UnicodeAhoCorasickState));
  self->next_states = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, UnicodeAhoCorasickState_free);
  return self;
}

static void
UnicodeAhoCorasickState_free(gpointer data)
{
  UnicodeAhoCorasickState *self = (UnicodeAhoCorasickState *) data;
  g_hash_table_destroy(self->next_states);
  g_free(self);
}

static UnicodeAhoCorasickState *
UnicodeAhoCorasickState_transferMany(UnicodeAhoCorasickState *self, const gunichar2 *inputs, gsize n_inputs)
{
  if (0 == n_inputs) {
    return self;
  }
  gpointer next_state = g_hash_table_lookup(self->next_states, GINT_TO_POINTER(*inputs));
  if (NULL == next_state) {
    // 入力に基づく遷移先がない
    return NULL;
  } else {
    return UnicodeAhoCorasickState_transferMany((UnicodeAhoCorasickState *) next_state, inputs + 1, n_inputs - 1);
  }
}

UnicodeAhoCorasickMatcher *
UnicodeAhoCorasickMatcher_new(gsize max_pattern_len)
{
  UnicodeAhoCorasickMatcher *self = (UnicodeAhoCorasickMatcher *) g_malloc0(sizeof(UnicodeAhoCorasickMatcher));
  self->max_pattern_len = max_pattern_len;
  self->start_state = UnicodeAhoCorasickState_new();
  self->conds_buf = (gunichar2 *) g_malloc0(sizeof(gunichar2) * max_pattern_len);
  self->need_update = FALSE;
  return self;
}

void
UnicodeAhoCorasickMatcher_free(UnicodeAhoCorasickMatcher *self)
{
  UnicodeAhoCorasickState_free(self->start_state);
  g_free(self->conds_buf);
  g_free(self);
}

void
UnicodeAhoCorasickMatcher_addKeywordImpl(UnicodeAhoCorasickMatcher *self, const gunichar2 *pattern, const gunichar2 *pattern_end, gconstpointer output)
{
  // 既に存在するノードをスキップする
  UnicodeAhoCorasickState *current_state = self->start_state;
  const gunichar2 *pattern_iter = pattern;
  for (; pattern_end != pattern_iter; ++pattern_iter) {
    gpointer next_state = g_hash_table_lookup(current_state->next_states, GINT_TO_POINTER(*pattern_iter));
    if (NULL == next_state) {
      break;
    }
    current_state = (UnicodeAhoCorasickState *) next_state;
  }
  // pattern を表現するために必要なノードを追加する
  for (; pattern_end != pattern_iter; ++pattern_iter) {
    UnicodeAhoCorasickState *new_state = UnicodeAhoCorasickState_new();
    new_state->fail_state = self->start_state;
    g_assert(g_hash_table_insert(current_state->next_states, GINT_TO_POINTER((gint) *pattern_iter), (gpointer) new_state));
    current_state = new_state;
  }
  // output を設定する
  current_state->output = output;
  // fail_state の更新を遅延実行する
  self->need_update = TRUE;
}

gboolean
UnicodeAhoCorasickMatcher_addKeywordAsUTF8(UnicodeAhoCorasickMatcher *self, const gchar *pattern, glong pattern_len, GError **error)
{
  glong u16pattern_len = 0L;
  GError *conv_error = NULL;
  gunichar2 *u16pattern = g_utf8_to_utf16(pattern, pattern_len, NULL, &u16pattern_len, &conv_error);
  if (u16pattern == NULL) {
    g_propagate_error(error, conv_error);
    return FALSE;
  }
  if (self->max_pattern_len < u16pattern_len) {
    g_set_error(error, AHOCORASICKUNICODE_ERROR, AHOCORASICKUNICODE_ERROR_TOO_LONG_PATTERN,
                "fed pattern is too long: len=%ld, max=%ld", u16pattern_len, self->max_pattern_len);
    g_free(u16pattern);
    return FALSE;
  }
  UnicodeAhoCorasickMatcher_addKeywordImpl(self, u16pattern, u16pattern + u16pattern_len, pattern);
  g_free(u16pattern);
  return TRUE;
}

gboolean
UnicodeAhoCorasickMatcher_addKeywordAsUTF16(UnicodeAhoCorasickMatcher *self, const gunichar2 *pattern, gsize pattern_len, GError **error)
{
  if (self->max_pattern_len < pattern_len) {
    g_set_error(error, AHOCORASICKUNICODE_ERROR, AHOCORASICKUNICODE_ERROR_TOO_LONG_PATTERN,
                "fed pattern is too long: len=%ld, max=%ld", pattern_len, self->max_pattern_len);
    return FALSE;
  }
  UnicodeAhoCorasickMatcher_addKeywordImpl(self, pattern, pattern + pattern_len, pattern);
  return TRUE;
}

static void
UnicodeAhoCorasickMatcher_updateFailStateRecursively(UnicodeAhoCorasickMatcher *self, UnicodeAhoCorasickState *state, gsize n_conditions)
{
  // fail_state を更新する
  if (2 <= n_conditions) {
    // 2層目までのステートの fail_state は必ず開始ノードなのでスキップする
    const gunichar2 *current_conds = self->conds_buf + 1;
    gsize current_n_conds = n_conditions - 1;
    for (; 0 < current_n_conds; ++current_conds, --current_n_conds) {
      UnicodeAhoCorasickState *fail_state = UnicodeAhoCorasickState_transferMany(self->start_state, current_conds, current_n_conds);
      if (NULL != fail_state) {
        state->fail_state = fail_state;
        break;
      }
    }
  }
  // 再帰的に処理する
  if (0 < g_hash_table_size(state->next_states)) {
    g_assert(n_conditions + 1 <= self->max_pattern_len);
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, state->next_states);
    gpointer condition = NULL;
    gpointer next_state = NULL;
    while (g_hash_table_iter_next(&iter, &condition, &next_state)) {
      self->conds_buf[n_conditions] = GPOINTER_TO_INT(condition);
      UnicodeAhoCorasickMatcher_updateFailStateRecursively(self, (UnicodeAhoCorasickState *) next_state, n_conditions + 1);
    }
  }
}

void
UnicodeAhoCorasickMatcher_scanImpl(UnicodeAhoCorasickMatcher *self, const gunichar2 *text, const gunichar2 *text_end, gunichar2 *text_allocated, UnicodeAhoCorasickPatternsIter **iter)
{
  // fail_state を再計算する
  if (self->need_update) {
    UnicodeAhoCorasickMatcher_updateFailStateRecursively(self, self->start_state, 0);
    memset(self->conds_buf, 0, sizeof(gunichar2) * self->max_pattern_len);
    self->need_update = FALSE;
  }
  UnicodeAhoCorasickPatternsIter *new_iter = (UnicodeAhoCorasickPatternsIter *) g_malloc0(sizeof(UnicodeAhoCorasickPatternsIter));
  new_iter->start_state = self->start_state;
  new_iter->current_state = self->start_state;
  new_iter->text_iter = text;
  new_iter->text_end = text_end;
  new_iter->text_allocated = text_allocated;
  g_assert(NULL != iter);
  *iter = new_iter;
}

gboolean
UnicodeAhoCorasickMatcher_scanUTF8String(UnicodeAhoCorasickMatcher *self, const gchar *text, glong textlen, UnicodeAhoCorasickPatternsIter **iter, GError **error)
{
  glong u16textlen = 0L;
  GError *conv_error = NULL;
  gunichar2 *u16text = g_utf8_to_utf16(text, textlen, NULL, &u16textlen, &conv_error);
  if (u16text == NULL) {
    g_propagate_error(error, conv_error);
    return FALSE;
  }
  UnicodeAhoCorasickMatcher_scanImpl(self, u16text, u16text + u16textlen, u16text, iter);
  return TRUE;
}

void
UnicodeAhoCorasickMatcher_scanUTF16String(UnicodeAhoCorasickMatcher *self, const gunichar2 *text, gsize textlen, UnicodeAhoCorasickPatternsIter **iter)
{
  UnicodeAhoCorasickMatcher_scanImpl(self, text, text + textlen, NULL, iter);
}

static void
UnicodeAhoCorasickMatcher_pprintAutomatonImpl(UnicodeAhoCorasickMatcher *self, UnicodeAhoCorasickState *state, gunichar2 condition, int depth, FILE *ostream)
{
  if (0 < depth) {
    for (int i = 1; i < depth; ++i) {
      fprintf(ostream, "  ");
    }
    gchar condition_as_utf8[6];
    gint length = g_unichar_to_utf8(condition, condition_as_utf8);
    fprintf(ostream, "\"%.*s\": <%p> ", length, condition_as_utf8, state);
    if (state->fail_state == self->start_state) {
      fprintf(ostream, "failure=(start), ");
    } else {
      fprintf(ostream, "failure=%p, ", state->fail_state);
    }
    fprintf(ostream, "output=%p\n", state->output);
  }

  GHashTableIter iter;
  g_hash_table_iter_init(&iter, state->next_states);
  gpointer next_condition = NULL;
  gpointer next_state = NULL;
  while (g_hash_table_iter_next(&iter, &next_condition, &next_state)) {
    UnicodeAhoCorasickMatcher_pprintAutomatonImpl(self, (UnicodeAhoCorasickState *) next_state, GPOINTER_TO_INT(next_condition), depth + 1, ostream);
  }
}

void
UnicodeAhoCorasickMatcher_pprintAutomaton(UnicodeAhoCorasickMatcher *self, FILE *ostream)
{
  // fail_state を再計算する
  if (self->need_update) {
    UnicodeAhoCorasickMatcher_updateFailStateRecursively(self, self->start_state, 0);
    memset(self->conds_buf, 0, sizeof(gunichar2) * self->max_pattern_len);
    self->need_update = FALSE;
  }
  UnicodeAhoCorasickMatcher_pprintAutomatonImpl(self, self->start_state, ' ', 0, ostream);
}

void
UnicodeAhoCorasickPatternsIter_free(UnicodeAhoCorasickPatternsIter *self)
{
  if (NULL != self) {
    g_free(self->text_allocated);
    g_free(self);
  }
}

gconstpointer
UnicodeAhoCorasickPatternsIter_next(UnicodeAhoCorasickPatternsIter *self)
{
  gconstpointer output = NULL;
  const UnicodeAhoCorasickState *current_state = self->current_state;
  const UnicodeAhoCorasickState *current_fail_state = self->current_fail_state;
  const gunichar2 *text_iter = self->text_iter;
  const gunichar2 *text_end = self->text_end;

  // fail_state を辿っている途中であれば継続する
  while (NULL != current_fail_state) {
    output = current_fail_state->output;
    current_fail_state = current_fail_state->fail_state;
    if (NULL != output) {
      goto escape;
    }
  }
  while (text_end != text_iter) {
    gpointer next_state = g_hash_table_lookup(current_state->next_states, GINT_TO_POINTER(*text_iter));
    if (NULL == next_state) {
      // 遷移先が存在しない
      if (self->start_state == current_state) {
        ++text_iter;
      } else {
        // fail_state に遷移してリトライする
        current_state = current_state->fail_state;
      }
    } else {
      current_state = (UnicodeAhoCorasickState *) next_state;
      current_fail_state = current_state->fail_state;
      ++text_iter;
      output = current_state->output;
      if (NULL != output) {
        break;
      }
      // fail_state も満たしていることになるので output の登録を調べる
      while (NULL != current_fail_state) {
        output = current_fail_state->output;
        current_fail_state = current_fail_state->fail_state;
        if (NULL != output) {
          goto escape;
        }
      }
    }
  }

 escape:
  self->current_state = current_state;
  self->current_fail_state = current_fail_state;
  self->text_iter = text_iter;
  return output;
}

