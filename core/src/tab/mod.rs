pub mod closed_tab_store;
pub mod history;
pub mod model;
pub mod tab_manager;

pub use closed_tab_store::{ClosedTabInfo, ClosedTabStore};
pub(crate) use model::Tab;
pub use tab_manager::TabManager;
