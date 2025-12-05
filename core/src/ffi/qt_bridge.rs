use std::{
    ffi::{CString, c_char},
    ptr,
};

use crate::text::{Editor, editor, selection};

pub struct NekoEditor {
    editor: Editor,
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_new() -> *mut NekoEditor {
    let editor = Box::new(NekoEditor {
        editor: Editor::new(),
    });
    Box::into_raw(editor)
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_free(editor: *mut NekoEditor) {
    if !editor.is_null() {
        unsafe {
            let _ = Box::from_raw(editor);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_left(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_left();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_right(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_right();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_up(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_up();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_move_down(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.move_down();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_text(
    editor: *mut NekoEditor,
    text: *const c_char,
    len: usize,
) {
    if editor.is_null() || text.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        let text_slice = std::slice::from_raw_parts(text as *const u8, len);
        if let Ok(text_str) = std::str::from_utf8(text_slice) {
            editor.editor.insert_text(text_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_newline(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.insert_newline();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_insert_tab(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.insert_tab();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_backspace(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.backspace();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_delete(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }
    unsafe {
        let editor = &mut *editor;
        editor.editor.delete();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_text(
    editor: *const NekoEditor,
    out_len: *mut usize,
) -> *mut c_char {
    if editor.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let editor = &*editor;
        let text = editor.editor.buffer().get_text();

        if !out_len.is_null() {
            *out_len = text.len();
        }

        match CString::new(text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_line(
    editor: *const NekoEditor,
    line_idx: usize,
    out_len: *mut usize,
) -> *mut c_char {
    if editor.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let editor = &*editor;
        let line_text = editor.editor.buffer().get_line(line_idx);

        if !out_len.is_null() {
            *out_len = line_text.len();
        }

        match CString::new(line_text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_line_count(
    editor: *const NekoEditor,
    out_line_count: *mut usize,
) {
    if editor.is_null() {
        if !out_line_count.is_null() {
            unsafe {
                *out_line_count = 0;
            }
        }
        return;
    }

    unsafe {
        let editor = &*editor;
        if !out_line_count.is_null() {
            *out_line_count = editor.editor.buffer().line_count();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_cursor_position(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let cursor = editor.editor.cursor();

        if !out_row.is_null() {
            *out_row = cursor.get_row();
        }
        if !out_col.is_null() {
            *out_col = cursor.get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_all(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_all();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_left(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_left();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_right(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_right();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_up(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_left();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_down(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_down();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_clear_selection(editor: *mut NekoEditor) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.clear_selection();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_start(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();

        if !out_row.is_null() {
            *out_row = selection.start().get_row();
        }
        if !out_col.is_null() {
            *out_col = selection.start().get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_end(
    editor: *const NekoEditor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();

        if !out_row.is_null() {
            *out_row = selection.end().get_row();
        }
        if !out_col.is_null() {
            *out_col = selection.end().get_col();
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_get_selection_active(editor: *const NekoEditor) -> bool {
    if editor.is_null() {
        return false;
    }

    unsafe {
        let editor = &*editor;
        let selection = editor.editor.selection();
        selection.is_active()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_string_free(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            let _ = CString::from_raw(s);
        }
    }
}
