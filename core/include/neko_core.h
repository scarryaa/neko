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

#ifdef __cplusplus
}
#endif

#endif // NEKO_CORE_H
