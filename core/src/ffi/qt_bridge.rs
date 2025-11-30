use std::{
    ffi::{CString, c_char},
    ptr,
};

use crate::Buffer;

pub struct NekoBuffer {
    buffer: Buffer,
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
pub extern "C" fn neko_string_free(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            let _ = CString::from_raw(s);
        }
    }
}
