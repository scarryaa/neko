use crate::{AppState, CloseTabOperationType, TabCommand, TabCommandState, TabContext};

pub fn tab_command_state(
    app_state: &AppState,
    id: usize,
) -> Result<TabCommandState, std::io::Error> {
    let tabs = app_state.get_tabs();

    let tab = tabs
        .iter()
        .find(|tab| tab.get_id() == id)
        .ok_or(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Tab with id {id} not found"),
        ))?;

    let is_pinned = tab.get_is_pinned();
    let document_id = tab.get_document_id();

    let has_path = app_state
        .get_document_manager()
        .get_document(document_id)
        .map(|doc| doc.path.is_some())
        .unwrap_or(false);

    let can_close_others = !app_state
        .get_close_tab_ids(CloseTabOperationType::Others, id, false)?
        .is_empty();
    let can_close_left = !app_state
        .get_close_tab_ids(CloseTabOperationType::Left, id, false)?
        .is_empty();
    let can_close_right = !app_state
        .get_close_tab_ids(CloseTabOperationType::Right, id, false)?
        .is_empty();
    let can_close_clean = !app_state
        .get_close_tab_ids(CloseTabOperationType::Clean, 0, false)?
        .is_empty();

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
    close_pinned: bool,
) -> std::io::Result<()> {
    use CloseTabOperationType::*;
    use TabCommand::*;

    let command: TabCommand = id.parse().map_err(|_| {
        std::io::Error::new(std::io::ErrorKind::InvalidInput, "Unknown tab command")
    })?;

    match command {
        tab_command @ (Close | CloseOthers | CloseLeft | CloseRight | CloseAll | CloseClean) => {
            let (operation_type, allow_pinned_close) = match tab_command {
                Close => (Single, true),
                CloseOthers => (Others, false),
                CloseLeft => (Left, false),
                CloseRight => (Right, false),
                CloseAll => (All, false),
                CloseClean => (Clean, false),
                _ => unreachable!("Pattern above guarantees only close variants"),
            };

            let _closed_ids =
                app_state.close_tabs(operation_type, ctx.id, allow_pinned_close && close_pinned)?;
        }
        Pin => {
            if ctx.is_pinned {
                app_state.unpin_tab(ctx.id)?;
            } else {
                app_state.pin_tab(ctx.id)?;
            }
        }
        Reveal => {
            if let Some(path) = ctx.file_path.as_deref() {
                app_state.reveal_in_file_tree(&path.to_string_lossy());
            }
        }
        CopyPath => {
            // Implemented by UI
        }
    }

    Ok(())
}

pub fn get_available_tab_commands() -> std::io::Result<Vec<TabCommand>> {
    Ok(TabCommand::all().to_vec())
}
