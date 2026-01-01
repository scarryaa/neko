pub mod buffer;
pub mod edit_ops;
#[allow(clippy::module_inception)]
pub mod editor;
pub mod history;
pub mod result;
pub mod types;
pub mod width_manager;

pub use buffer::Buffer;
pub use editor::Editor;
pub use history::*;
use result::*;
pub(crate) use types::*;
pub use width_manager::WidthManager;
