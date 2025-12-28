use super::{TabCommand, TabCommandState, TabContext};
use crate::AppState;

pub fn tab_command_state(app_state: &AppState, id: usize) -> std::io::Result<TabCommandState> {
    let tabs = app_state.get_tabs();

    let index = tabs.iter().position(|t| t.get_id() == id).ok_or_else(|| {
        std::io::Error::new(std::io::ErrorKind::NotFound, "Tab with given id not found")
    })?;

    let tab = &tabs[index];
    let is_pinned = tab.get_is_pinned();
    let has_path = tab.get_file_path().is_some();

    let can_close_others = !app_state.get_close_other_tab_ids(id)?.is_empty();
    let can_close_left = !app_state.get_close_left_tab_ids(id)?.is_empty();
    let can_close_right = !app_state.get_close_right_tab_ids(id)?.is_empty();
    let can_close_clean = !app_state.get_close_clean_tab_ids()?.is_empty();

    Ok(TabCommandState {
        can_close: true,
        can_close_others,
        can_close_left,
        can_close_right,
        can_close_all: true,
        can_close_clean,
        can_copy_path: has_path,
        can_reveal: has_path,
        is_pinned,
    })
}

pub fn run_tab_command(
    app_state: &mut AppState,
    id: &str,
    ctx: &TabContext,
) -> std::io::Result<()> {
    let command: TabCommand = id.parse().map_err(|_| {
        std::io::Error::new(std::io::ErrorKind::InvalidInput, "Unknown tab command")
    })?;

    match command {
        TabCommand::Close => {
            app_state.close_tab(ctx.id)?;
        }
        TabCommand::CloseOthers => {
            app_state.close_other_tabs(ctx.id)?;
        }
        TabCommand::CloseLeft => {
            app_state.close_left_tabs(ctx.id)?;
        }
        TabCommand::CloseRight => {
            app_state.close_right_tabs(ctx.id)?;
        }
        TabCommand::CloseAll => {
            app_state.close_all_tabs()?;
        }
        TabCommand::CloseClean => {
            app_state.close_clean_tabs()?;
        }
        TabCommand::Pin => {
            if ctx.is_pinned {
                app_state.unpin_tab(ctx.id)?;
            } else {
                app_state.pin_tab(ctx.id)?;
            }
        }
        TabCommand::Reveal => {
            if let Some(path) = ctx.file_path.as_deref() {
                app_state.reveal_in_file_tree(&path.to_string_lossy());
            }
        }
        TabCommand::CopyPath => {
            // Implemented by UI
        }
    }

    Ok(())
}

pub fn get_available_tab_commands() -> std::io::Result<Vec<TabCommand>> {
    Ok(TabCommand::all().to_vec())
}
