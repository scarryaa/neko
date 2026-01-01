pub mod cursor;
pub mod document;
pub mod editor;
pub mod selection;
pub mod view;

pub use cursor::{CursorManager, types::*};
pub use document::{Document, DocumentManager, error::*, result::*, types::*};
pub use editor::{Buffer, Edit, Editor, Transaction, UndoHistory, ViewState};
pub(crate) use editor::{result::*, types::*};
pub use selection::{SelectionManager, types::*};
pub use view::{View, ViewId, ViewManager, Viewport};
