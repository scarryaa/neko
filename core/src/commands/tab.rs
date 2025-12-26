use crate::AppState;
use std::{path::PathBuf, str::FromStr};

#[derive(Clone)]
pub enum TabCommand {
    Close,
    CloseOthers,
    CloseLeft,
    CloseRight,
    CloseAll,
    CopyPath,
    Reveal,
    Pin,
}

impl TabCommand {
    pub fn as_str(&self) -> &'static str {
        match self {
            TabCommand::Close => "tab.close",
            TabCommand::CloseOthers => "tab.closeOthers",
            TabCommand::CloseLeft => "tab.closeLeft",
            TabCommand::CloseRight => "tab.closeRight",
            TabCommand::CloseAll => "tab.closeAll",
            TabCommand::CopyPath => "tab.copyPath",
            TabCommand::Reveal => "tab.reveal",
            TabCommand::Pin => "tab.pin",
        }
    }

    pub fn all() -> &'static [TabCommand] {
        &[
            TabCommand::Close,
            TabCommand::CloseOthers,
            TabCommand::CloseLeft,
            TabCommand::CloseRight,
            TabCommand::CloseAll,
            TabCommand::CopyPath,
            TabCommand::Reveal,
            TabCommand::Pin,
        ]
    }
}

impl FromStr for TabCommand {
    type Err = ();

    fn from_str(id: &str) -> Result<Self, Self::Err> {
        match id {
            "tab.close" => Ok(Self::Close),
            "tab.closeOthers" => Ok(Self::CloseOthers),
            "tab.closeLeft" => Ok(Self::CloseLeft),
            "tab.closeRight" => Ok(Self::CloseRight),
            "tab.closeAll" => Ok(Self::CloseAll),
            "tab.copyPath" => Ok(Self::CopyPath),
            "tab.reveal" => Ok(Self::Reveal),
            "tab.pin" => Ok(Self::Pin),

            _ => Err(()),
        }
    }
}

pub struct TabContext {
    pub id: usize,
    pub is_pinned: bool,
    pub is_modified: bool,
    pub file_path: Option<PathBuf>,
}

pub struct TabCommandState {
    pub can_close: bool,
    pub can_close_others: bool,
    pub can_close_left: bool,
    pub can_close_right: bool,
    pub can_close_all: bool,
    pub can_copy_path: bool,
    pub can_reveal: bool,
    pub is_pinned: bool,
}

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

    Ok(TabCommandState {
        can_close: true,
        can_close_others,
        can_close_left,
        can_close_right,
        can_close_all: true,
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
        TabCommand::Close => app_state.close_tab(ctx.id)?,
        TabCommand::CloseOthers => app_state.close_other_tabs(ctx.id)?,
        TabCommand::CloseLeft => app_state.close_left_tabs(ctx.id)?,
        TabCommand::CloseRight => app_state.close_right_tabs(ctx.id)?,
        TabCommand::CloseAll => app_state.close_all_tabs()?,
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
