use std::{fmt, io, path::PathBuf, str::FromStr};

#[derive(Debug)]
pub enum FileExplorerCommandError {
    UnknownCommand(String),
    ParseError(String),
    IoError(String, io::Error),
}

impl fmt::Display for FileExplorerCommandError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            FileExplorerCommandError::UnknownCommand(id) => {
                write!(f, "Unknown File Explorer Command: {id}")
            }
            FileExplorerCommandError::ParseError(id) => {
                write!(f, "Unable to parse command id {id}")
            }
            FileExplorerCommandError::IoError(id, error) => {
                write!(
                    f,
                    "Encountered IO error during a File Explorer Command: {id} - {error}"
                )
            }
        }
    }
}

#[derive(Clone, Copy)]
pub enum FileExplorerNavigationDirection {
    Left,
    Right,
    Up,
    Down,
}

#[derive(Clone, Copy)]
pub enum FileExplorerCommand {
    NewFile,
    NewFolder,
    Reveal,
    OpenInTerminal,
    FindInFolder,
    Cut,
    Copy,
    Duplicate,
    Paste,
    CopyPath,
    CopyRelativePath,
    ShowHistory,
    Rename,
    Delete,
    Expand,
    CollapseAll,
    Navigation(FileExplorerNavigationDirection),
    ToggleSelect,
    Action,
    ActionIndex,
    ClearSelected,
    Move,
}

const ALL_IN_ORDER: &[FileExplorerCommand] = &[
    FileExplorerCommand::NewFile,
    FileExplorerCommand::NewFolder,
    FileExplorerCommand::Reveal,
    FileExplorerCommand::OpenInTerminal,
    FileExplorerCommand::FindInFolder,
    FileExplorerCommand::Cut,
    FileExplorerCommand::Copy,
    FileExplorerCommand::Duplicate,
    FileExplorerCommand::Paste,
    FileExplorerCommand::CopyPath,
    FileExplorerCommand::CopyRelativePath,
    FileExplorerCommand::ShowHistory,
    FileExplorerCommand::Rename,
    FileExplorerCommand::Delete,
    FileExplorerCommand::Expand,
    FileExplorerCommand::CollapseAll,
];

impl FileExplorerCommand {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::NewFile => "fileExplorer.newFile",
            Self::NewFolder => "fileExplorer.newFolder",
            Self::Reveal => "fileExplorer.reveal",
            Self::OpenInTerminal => "fileExplorer.openInTerminal",
            Self::FindInFolder => "fileExplorer.findInFolder",
            Self::Cut => "fileExplorer.cut",
            Self::Copy => "fileExplorer.copy",
            Self::Duplicate => "fileExplorer.duplicate",
            Self::Paste => "fileExplorer.paste",
            Self::CopyPath => "fileExplorer.copyPath",
            Self::CopyRelativePath => "fileExplorer.copyRelativePath",
            Self::ShowHistory => "fileExplorer.showHistory",
            Self::Rename => "fileExplorer.rename",
            Self::Delete => "fileExplorer.delete",
            Self::Expand => "fileExplorer.expand",
            Self::CollapseAll => "fileExplorer.collapseAll",
            Self::Navigation(direction) => match direction {
                FileExplorerNavigationDirection::Left => "fileExplorer.navigateLeft",
                FileExplorerNavigationDirection::Right => "fileExplorer.navigateRight",
                FileExplorerNavigationDirection::Up => "fileExplorer.navigateUp",
                FileExplorerNavigationDirection::Down => "fileExplorer.navigateDown",
            },
            Self::ToggleSelect => "fileExplorer.toggleSelect",
            Self::Action => "fileExplorer.action",
            Self::ActionIndex => "fileExplorer.actionIndex",
            Self::ClearSelected => "fileExplorer.clearSelected",
            Self::Move => "fileExplorer.move",
        }
    }

    fn should_include(cmd: FileExplorerCommand, ctx: &FileExplorerContext) -> bool {
        match cmd {
            // TODO(scarlet): Add these at a later date.
            FileExplorerCommand::OpenInTerminal | FileExplorerCommand::FindInFolder => false,

            FileExplorerCommand::ShowHistory => ctx.target_is_item && !ctx.item_is_directory,
            FileExplorerCommand::Delete => ctx.target_is_item,
            FileExplorerCommand::Expand => ctx.target_is_item && ctx.item_is_directory,
            FileExplorerCommand::CollapseAll => !ctx.target_is_item,
            _ => true,
        }
    }

    pub fn all(ctx: &FileExplorerContext) -> Vec<FileExplorerCommand> {
        ALL_IN_ORDER
            .iter()
            .copied()
            .filter(|cmd| Self::should_include(*cmd, ctx))
            .collect()
    }
}

impl FromStr for FileExplorerCommand {
    type Err = ();

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "fileExplorer.newFile" => Ok(Self::NewFile),
            "fileExplorer.newFolder" => Ok(Self::NewFolder),
            "fileExplorer.reveal" => Ok(Self::Reveal),
            "fileExplorer.openInTerminal" => Ok(Self::OpenInTerminal),
            "fileExplorer.findInFolder" => Ok(Self::FindInFolder),
            "fileExplorer.cut" => Ok(Self::Cut),
            "fileExplorer.copy" => Ok(Self::Copy),
            "fileExplorer.duplicate" => Ok(Self::Duplicate),
            "fileExplorer.paste" => Ok(Self::Paste),
            "fileExplorer.copyPath" => Ok(Self::CopyPath),
            "fileExplorer.copyRelativePath" => Ok(Self::CopyRelativePath),
            "fileExplorer.showHistory" => Ok(Self::ShowHistory),
            "fileExplorer.rename" => Ok(Self::Rename),
            "fileExplorer.delete" => Ok(Self::Delete),
            "fileExplorer.expand" => Ok(Self::Expand),
            "fileExplorer.collapseAll" => Ok(Self::CollapseAll),

            "fileExplorer.navigateLeft" => {
                Ok(Self::Navigation(FileExplorerNavigationDirection::Left))
            }
            "fileExplorer.navigateRight" => {
                Ok(Self::Navigation(FileExplorerNavigationDirection::Right))
            }
            "fileExplorer.navigateUp" => Ok(Self::Navigation(FileExplorerNavigationDirection::Up)),
            "fileExplorer.navigateDown" => {
                Ok(Self::Navigation(FileExplorerNavigationDirection::Down))
            }
            "fileExplorer.toggleSelect" => Ok(Self::ToggleSelect),
            "fileExplorer.action" => Ok(Self::Action),
            "fileExplorer.actionIndex" => Ok(Self::ActionIndex),
            "fileExplorer.clearSelected" => Ok(Self::ClearSelected),
            "fileExplorer.move" => Ok(Self::Move),
            _ => Err(()),
        }
    }
}

#[derive(Clone)]
pub struct PasteItem {
    pub path: PathBuf,
    pub is_dir: bool,
}

#[derive(Clone)]
pub struct PasteInfo {
    pub items: Vec<PasteItem>,
    pub is_cut_operation: bool,
}

pub struct FileExplorerContext {
    pub index: i64,
    pub item_path: PathBuf,
    pub target_is_item: bool,
    pub item_is_directory: bool,
    pub item_is_expanded: bool,
    pub paste_info: PasteInfo,
    pub move_target_node_path: PathBuf,
    pub move_destination_node_path: PathBuf,
}

#[derive(Default)]
pub struct FileExplorerCommandState {
    pub can_make_new_file: bool,
    pub can_make_new_folder: bool,
    pub can_reveal_in_system: bool,
    pub can_open_in_terminal: bool,

    pub can_find_in_folder: bool,

    pub can_cut: bool,
    pub can_copy: bool,
    pub can_duplicate: bool,
    pub can_paste: bool,

    pub can_copy_path: bool,
    pub can_copy_relative_path: bool,

    pub can_show_history: bool,

    pub can_rename: bool,
    pub can_delete: bool,

    pub can_expand_item: bool,
    pub can_collapse_item: bool,
    pub can_collapse_all: bool,
}

#[derive(Clone, Debug)]
pub enum FileExplorerUiIntent {
    DirectoryRefreshed { path: PathBuf },
    OpenFile { path: PathBuf },
}

#[derive(Clone, Debug)]
pub struct FileExplorerCommandResult {
    pub intents: Vec<FileExplorerUiIntent>,
}
