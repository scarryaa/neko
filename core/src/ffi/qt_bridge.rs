use std::{
    ffi::{CString, c_char},
    ptr,
};

use crate::Buffer;
use crate::Cursor;

pub struct NekoBuffer {
    buffer: Buffer,
}

pub struct NekoCursor {
    cursor: Cursor,
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_new() -> *mut NekoBuffer {
    let buffer = Box::new(NekoBuffer {
        buffer: Buffer::new(),
    });
    Box::into_raw(buffer)
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_free(buffer: *mut NekoBuffer) {
    if !buffer.is_null() {
        unsafe {
            let _ = Box::from_raw(buffer);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_insert(
    buffer: *mut NekoBuffer,
    pos: usize,
    text: *const c_char,
    len: usize,
) {
    if buffer.is_null() || text.is_null() {
        return;
    }

    unsafe {
        let buffer = &mut *buffer;
        let text_slice = std::slice::from_raw_parts(text as *const u8, len);
        if let Ok(text_str) = std::str::from_utf8(text_slice) {
            buffer.buffer.insert(pos, text_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_backspace(buffer: *mut NekoBuffer, pos: usize) {
    if buffer.is_null() {
        return;
    }

    unsafe {
        let buffer = &mut *buffer;
        buffer.buffer.backspace(pos);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_get_text(
    buffer: *const NekoBuffer,
    out_len: *mut usize,
) -> *mut c_char {
    if buffer.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let buffer = &*buffer;
        let text = buffer.buffer.get_text();

        if !out_len.is_null() {
            *out_len = text.len();
        }

        // Create a C string that the caller must free
        match CString::new(text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_get_line(
    buffer: *const NekoBuffer,
    line_idx: usize,
    out_len: *mut usize,
) -> *mut c_char {
    if buffer.is_null() {
        if !out_len.is_null() {
            unsafe {
                *out_len = 0;
            }
        }
        return ptr::null_mut();
    }

    unsafe {
        let buffer = &*buffer;
        let line_text = buffer.buffer.get_line(line_idx);

        if !out_len.is_null() {
            *out_len = line_text.len();
        }

        // Create a C string that the caller must free
        match CString::new(line_text) {
            Ok(c_string) => c_string.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_buffer_get_line_count(
    buffer: *const NekoBuffer,
    out_line_count: *mut usize,
) {
    if buffer.is_null() {
        if !out_line_count.is_null() {
            unsafe {
                *out_line_count = 0;
            }
        }
        return;
    }

    unsafe {
        let buffer = &*buffer;

        if !out_line_count.is_null() {
            *out_line_count = buffer.buffer.line_count();
        }
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

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_move_left(cursor: *mut NekoCursor, buffer: &NekoBuffer) {
    unsafe {
        let cursor = &mut *cursor;
        cursor.cursor.move_left(&buffer.buffer);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_move_up(cursor: *mut NekoCursor, buffer: &NekoBuffer) {
    unsafe {
        let cursor = &mut *cursor;
        cursor.cursor.move_up(&buffer.buffer);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_move_down(cursor: *mut NekoCursor, buffer: &NekoBuffer) {
    unsafe {
        let cursor = &mut *cursor;
        cursor.cursor.move_down(&buffer.buffer);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_move_right(cursor: *mut NekoCursor, buffer: &NekoBuffer) {
    unsafe {
        let cursor = &mut *cursor;
        cursor.cursor.move_right(&buffer.buffer);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_new() -> *mut NekoCursor {
    let cursor = Box::new(NekoCursor {
        cursor: Cursor::new(),
    });
    Box::into_raw(cursor)
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_get_position(
    cursor: *const NekoCursor,
    out_row: *mut usize,
    out_col: *mut usize,
) {
    if cursor.is_null() {
        return;
    }

    unsafe {
        let cursor = &*cursor;
        let row = cursor.cursor.get_row();
        let col = cursor.cursor.get_col();

        if !out_row.is_null() {
            *out_row = row;
        }

        if !out_col.is_null() {
            *out_col = col;
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_set_position(
    cursor: *mut NekoCursor,
    new_row: usize,
    new_col: usize,
) {
    if cursor.is_null() {
        return;
    }

    unsafe {
        let cursor = &mut *cursor;

        cursor.cursor.set_row(new_row);
        cursor.cursor.set_col(new_col);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_get_idx(
    cursor: *const NekoCursor,
    buffer: *const NekoBuffer,
    out_idx: *mut usize,
) {
    if cursor.is_null() {
        unsafe {
            *out_idx = 0;
            return;
        }
    }

    unsafe {
        let cursor = &*cursor;
        let buffer = &*buffer;
        let idx = cursor.cursor.get_idx(&buffer.buffer);

        if !out_idx.is_null() {
            *out_idx = idx;
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_cursor_free(cursor: *mut NekoCursor) {
    if !cursor.is_null() {
        unsafe {
            let _ = Box::from_raw(cursor);
        }
    }
}
