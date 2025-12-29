#[allow(clippy::module_inception)]
pub mod app;
mod document_manager;

pub use app::AppState;
pub use document_manager::DocumentManager;
