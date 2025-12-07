#ifndef NEKO_CORE_H
#define NEKO_CORE_H

#include <cstddef>
#include <cstdint>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NekoAppState NekoAppState;
typedef struct NekoEditor NekoEditor;
typedef struct FileTree FileTree;
typedef struct FileNode {
  const char *path;
  const char *name;
  bool is_dir;
  bool is_hidden;
  uint64_t size;
  uint64_t modified;
  uint64_t depth;
} FileNode;

NekoAppState *neko_app_state_new(const char *root_path);
bool neko_app_state_open_file(NekoAppState *app, const char *path);
NekoEditor *neko_app_state_get_editor(NekoAppState *app);
FileTree *neko_app_state_get_file_tree(NekoAppState *app);
void neko_app_state_free(NekoAppState *app);

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
void neko_editor_clear_selection(NekoEditor *editor);

char *neko_editor_copy(NekoEditor *editor, size_t *out_len);
void neko_editor_paste(NekoEditor *editor, const char *text, size_t *out_len);

// Cursor methods
void neko_editor_get_cursor_position(const NekoEditor *editor, size_t *out_row,
                                     size_t *out_col);

void neko_editor_get_selection_start(const NekoEditor *editor, size_t *out_row,
                                     size_t *out_col);
void neko_editor_get_selection_end(const NekoEditor *editor, size_t *out_row,
                                   size_t *out_col);
bool neko_editor_get_selection_active(const NekoEditor *editor);

FileTree *neko_file_tree_new(const char *root_path);
void neko_file_tree_get_children(FileTree *tree, const char *path,
                                 const FileNode **out_nodes, size_t *out_count);
void neko_file_tree_free(FileTree *tree);
const FileNode *neko_file_tree_next(FileTree *tree, const char *current_path);
const FileNode *neko_file_tree_prev(FileTree *tree, const char *current_path);
void neko_file_tree_toggle_expanded(FileTree *tree, const char *path);
void neko_file_tree_set_expanded(FileTree *tree, const char *path);
void neko_file_tree_set_collapsed(FileTree *tree, const char *path);
void neko_file_tree_toggle_select(FileTree *tree, const char *path);
void neko_file_tree_set_current(FileTree *tree, const char *path);
void neko_file_tree_set_root_path(FileTree *tree, const char *path);
const char *neko_file_tree_get_current(FileTree *tree);
char *neko_file_tree_get_parent(FileTree *tree, const char *current_path);
const FileNode *neko_file_tree_get_node(FileTree *tree,
                                        const char *current_path);
void neko_file_tree_get_visible_nodes(FileTree *tree,
                                      const FileNode **out_nodes,
                                      size_t *out_count);
bool neko_file_tree_is_selected(FileTree *tree, const char *path);
bool neko_file_tree_is_expanded(FileTree *tree, const char *path);
bool neko_file_tree_is_current(FileTree *tree, const char *path);

#ifdef __cplusplus
}
#endif

#endif // NEKO_CORE_H
