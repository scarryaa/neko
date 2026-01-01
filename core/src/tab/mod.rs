pub mod closed_tab_store;
pub mod error;
pub mod history;
pub mod manager;
#[allow(clippy::module_inception)]
pub mod tab;
pub mod types;

use closed_tab_store::ClosedTabStore;
pub use error::*;
use history::TabHistoryManager;
pub use manager::TabManager;
pub use tab::Tab;
pub use types::{CloseTabOperationType, ClosedTabInfo, MoveActiveTabResult, ScrollOffsets};
