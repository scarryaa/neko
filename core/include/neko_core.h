#ifndef NEKO_CORE_H
#define NEKO_CORE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Create a new text buffer
typedef struct NekoBuffer NekoBuffer;
NekoBuffer *neko_buffer_new(void);
void neko_buffer_free(NekoBuffer *buffer);

// Insert text at position
void neko_buffer_insert(NekoBuffer *buffer, size_t pos, const char *text,
                        size_t len);

// Get buffer content
const char *neko_buffer_get_text(const NekoBuffer *buffer, size_t *out_len);

// Free a string
void neko_string_free(char *s);

// Cursor methods
typedef struct NekoCursor NekoCursor;
NekoCursor *neko_cursor_new(void);
void neko_cursor_free(NekoCursor *cursor);
void neko_cursor_move_left(NekoCursor *cursor, const NekoBuffer *buffer);
void neko_cursor_move_right(NekoCursor *cursor, const NekoBuffer *buffer);
void neko_cursor_move_up(NekoCursor *cursor, const NekoBuffer *buffer);
void neko_cursor_move_down(NekoCursor *cursor, const NekoBuffer *buffer);
void neko_cursor_get_position(const NekoCursor *cursor, size_t *out_row,
                              size_t *out_col);
void neko_cursor_get_idx(const NekoCursor *cursor, const NekoBuffer *buffer,
                         size_t *out_idx);

#ifdef __cplusplus
}
#endif

#endif // NEKO_CORE_H
