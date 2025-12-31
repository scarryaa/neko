pub mod closed_tab_store;
pub mod history;
#[allow(clippy::module_inception)]
pub mod tab;
pub mod tab_manager;
pub mod types;

use closed_tab_store::ClosedTabStore;
use history::TabHistoryManager;
pub use tab::Tab;
pub use tab_manager::TabManager;
pub use types::{
    CloseTabOperationType, ClosedTabInfo, MoveActiveTabResult, ScrollOffsets, TabResult,
};
