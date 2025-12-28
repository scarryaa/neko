use std::{path::PathBuf, str::FromStr};

#[derive(Clone)]
pub enum TabCommand {
    Close,
    CloseOthers,
    CloseLeft,
    CloseRight,
    CloseAll,
    CloseClean,
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
            TabCommand::CloseClean => "tab.closeClean",
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
            TabCommand::CloseClean,
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
            "tab.closeClean" => Ok(Self::CloseClean),
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
    pub can_close_clean: bool,
    pub can_copy_path: bool,
    pub can_reveal: bool,
    pub is_pinned: bool,
}
