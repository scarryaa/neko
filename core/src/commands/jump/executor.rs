use super::{
    DocumentTarget, JumpAliasInfo, JumpCommand, JumpManagementCommand, JumpManagementResult,
    LineTarget, resolve_jump_key,
};
use crate::{AppState, ConfigManager};

pub fn execute_jump_key(key: String, app_state: &mut AppState) {
    let config_snapshot = app_state.get_config_snapshot();

    if let Some(command) = resolve_jump_key(&key, &config_snapshot) {
        execute_jump_command(command, app_state);
    }
}

pub fn execute_jump_management_command(
    command: JumpManagementCommand,
    config_manager: &mut ConfigManager,
) -> Result<JumpManagementResult, std::io::Error> {
    match command {
        JumpManagementCommand::AddAlias { name, spec } => {
            config_manager.update(|config| {
                config.jump.aliases.insert(name, spec);
            });
            Ok(JumpManagementResult::None)
        }
        JumpManagementCommand::RemoveAlias { name } => {
            config_manager.update(|config| {
                config.jump.aliases.remove(&name);
            });
            Ok(JumpManagementResult::None)
        }
        JumpManagementCommand::ListAliases => {
            let config = config_manager.get_snapshot();
            let aliases = config
                .jump
                .aliases
                .iter()
                .map(|(name, spec)| {
                    let description = JumpCommand::from_spec(spec)
                        .map(|command| command.to_string())
                        .unwrap_or_else(|| spec.clone());

                    JumpAliasInfo {
                        name: name.to_string(),
                        spec: spec.to_string(),
                        description,
                        is_builtin: false,
                    }
                })
                .collect();

            Ok(JumpManagementResult::Aliases(aliases))
        }
    }
}

pub fn execute_jump_command(command: JumpCommand, app_state: &mut AppState) {
    let should_record = !matches!(command, JumpCommand::ToLastTarget);

    if should_record {
        app_state.jump_history.store(command.clone());
    }

    match command {
        // TODO(scarlet): Reduce repetition here?
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
            if let Some(command) = app_state.jump_history.last() {
                execute_jump_command(command.clone(), app_state);
            }
        }
    }
}

pub fn get_available_jump_commands() -> std::io::Result<Vec<JumpCommand>> {
    Ok(JumpCommand::all().to_vec())
}
