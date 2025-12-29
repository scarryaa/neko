use super::{DocumentTarget, JumpCommand, LineTarget};
use crate::AppState;

pub fn execute_jump_command(cmd: JumpCommand, app_state: &mut AppState) {
    match cmd {
        JumpCommand::ToPosition { row, column } => {
            if let Some(editor) = app_state.get_active_editor_mut() {
                editor.move_to(row, column, true);
            }
        }
        JumpCommand::ToLine(target) => match target {
            LineTarget::Start => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_row = editor.get_last_added_cursor().row;

                    editor.move_to(current_row, 0, true);
                }
            }
            LineTarget::Middle => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_row = editor.get_last_added_cursor().row;
                    let line_len = editor.line_length(current_row);

                    editor.move_to(current_row, line_len / 2, true);
                }
            }
            LineTarget::End => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_row = editor.get_last_added_cursor().row;
                    let line_len = editor.line_length(current_row);

                    editor.move_to(current_row, line_len, true);
                }
            }
        },
        JumpCommand::ToDocument(target) => match target {
            DocumentTarget::Start => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    editor.move_to(0, 0, true);
                }
            }
            DocumentTarget::Middle => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_column = editor.get_last_added_cursor().col;
                    let line_count = editor.get_line_count();

                    editor.move_to(line_count / 2, current_column, true);
                }
            }
            DocumentTarget::End => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let line_count = editor.get_line_count();
                    let last_line_len = editor.line_length(line_count);

                    editor.move_to(line_count, last_line_len, true);
                }
            }
            DocumentTarget::Quarter => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_column = editor.get_last_added_cursor().col;
                    let line_count = editor.get_line_count();

                    editor.move_to(line_count / 4, current_column, true);
                }
            }
            DocumentTarget::ThreeQuarters => {
                if let Some(editor) = app_state.get_active_editor_mut() {
                    let current_column = editor.get_last_added_cursor().col;
                    let line_count = editor.get_line_count();

                    editor.move_to((line_count / 4) * 3, current_column, true);
                }
            }
        },
        JumpCommand::ToLastTarget => {
            // TODO(scarlet)
        }
    }
}

pub fn get_available_jump_commands() -> std::io::Result<Vec<JumpCommand>> {
    Ok(JumpCommand::all().to_vec())
}
