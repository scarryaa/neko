use super::resolve_jump_key;
use crate::{
    AppState, ConfigManager, DocumentTarget, JumpAliasInfo, JumpCommand, JumpManagementCommand,
    JumpManagementResult, LineTarget,
};

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
        JumpCommand::ToPosition { row, column } => {
            app_state.with_editor_and_buffer_mut(|editor, buffer| {
                editor.move_to(buffer, row, column, true);
            });
        }
        JumpCommand::ToLine(target) => match target {
            LineTarget::Start => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_row = editor.last_added_cursor().row;

                    editor.move_to(buffer, current_row, 0, true);
                });
            }
            LineTarget::Middle => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_row = editor.last_added_cursor().row;
                    let line_len = editor.line_length(buffer, current_row);

                    editor.move_to(buffer, current_row, line_len / 2, true);
                });
            }
            LineTarget::End => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_row = editor.last_added_cursor().row;
                    let line_len = editor.line_length(buffer, current_row);

                    editor.move_to(buffer, current_row, line_len, true);
                });
            }
        },
        JumpCommand::ToDocument(target) => match target {
            DocumentTarget::Start => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    editor.move_to(buffer, 0, 0, true);
                });
            }
            DocumentTarget::Middle => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_column = editor.last_added_cursor().column;
                    let line_count = buffer.line_count();

                    editor.move_to(buffer, line_count / 2, current_column, true);
                });
            }
            DocumentTarget::End => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let line_count = buffer.line_count();
                    let last_line_len = editor.line_length(buffer, line_count);

                    editor.move_to(buffer, line_count, last_line_len, true);
                });
            }
            DocumentTarget::Quarter => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_column = editor.last_added_cursor().column;
                    let line_count = buffer.line_count();

                    editor.move_to(buffer, line_count / 4, current_column, true);
                });
            }
            DocumentTarget::ThreeQuarters => {
                app_state.with_editor_and_buffer_mut(|editor, buffer| {
                    let current_column = editor.last_added_cursor().column;
                    let line_count = buffer.line_count();

                    editor.move_to(buffer, (line_count / 4) * 3, current_column, true);
                });
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
