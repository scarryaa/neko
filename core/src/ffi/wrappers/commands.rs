use crate::{
    AppState,
    ffi::{TabCommandFfi, TabCommandStateFfi, TabContextFfi},
    get_available_tab_commands, run_tab_command, tab_command_state,
};

// Tab commands
pub(crate) fn get_tab_command_state(app_state: &AppState, id: u64) -> TabCommandStateFfi {
    match tab_command_state(app_state, id as usize) {
        Ok(state) => state.into(),
        Err(_) => TabCommandStateFfi {
            can_close: false,
            can_close_others: false,
            can_close_left: false,
            can_close_right: false,
            can_close_all: false,
            can_close_clean: false,
            can_copy_path: false,
            can_reveal: false,
            is_pinned: false,
        },
    }
}

pub(crate) fn run_tab_command_wrapper(
    app_state: &mut AppState,
    id: &str,
    ctx: TabContextFfi,
    close_pinned: bool,
) -> bool {
    run_tab_command(app_state, id, &ctx.into(), close_pinned).is_ok()
}

pub fn get_available_tab_commands_wrapper() -> Vec<TabCommandFfi> {
    let Ok(commands) = get_available_tab_commands() else {
        return Vec::new();
    };

    commands
        .into_iter()
        .map(|cmd| TabCommandFfi {
            id: cmd.as_str().to_string(),
        })
        .collect()
}
