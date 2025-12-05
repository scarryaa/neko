#ifndef NEKO_CORE_H
#define NEKO_CORE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NekoEditor NekoEditor;
NekoEditor *neko_editor_new(void);
void neko_editor_free(NekoEditor *editor);

// Insert text at position
void neko_editor_insert_text(NekoEditor *editor, const char *text, size_t len);
void neko_editor_insert_newline(NekoEditor *editor);
void neko_editor_insert_tab(NekoEditor *editor);

// Backspace text at position
void neko_editor_backspace(NekoEditor *editor);
void neko_editor_delete(NekoEditor *editor);

// Get content
const char *neko_editor_get_text(const NekoEditor *editor, size_t *out_len);
const char *neko_editor_get_line(const NekoEditor *editor,
                                 const size_t line_idx, size_t *out_len);

// Get line count
void neko_editor_get_line_count(NekoEditor *editor, size_t *out_line_count);

// Free a string
void neko_string_free(char *s);

void neko_editor_move_left(const NekoEditor *editor);
void neko_editor_move_right(const NekoEditor *editor);
void neko_editor_move_up(const NekoEditor *editor);
void neko_editor_move_down(const NekoEditor *editor);

void neko_editor_select_all(NekoEditor *editor);
void neko_editor_select_left(NekoEditor *editor);
void neko_editor_select_right(NekoEditor *editor);
void neko_editor_select_up(NekoEditor *editor);
void neko_editor_select_down(NekoEditor *editor);

// Cursor methods
void neko_editor_get_cursor_position(const NekoEditor *editor, size_t *out_row,
                                     size_t *out_col);

#ifdef __cplusplus
}
#endif

#endif // NEKO_CORE_H
