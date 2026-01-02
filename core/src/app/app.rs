use crate::{
    Buffer, CloseTabOperationType, Config, ConfigManager, Document, DocumentError, DocumentId,
    DocumentManager, DocumentResult, Editor, FileSystemResult, FileTree, JumpHistory,
    MoveActiveTabResult, Tab, TabError, TabId, TabManager, View, ViewId, ViewManager,
};
use std::path::Path;

// TODO(scarlet): Make new tab + open file atomic? Or at least provide an atomic fn version
// TODO(scarlet): Add error types
/// Application state.
///
/// Acts primarily as a facade over [`TabManager`] and [`FileTree`],
/// delegating most tab and file-tree operations to those components.
#[derive(Debug)]
pub struct AppState {
    file_tree: FileTree,
    config_manager: *const ConfigManager,
    tab_manager: TabManager,
    document_manager: DocumentManager,
    view_manager: ViewManager,
    pub jump_history: JumpHistory,
}

impl AppState {
    // TODO(scarlet): Convert this result to e.g. AppStateError
    // TODO(scarlet): Convert the path arg to <P: AsRef<Path>>
    pub fn new(config_manager: &ConfigManager, root_path: Option<&str>) -> FileSystemResult<Self> {
        let file_tree = FileTree::new(root_path)?;

        let mut document_manager = DocumentManager::default();
        let mut tab_manager = TabManager::new();
        let mut view_manager = ViewManager::default();

        let (_doc_id, _tab_id, _view_id) = AppState::create_document_tab_and_view_impl(
            &mut document_manager,
            &mut tab_manager,
            &mut view_manager,
            None,
            true,
            true,
        );

        Ok(Self {
            file_tree,
            config_manager,
            tab_manager,
            document_manager,
            view_manager,
            jump_history: JumpHistory::default(),
        })
    }

    // Getters
    pub fn get_tabs(&self) -> &Vec<Tab> {
        self.tab_manager.get_tabs()
    }

    pub fn get_tab(&self, id: TabId) -> Result<&Tab, TabError> {
        self.tab_manager.get_tab(id)
    }

    pub fn get_active_tab_id(&self) -> TabId {
        self.tab_manager.get_active_tab_id()
    }

    pub fn get_close_tab_ids(
        &self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: Option<TabId>,
        close_pinned: bool,
    ) -> Result<Vec<TabId>, TabError> {
        self.tab_manager
            .get_close_tab_ids(operation_type, anchor_tab_id, close_pinned)
    }

    pub fn get_config_snapshot(&self) -> Config {
        unsafe { &*self.config_manager }.get_snapshot()
    }

    pub fn get_document_manager(&self) -> &DocumentManager {
        &self.document_manager
    }

    pub fn get_view_manager(&self) -> &ViewManager {
        &self.view_manager
    }

    pub fn active_view_id(&self) -> Option<ViewId> {
        self.view_manager.active_view()
    }

    pub fn set_active_view_id(&mut self, id: ViewId) {
        self.view_manager.set_active_view(id);
    }

    /// Helper to get the active view and its document.
    pub fn with_active_view<F, R>(&self, f: F) -> Option<R>
    where
        F: FnOnce(&View, &Document) -> R,
    {
        let view_id = self.view_manager.active_view()?;
        let view = self.view_manager.get_view(view_id)?;
        let document_id = view.document_id();
        let document = self.document_manager.get_document(document_id)?;

        Some(f(view, document))
    }

    /// Helper to run something on the active view and its document.
    pub fn with_active_view_mut<F, R>(&mut self, f: F) -> Option<R>
    where
        F: FnOnce(&mut View, &mut Document) -> R,
    {
        let view_id = self.view_manager.active_view()?;
        let view = self.view_manager.get_view_mut(view_id)?;
        let document_id = view.document_id();
        let document = self.document_manager.get_document_mut(document_id)?;

        Some(f(view, document))
    }

    /// Returns a reference to the current (active) [`Editor`].
    pub fn editor(&self) -> Option<&Editor> {
        let view_id = self.view_manager.active_view()?;
        let view = self.view_manager.get_view(view_id)?;
        Some(view.editor())
    }

    /// Returns a mutable reference to the current (active) [`Editor`].
    pub fn editor_mut(&mut self) -> Option<&mut Editor> {
        let view_id = self.view_manager.active_view()?;
        let view = self.view_manager.get_view_mut(view_id)?;
        Some(view.editor_mut())
    }

    /// Helper to run something on the active view's editor and the active document's buffer.
    pub fn with_editor_and_buffer_mut<F, R>(&mut self, f: F) -> Option<R>
    where
        F: FnOnce(&mut Editor, &mut Buffer) -> R,
    {
        self.with_active_view_mut(|view, document| f(view.editor_mut(), &mut document.buffer))
    }

    /// Helper to run something on the active document's buffer.
    pub fn with_buffer_mut<F, R>(&mut self, f: F) -> Option<R>
    where
        F: FnOnce(&mut Buffer) -> R,
    {
        self.with_active_view_mut(|_, document| f(&mut document.buffer))
    }

    // Setters
    // TODO(scarlet): Needs to handle closing the corresponding document/view
    pub fn close_tabs(
        &mut self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: TabId,
        close_pinned: bool,
    ) -> Result<Vec<TabId>, TabError> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;

        self.tab_manager
            .close_tabs(operation_type, anchor_tab_id, history_enabled, close_pinned)
    }

    /// Returns the tab id for the given path, reusing an existing tab if present,
    /// otherwise opening the document and creating a new tab.
    pub fn ensure_tab_for_path(
        &mut self,
        path: &Path,
        add_to_history: bool,
        // TODO(scarlet): Should not be a DocumentResult?
    ) -> Result<TabId, DocumentError> {
        // Open (or reuse) the document for the given path
        let document_id = self.document_manager.open_document(path)?;

        // Try to find an existing tab for the document
        if let Some(tab_id) = self.tab_manager.find_tab_by_document(document_id) {
            return Ok(tab_id);
        }

        // Otherwise, create a new tab for the document
        let tab_id = self
            .tab_manager
            .add_tab_for_document(document_id, add_to_history);

        Ok(tab_id)
    }

    // TODO(scarlet): Fix this fn and rename it
    pub fn move_active_tab_by(
        &mut self,
        _buffer: &mut Buffer,
        _delta: i64,
        _use_history: bool,
    ) -> MoveActiveTabResult {
        // let auto_reopen_closed_tabs_in_history = self
        //     .config_manager()
        //     .get_snapshot()
        //     .editor
        //     .auto_reopen_closed_tabs_in_history;
        //
        // let mut result = self.tab_manager.move_active_tab_by(
        //     delta,
        //     use_history,
        //     auto_reopen_closed_tabs_in_history,
        // );
        //
        // if result.reopened_tab {
        //     if let Some(path) = &result.reopen_path {
        //         let (new_id, _) = self.tab_manager.new_tab(false);
        //
        //         if self.open_file(buffer, &path.to_string_lossy()).is_ok() {
        //             let _ = self.tab_manager.set_active_tab(new_id);
        //
        //             if let Some(old_id) = result.found_id {
        //                 // Update the tab id in history
        //                 self.tab_manager
        //                     .get_history_manager_mut()
        //                     .remap_id(old_id, new_id);
        //             }
        //
        //             // Set the found_id to the new tab id
        //             result.found_id = Some(new_id);
        //
        //             // Restore cursors, selections for reopened tab
        //             if let Ok(tab) = self.tab_manager.get_tab_mut(new_id) {
        //                 let editor = tab.get_editor_mut();
        //
        //                 if let Some(ref cursors) = result.cursors {
        //                     editor.cursor_manager_mut().set_cursors(cursors.to_vec());
        //                 }
        //
        //                 if let Some(ref selections) = result.selections {
        //                     editor.selection_manager_mut().set_selection(selections);
        //                 }
        //             }
        //         } else {
        //             // Reopen failed, clear reopened flag
        //             result.reopened_tab = false;
        //             result.reopen_path = None;
        //             result.scroll_offsets = None;
        //             result.cursors = None;
        //             result.selections = None;
        //         }
        //     }
        // }
        //
        // result

        MoveActiveTabResult {
            found_id: Some(TabId::new(1).expect("Tab id should not be 0")),
            reopened_tab: false,
            scroll_offsets: None,
            cursors: None,
            selections: None,
            reopen_path: None,
        }
    }

    pub fn pin_tab(&mut self, id: TabId) -> Result<(), TabError> {
        self.tab_manager.pin_tab(id)
    }

    pub fn unpin_tab(&mut self, id: TabId) -> Result<(), TabError> {
        self.tab_manager.unpin_tab(id)
    }

    pub fn set_tab_scroll_offsets(
        &mut self,
        id: TabId,
        new_offsets: (i32, i32),
    ) -> Result<(), TabError> {
        self.tab_manager.set_tab_scroll_offsets(id, new_offsets)
    }

    // TODO(scarlet): Need to ensure document/view is switched too
    pub fn set_active_tab(&mut self, id: TabId) -> Result<(), TabError> {
        self.tab_manager.set_active_tab(id)
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), TabError> {
        self.tab_manager.move_tab(from, to)
    }

    fn create_document_tab_and_view_impl(
        document_manager: &mut DocumentManager,
        tab_manager: &mut TabManager,
        view_manager: &mut ViewManager,
        title: Option<String>,
        add_tab_to_history: bool,
        activate_view: bool,
    ) -> (DocumentId, TabId, ViewId) {
        let document_id = document_manager.new_document(title);
        let tab_id = tab_manager.add_tab_for_document(document_id, add_tab_to_history);

        let editor = Editor::default();
        let view_id = view_manager.create_view(document_id, editor);

        if activate_view {
            view_manager.set_active_view(view_id);
        }

        (document_id, tab_id, view_id)
    }

    /// Creates a new empty document, a tab for it, and a view with a new editor,
    /// optionally adding the tab to history and making the view active.
    ///
    /// This public-facing version of the function delegates to an impl variant that accepts the
    /// three needed managers as arguments (see [`Self::create_document_tab_and_view_impl`]).
    pub fn create_document_tab_and_view(
        &mut self,
        title: Option<String>,
        add_tab_to_history: bool,
        activate_view: bool,
    ) -> (DocumentId, TabId, ViewId) {
        Self::create_document_tab_and_view_impl(
            &mut self.document_manager,
            &mut self.tab_manager,
            &mut self.view_manager,
            title,
            add_tab_to_history,
            activate_view,
        )
    }

    pub fn open_document(&mut self, path: &Path) -> DocumentResult<DocumentId> {
        let new_document_id = self.document_manager.open_document(path)?;
        self.tab_manager.add_tab_for_document(new_document_id, true);

        Ok(new_document_id)
    }

    // TODO(scarlet): (Tab)HistoryManager needs to be relocated (maybe to the DocumentManager or AppState)?
    pub fn new_document(&mut self, add_tab_to_history: bool) -> DocumentId {
        let new_document_id = self.document_manager.new_document(None);
        self.tab_manager
            .add_tab_for_document(new_document_id, add_tab_to_history);

        new_document_id
    }

    pub fn save_document(&mut self, document_id: DocumentId) -> DocumentResult<()> {
        self.document_manager.save_document(document_id)?;

        if let Some(document) = self.document_manager.get_document(document_id) {
            if let Some(path) = &document.path {
                // Refresh config if tab with config path was saved in the editor
                if *path == ConfigManager::get_config_path() {
                    // TODO(scarlet): Run a file watcher or call reload before writing via
                    // update fn to accommodate external changes eventually
                    if let Err(e) = self.config_manager().reload_from_disk() {
                        eprintln!("Failed to reload config: {e}");
                    }
                }
            }
        }

        Ok(())
    }

    pub fn save_document_as(&mut self, document_id: DocumentId, path: &Path) -> DocumentResult<()> {
        if path.as_os_str().is_empty() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "Path cannot be empty",
            )
            .into());
        }

        self.document_manager.save_document_as(document_id, path)?;
        Ok(())
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
    }

    pub fn reveal_in_file_tree(&mut self, path: &str) {
        // Expand ancestors so the file becomes visible
        _ = self.file_tree.ensure_path_visible(path);

        // Mark it as current
        self.file_tree.set_current_path(path);
    }

    // Utility
    fn config_manager(&self) -> &ConfigManager {
        // SAFETY: C++ must ensure ConfigManager outlives AppState
        assert!(
            !self.config_manager.is_null(),
            "config_manager pointer is null"
        );
        unsafe { &*self.config_manager }
    }
}

#[cfg(test)]
mod test {
    // use super::*;

    // TODO(scarlet): Fix these (and add more)
    #[test]
    fn save_tab_returns_error_when_tabs_are_empty() {
        // let c = ConfigManager::new();
        // let b = Buffer::new();
        // let mut app = AppState::new(&c, Some("")).unwrap();
        //
        // let _ = app.close_tabs(CloseTabOperationType::Single, 0, false);
        // let result = app.save_tab(0, &b);
        //
        // assert!(result.is_err())
    }

    #[test]
    fn save_tab_returns_error_when_there_is_no_current_file() {
        // let c = ConfigManager::new();
        // let b = Buffer::new();
        // let mut app = AppState::new(&c, Some("")).unwrap();
        // let result = app.save_tab(0, &b);
        //
        // assert!(result.is_err())
    }

    #[test]
    fn save_tab_as_returns_error_when_provided_path_is_empty() {
        // let c = ConfigManager::new();
        // let b = Buffer::new();
        // let mut app = AppState::new(&c, Some("")).unwrap();
        // let result = app.save_tab_as(&b, 0, "");
        //
        // assert!(result.is_err())
    }

    #[test]
    fn save_tab_as_returns_error_when_tabs_are_empty() {
        // let c = ConfigManager::new();
        // let b = Buffer::new();
        // let mut app = AppState::new(&c, Some("")).unwrap();
        //
        // let _ = app.close_tabs(CloseTabOperationType::Single, 0, false);
        // let result = app.save_tab_as(&b, 0, "path");
        //
        // assert!(result.is_err())
    }

    #[test]
    fn open_file_returns_error_when_file_is_already_open() {
        // let c = ConfigManager::new();
        // let mut b = Buffer::new();
        // let mut app = AppState::new(&c, Some("")).unwrap();
        //
        // app.tab_manager.new_tab(true);
        // app.tab_manager.get_tabs_mut()[1].set_file_path(Some("test".to_string()));
        //
        // let result = app.open_file(&mut b, "test");
        // assert!(result.is_err())
    }
}
