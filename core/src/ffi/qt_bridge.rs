use std::{
    ffi::{CStr, CString, c_char},
    path::PathBuf,
    ptr,
};

use crate::{AppState, FileNode, FileTree, config::ConfigManager, text::Editor};

#[repr(C)]
pub struct NekoAppState {
    state: AppState,
}

#[repr(C)]
pub struct NekoConfigManager {
    manager: ConfigManager,
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_manager_new() -> *mut NekoConfigManager {
    let manager = Box::new(NekoConfigManager {
        manager: ConfigManager::new(),
    });
    Box::into_raw(manager)
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_manager_free(manager: *mut NekoConfigManager) {
    if !manager.is_null() {
        unsafe {
            let _ = Box::from_raw(manager);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_save(manager: *mut NekoConfigManager) -> bool {
    if manager.is_null() {
        return false;
    }

    unsafe {
        let manager = &*manager;
        manager.manager.save().is_ok()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_get_editor_font_size(manager: *mut NekoConfigManager) -> usize {
    if manager.is_null() {
        return 15;
    }

    unsafe {
        let manager = &*manager;
        manager.manager.get_snapshot().editor_font_size
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_get_editor_font_family(
    manager: *mut NekoConfigManager,
) -> *mut c_char {
    if manager.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let manager = &*manager;
        let s = manager.manager.get_snapshot().editor_font_family;
        match CString::new(s) {
            Ok(c) => c.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_get_file_explorer_font_size(
    manager: *mut NekoConfigManager,
) -> usize {
    if manager.is_null() {
        return 15;
    }

    unsafe {
        let manager = &*manager;
        manager.manager.get_snapshot().file_explorer_font_size
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_get_file_explorer_font_family(
    manager: *mut NekoConfigManager,
) -> *mut c_char {
    if manager.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let manager = &*manager;
        let s = manager.manager.get_snapshot().file_explorer_font_family;
        match CString::new(s) {
            Ok(c) => c.into_raw(),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_set_editor_font_size(
    manager: *mut NekoConfigManager,
    font_size: usize,
) {
    if manager.is_null() {
        return;
    }

    unsafe {
        let manager = &*manager;
        manager.manager.update(|c| c.editor_font_size = font_size);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_set_editor_font_family(
    manager: *mut NekoConfigManager,
    font_family: *const c_char,
) {
    if manager.is_null() || font_family.is_null() {
        return;
    }

    unsafe {
        let manager = &*manager;
        if let Ok(s) = CStr::from_ptr(font_family).to_str() {
            let str = s.to_string();
            manager.manager.update(|c| c.editor_font_family = str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_set_file_explorer_font_size(
    manager: *mut NekoConfigManager,
    font_size: usize,
) {
    if manager.is_null() {
        return;
    }

    unsafe {
        let manager = &*manager;
        manager
            .manager
            .update(|c| c.file_explorer_font_size = font_size);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_set_file_explorer_font_family(
    manager: *mut NekoConfigManager,
    font_family: *const c_char,
) {
    if manager.is_null() || font_family.is_null() {
        return;
    }

    unsafe {
        let manager = &*manager;
        if let Ok(s) = CStr::from_ptr(font_family).to_str() {
            let str = s.to_string();
            manager
                .manager
                .update(|c| c.file_explorer_font_family = str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_set_file_explorer_directory(
    manager: *mut NekoConfigManager,
    dir: *const c_char,
) {
    if manager.is_null() || dir.is_null() {
        return;
    }

    unsafe {
        let manager = &*manager;
        if let Ok(s) = CStr::from_ptr(dir).to_str() {
            let str = s.to_string();
            manager
                .manager
                .update(|c| c.file_explorer_directory = Some(str));
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_config_get_file_explorer_directory(
    manager: *mut NekoConfigManager,
) -> *mut c_char {
    if manager.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let manager = &*manager;
        let maybe_dir = manager.manager.get_snapshot().file_explorer_directory;

        match maybe_dir {
            Some(dir_str) => match CString::new(dir_str) {
                Ok(c_string) => c_string.into_raw(),
                Err(_) => ptr::null_mut(),
            },
            None => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_new(root_path: *const c_char) -> *mut NekoAppState {
    if root_path.is_null() {
        return match AppState::new(None) {
            Ok(state) => Box::into_raw(Box::new(NekoAppState { state })),
            Err(_) => ptr::null_mut(),
        };
    };

    unsafe {
        let path_str = match CStr::from_ptr(root_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null_mut(),
        };

        match AppState::new(Some(path_str)) {
            Ok(state) => Box::into_raw(Box::new(NekoAppState { state })),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_free(app: *mut NekoAppState) {
    if !app.is_null() {
        unsafe {
            let _ = Box::from_raw(app);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_is_file_open(app: *mut NekoAppState, path: *const c_char) -> bool {
    if app.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let app = &*app;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return false,
        };
        app.state.tab_with_path_exists(path_str)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_tab_index_by_path(
    app: *mut NekoAppState,
    path: *const c_char,
) -> i64 {
    if app.is_null() || path.is_null() {
        return -1;
    }

    unsafe {
        let app = &*app;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return -1,
        };

        app.state
            .get_tab_index_by_path(path_str)
            .map(|i| i as i64)
            .unwrap_or(-1)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_open_file(app: *mut NekoAppState, path: *const c_char) -> bool {
    if app.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let app = &mut *app;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return false,
        };

        app.state.open_file(path_str).is_ok()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_save_file(app: *mut NekoAppState) -> bool {
    if app.is_null() {
        return false;
    }

    unsafe {
        let app = &mut *app;
        app.state.save_file().is_ok()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_save_and_set_path(
    app: *mut NekoAppState,
    path: *const c_char,
) -> bool {
    if app.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let app = &mut *app;
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return false,
        };

        app.state.save_and_set_path(path_str).is_ok()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_editor(app: *mut NekoAppState) -> *mut Editor {
    if app.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let app = &mut *app;
        app.state
            .get_active_editor_mut()
            .ok_or(ptr::null_mut::<Editor>())
            .unwrap()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_tab_titles(
    app: *mut NekoAppState,
    titles: *mut *mut *mut c_char,
    count: *mut usize,
) {
    if app.is_null() || titles.is_null() || count.is_null() {
        return;
    }

    unsafe {
        let app = &*app;
        let tab_titles = app.state.get_tab_titles();

        let c_titles: Vec<*mut c_char> = tab_titles
            .iter()
            .map(|title| {
                CString::new(title.as_str())
                    .unwrap_or_else(|_| CString::new("").unwrap())
                    .into_raw()
            })
            .collect();

        *count = c_titles.len();

        let boxed_slice = c_titles.into_boxed_slice();
        *titles = Box::into_raw(boxed_slice) as *mut *mut c_char;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_free_tab_titles(titles: *mut *mut c_char, count: usize) {
    if titles.is_null() {
        return;
    }

    unsafe {
        let titles_slice = std::slice::from_raw_parts_mut(titles, count);

        (0..count).for_each(|i| {
            let title_ptr = titles_slice[i];
            if !title_ptr.is_null() {
                drop(CString::from_raw(title_ptr));
            }
        });

        drop(Box::from_raw(std::slice::from_raw_parts_mut(titles, count)));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_tab_modified_states(
    app: *mut NekoAppState,
    modifieds: *mut *mut bool,
    count: *mut usize,
) {
    if app.is_null() || modifieds.is_null() || count.is_null() {
        return;
    }
    unsafe {
        let app = &*app;
        let tab_modifieds = app.state.get_tab_modified_states();
        *count = tab_modifieds.len();

        let boxed_slice = tab_modifieds.into_boxed_slice();
        let ptr = Box::into_raw(boxed_slice) as *mut bool;
        *modifieds = ptr;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_tab_modified(app: *mut NekoAppState, index: usize) -> bool {
    if app.is_null() {
        return false;
    }

    unsafe {
        let app = &*app;
        app.state.get_tab_modified(index)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_free_tab_modified_states(modifieds: *mut bool, count: usize) {
    if modifieds.is_null() {
        return;
    }
    unsafe {
        let _ = Box::from_raw(std::slice::from_raw_parts_mut(modifieds, count));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_tab_count(app: *mut NekoAppState) -> usize {
    if app.is_null() {
        return 0;
    }

    unsafe {
        let app = &*app;
        app.state.get_tab_count()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_active_tab_index(app: *mut NekoAppState) -> usize {
    if app.is_null() {
        return 0;
    }

    unsafe {
        let app = &*app;
        app.state.get_active_tab_index()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_close_tab(app: *mut NekoAppState, index: usize) -> bool {
    if app.is_null() {
        return false;
    }

    unsafe {
        let app = &mut *app;
        app.state.close_tab(index).is_ok()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_set_active_tab(app: *mut NekoAppState, index: usize) {
    if app.is_null() {
        return;
    }

    unsafe {
        let app = &mut *app;
        app.state.set_active_tab_index(index);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_new_tab(app: *mut NekoAppState) {
    if app.is_null() {
        return;
    }

    unsafe {
        let app = &mut *app;
        app.state.new_tab();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_app_state_get_file_tree(app: *mut NekoAppState) -> *mut FileTree {
    if app.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let app = &mut *app;
        &mut *app.state.get_file_tree_mut()
    }
}

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
pub extern "C" fn neko_editor_move_to(editor: *mut NekoEditor, row: usize, col: usize) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.move_to(row, col, true);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_select_to(editor: *mut NekoEditor, to_row: usize, to_col: usize) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.select_to(to_row, to_col);
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
pub extern "C" fn neko_editor_get_max_width(editor: *mut NekoEditor) -> f64 {
    if editor.is_null() {
        return 0.0;
    }

    unsafe {
        let editor = &*editor;
        editor.editor.max_width()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_set_line_width(
    editor: *mut NekoEditor,
    line_idx: usize,
    line_width: usize,
) {
    if editor.is_null() {
        return;
    }

    unsafe {
        let editor = &mut *editor;
        editor.editor.update_line_width(line_idx, line_width as f64);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_needs_width_measurement(
    editor: *const NekoEditor,
    line_idx: usize,
) -> bool {
    if editor.is_null() {
        return false;
    }

    unsafe {
        let editor = &*editor;
        editor.editor.needs_width_measurement(line_idx)
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
        editor.editor.select_up();
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
pub extern "C" fn neko_editor_paste(editor: *mut NekoEditor, text: *const c_char, len: usize) {
    if !editor.is_null() && !text.is_null() {
        unsafe {
            let editor = &mut *editor;
            let text_slice = std::slice::from_raw_parts(text as *const u8, len);
            if let Ok(text_str) = std::str::from_utf8(text_slice) {
                editor.editor.insert_text(text_str);
            }
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_editor_copy(editor: *const NekoEditor, out_len: *mut usize) -> *mut c_char {
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
        let selection = editor.editor.selection();

        let text = editor
            .editor
            .buffer()
            .get_text_range(selection.start().get_idx(), selection.end().get_idx());

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
pub extern "C" fn neko_string_free(s: *mut c_char) {
    if !s.is_null() {
        unsafe {
            let _ = CString::from_raw(s);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_new(root_path: *const c_char) -> *mut FileTree {
    if root_path.is_null() {
        return ptr::null_mut();
    }

    unsafe {
        let c_str = CStr::from_ptr(root_path);
        let path_str = match c_str.to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null_mut(),
        };

        match FileTree::new(Some(path_str)) {
            Ok(tree) => Box::into_raw(Box::new(tree)),
            Err(_) => ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_free(tree: *mut FileTree) {
    if !tree.is_null() {
        unsafe {
            let _ = Box::from_raw(tree);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_set_root_path(tree: *mut FileTree, path: *const c_char) {
    if path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        let c_str = CStr::from_ptr(path);
        if let Ok(s) = c_str.to_str() {
            tree.set_root_path(s)
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_children(
    tree: *mut FileTree,
    path: *const c_char,
    out_nodes: *mut *const FileNode,
    out_count: *mut usize,
) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        let c_str = CStr::from_ptr(path);

        if let Ok(path_str) = c_str.to_str() {
            let children = tree.get_children(path_str);

            if !out_nodes.is_null() {
                *out_nodes = children.as_ptr();
            }

            if !out_count.is_null() {
                *out_count = children.len();
            }
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_node(
    tree: *mut FileTree,
    current_path: *const c_char,
) -> *const FileNode {
    if tree.is_null() || current_path.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;
        let path = match CStr::from_ptr(current_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null(),
        };

        tree.get_node(path)
            .map(|node| node as *const FileNode)
            .unwrap_or(ptr::null())
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_next(
    tree: *mut FileTree,
    current_path: *const c_char,
) -> *const FileNode {
    if tree.is_null() || current_path.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;
        let path = match CStr::from_ptr(current_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null(),
        };

        tree.next(path)
            .map(|node| node as *const FileNode)
            .unwrap_or(ptr::null())
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_prev(
    tree: *mut FileTree,
    current_path: *const c_char,
) -> *const FileNode {
    if tree.is_null() || current_path.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;
        let path = match CStr::from_ptr(current_path).to_str() {
            Ok(s) => s,
            Err(_) => return ptr::null(),
        };

        tree.prev(path)
            .map(|node| node as *const FileNode)
            .unwrap_or(ptr::null())
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_parent(
    tree: *mut FileTree,
    path: *const c_char,
) -> *mut c_char {
    if tree.is_null() || path.is_null() {
        return std::ptr::null_mut();
    }

    unsafe {
        let path_str = match CStr::from_ptr(path).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        };

        let path_buf = PathBuf::from(path_str);

        let parent = match path_buf.parent() {
            Some(p) => p,
            None => return std::ptr::null_mut(),
        };

        match CString::new(parent.to_string_lossy().as_ref()) {
            Ok(cstring) => cstring.into_raw(),
            Err(_) => std::ptr::null_mut(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_toggle_expanded(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.toggle_expanded(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_set_expanded(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.set_expanded(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_set_collapsed(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.set_collapsed(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_visible_nodes(
    tree: *mut FileTree,
    out_nodes: *mut *const FileNode,
    out_count: *mut usize,
) {
    if tree.is_null() || out_nodes.is_null() || out_count.is_null() {
        return;
    }
    unsafe {
        let tree = &mut *tree;

        let new_visible = tree.get_visible_nodes_owned();

        *out_count = new_visible.len();
        *out_nodes = new_visible.as_ptr();

        for mut node in std::mem::take(&mut tree.cached_visible) {
            node.free_strings();
        }

        tree.cached_visible = new_visible;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_toggle_select(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.toggle_select(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_set_current(tree: *mut FileTree, path: *const c_char) {
    if tree.is_null() || path.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        if let Ok(path_str) = CStr::from_ptr(path).to_str() {
            tree.set_current(path_str);
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_clear_current(tree: *mut FileTree) {
    if tree.is_null() {
        return;
    }

    unsafe {
        let tree = &mut *tree;
        tree.clear_current();
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_index(tree: *mut FileTree) -> usize {
    if tree.is_null() {
        return 0;
    }

    unsafe {
        let tree = &*tree;
        tree.get_index()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_get_current(tree: *const FileTree) -> *const c_char {
    if tree.is_null() {
        return ptr::null();
    }

    unsafe {
        let tree = &*tree;

        match tree.get_current() {
            Some(path_buf) => match path_buf.to_str() {
                Some(path_str) => match CString::new(path_str) {
                    Ok(cstr) => cstr.into_raw(),
                    Err(_) => ptr::null(),
                },
                None => ptr::null(),
            },
            None => ptr::null(),
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_is_expanded(tree: *mut FileTree, path: *const c_char) -> bool {
    if tree.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let tree = &*tree;
        CStr::from_ptr(path)
            .to_str()
            .map(|p| tree.is_expanded(p))
            .unwrap_or(false)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_is_selected(tree: *mut FileTree, path: *const c_char) -> bool {
    if tree.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let tree = &*tree;
        CStr::from_ptr(path)
            .to_str()
            .map(|p| tree.is_selected(p))
            .unwrap_or(false)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn neko_file_tree_is_current(tree: *mut FileTree, path: *const c_char) -> bool {
    if tree.is_null() || path.is_null() {
        return false;
    }

    unsafe {
        let tree = &*tree;
        CStr::from_ptr(path)
            .to_str()
            .map(|p| tree.is_current(p))
            .unwrap_or(false)
    }
}
