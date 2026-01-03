use crate::{
    Buffer, Change, ChangeSet, CloseTabOperationType, ClosedTabInfo, Config, ConfigManager,
    Document, DocumentError, DocumentId, DocumentManager, DocumentResult, Editor, FileSystemResult,
    FileTree, JumpHistory, MoveActiveTabResult, Tab, TabError, TabId, TabManager, View, ViewId,
    ViewManager,
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
        let mut ids = self.tab_manager.get_close_tab_ids(
            operation_type.clone(),
            anchor_tab_id,
            close_pinned,
        )?;

        // Filter out modified documents if needed
        if matches!(operation_type, CloseTabOperationType::Clean) {
            ids.retain(|&tab_id| !self.is_tab_modified(tab_id));
        }

        Ok(ids)
    }

    fn is_tab_modified(&self, tab_id: TabId) -> bool {
        if let Ok(tab) = self.tab_manager.get_tab(tab_id) {
            if let Some(document) = self.document_manager.get_document(tab.get_document_id()) {
                return document.modified;
            }
        }

        false
    }

    pub fn get_config_snapshot(&self) -> Config {
        unsafe { &*self.config_manager }.get_snapshot()
    }

    pub fn get_document_manager(&self) -> &DocumentManager {
        &self.document_manager
    }

    pub fn get_document_manager_mut(&mut self) -> &mut DocumentManager {
        &mut self.document_manager
    }

    pub fn get_view_manager(&self) -> &ViewManager {
        &self.view_manager
    }

    pub fn get_view_manager_mut(&mut self) -> &mut ViewManager {
        &mut self.view_manager
    }

    pub fn active_view_id(&self) -> Option<ViewId> {
        self.view_manager.active_view()
    }

    pub fn set_active_view_id(&mut self, id: ViewId) {
        // TODO(scarlet) handle errors
        _ = self.view_manager.set_active_view(id);
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

    /// Helper to run something on the specified view and its associated document.
    pub fn with_view_and_document_mut<F, R>(&mut self, view_id: ViewId, f: F) -> Option<R>
    where
        F: FnOnce(&mut View, &mut Document) -> R,
    {
        let view = self.view_manager.get_view_mut(view_id)?;
        let document_id = view.document_id();
        let document = self.document_manager.get_document_mut(document_id)?;

        Some(f(view, document))
    }

    /// Helper to query something on the specified view and its associated document.
    pub fn with_view_and_document<F, R>(&self, view_id: ViewId, f: F) -> Option<R>
    where
        F: FnOnce(&View, &Document) -> R,
    {
        let view = self.view_manager.get_view(view_id)?;
        let document_id = view.document_id();
        let document = self.document_manager.get_document(document_id)?;

        Some(f(view, document))
    }

    // Setters
    pub fn close_tabs(
        &mut self,
        operation_type: CloseTabOperationType,
        anchor_tab_id: TabId,
        close_pinned: bool,
    ) -> Result<Vec<TabId>, TabError> {
        let ids_to_close =
            self.get_close_tab_ids(operation_type.clone(), Some(anchor_tab_id), close_pinned)?;

        if ids_to_close.is_empty() {
            return Ok(vec![]);
        }

        let mut tabs_with_info = Vec::new();

        for &tab_id in &ids_to_close {
            let mut info = None;

            if let Ok(tab) = self.tab_manager.get_tab(tab_id) {
                // Only save info if we can resolve the Document and the View.
                if let Some(document) = self.document_manager.get_document(tab.get_document_id()) {
                    if let Some(path) = &document.path {
                        if let Some(view) = self.view_manager.get_view(tab.get_view_id()) {
                            let editor = view.editor();

                            let scroll_offsets = tab.get_scroll_offsets();
                            info = Some(ClosedTabInfo {
                                path: path.clone(),
                                scroll_offsets: (
                                    scroll_offsets.0 as usize,
                                    scroll_offsets.1 as usize,
                                ),
                                cursors: editor.cursors().to_vec(),
                                selections: editor.selection().clone(),
                            });
                        }
                    }
                }
            }

            tabs_with_info.push((tab_id, info));
        }

        let mut views_and_documents_to_close = Vec::new();
        for &tab_id in &ids_to_close {
            if let Ok(tab) = self.tab_manager.get_tab(tab_id) {
                views_and_documents_to_close.push((tab.get_view_id(), tab.get_document_id()));
            }
        }

        // Close the tabs.
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;

        let closed_ids = self
            .tab_manager
            .close_tabs(tabs_with_info, history_enabled)?;

        for (view_id, document_id) in views_and_documents_to_close {
            self.view_manager.remove_view(view_id);

            // Only close the document if no other views are pointing to it.
            if !self.view_manager.has_views_for_document(document_id) {
                self.document_manager.close_document(document_id);
            }
        }

        let active_tab_id = self.tab_manager.get_active_tab_id();
        // Check if there are any tabs left.
        if !self.tab_manager.get_tabs().is_empty() {
            // Sync the active view.
            if let Ok(tab) = self.tab_manager.get_tab(active_tab_id) {
                // TODO(scarlet) handle errors
                _ = self.view_manager.set_active_view(tab.get_view_id());
            }
        } else {
            self.view_manager.clear_active_view();
        }

        Ok(closed_ids)
    }

    /// Returns the tab id for the given path, reusing an existing tab if present,
    /// otherwise opening the document and creating a new tab.
    pub fn ensure_tab_for_path(
        &mut self,
        path: &Path,
        add_to_history: bool,
        // TODO(scarlet): Make a generic AppStateError type?
    ) -> Result<TabId, DocumentError> {
        // Open (or reuse) the document for the given path
        let document_id = self.document_manager.open_document(path)?;

        // Try to find an existing tab for the document
        if let Some(tab_id) = self.tab_manager.find_tab_by_document(document_id) {
            let existing_view_id = self.tab_manager.get_tab(tab_id).map(|t| t.get_view_id());

            if let Ok(view_id) = existing_view_id {
                // Try to set the active view
                if self.view_manager.set_active_view(view_id).is_ok() {
                    return Ok(tab_id);
                }

                eprintln!("Adding missing view for tab {tab_id}");

                let editor = Editor::default();
                let new_view_id = self.view_manager.create_view(document_id, editor);

                self.tab_manager
                    .update_tab_view_id(tab_id, new_view_id)
                    .map_err(|_| {
                        // TODO(scarlet): Fix this error type
                        DocumentError::NotFound(u64::from(tab_id).into())
                    })?;

                let _ = self.view_manager.set_active_view(new_view_id);

                return Ok(tab_id);
            }
        }

        // Otherwise, create a new tab/view/editor
        let editor = Editor::default();
        let view_id = self.view_manager.create_view(document_id, editor);
        let tab_id = self
            .tab_manager
            .add_tab_for_document(document_id, view_id, add_to_history);
        let _ = self.view_manager.set_active_view(view_id);

        Ok(tab_id)
    }

    // TODO(scarlet): Rename this function to be clearer about what it actually does, and to avoid
    // confusion with `move_tab`.
    pub fn move_active_tab_by(&mut self, delta: i64, use_history: bool) -> MoveActiveTabResult {
        let auto_reopen = self
            .config_manager()
            .get_snapshot()
            .editor
            .auto_reopen_closed_tabs_in_history;

        let document_manager = &self.document_manager;
        let tab_manager = &mut self.tab_manager;

        let mut result = tab_manager.move_active_tab_by(delta, use_history, auto_reopen, |path| {
            document_manager.find_document_id_by_path(path)
        });

        if result.reopened_tab {
            if let Some(path) = &result.reopen_path {
                // Ensure the tab exists.
                if let Ok(new_tab_id) = self.ensure_tab_for_path(path, false) {
                    _ = self.tab_manager.set_active_tab(new_tab_id);

                    if let Some(old_id) = result.found_id {
                        // Update the tab id in history.
                        self.tab_manager
                            .get_history_manager_mut()
                            .remap_id(old_id, new_tab_id);
                    }

                    // Set the found_id to the new tab id.
                    result.found_id = Some(new_tab_id);

                    // Restore cursors, selections for reopened tab.
                    if let Ok(tab) = self.tab_manager.get_tab_mut(new_tab_id) {
                        let view_id = tab.get_view_id();
                        if let Some(view) = self.view_manager.get_view_mut(view_id) {
                            let editor = view.editor_mut();

                            if let Some(ref cursors) = result.cursors {
                                editor.cursor_manager_mut().set_cursors(cursors.to_vec());
                            }

                            if let Some(ref selections) = result.selections {
                                editor.selection_manager_mut().set_selection(selections);
                            }

                            if let Some(ref scroll_offsets) = result.scroll_offsets {
                                let converted_scroll_offsets =
                                    (scroll_offsets.x as i32, scroll_offsets.y as i32);
                                tab.set_scroll_offsets(converted_scroll_offsets);
                            }
                        }
                    }
                } else {
                    // Failed to reopen file.
                    result.reopened_tab = false;
                    result.reopen_path = None;
                }
            }
        }

        // Sync the active view.
        if let Some(found_id) = result.found_id {
            if let Ok(tab) = self.tab_manager.get_tab(found_id) {
                _ = self.view_manager.set_active_view(tab.get_view_id());
            }
        }

        result
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

    pub fn set_active_tab(&mut self, id: TabId) -> Result<(), TabError> {
        self.tab_manager.set_active_tab(id)?;
        let view_id = self.tab_manager.get_tab(id)?.get_view_id();
        // TODO(scarlet): Handle errors
        _ = self.view_manager.set_active_view(view_id);

        Ok(())
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), TabError> {
        self.tab_manager.move_tab(from, to)
    }

    /// Executes an editor action on the specified view.
    pub fn apply_editor_action<F>(&mut self, view_id: ViewId, action: F) -> Option<ChangeSet>
    where
        F: FnOnce(&mut Editor, &mut Buffer) -> ChangeSet,
    {
        let view = self.view_manager.get_view_mut(view_id)?;
        let document_id = view.document_id();
        let document = self.document_manager.get_document_mut(document_id)?;
        let change_set = action(view.editor_mut(), &mut document.buffer);

        // Update document modified status
        if change_set.change.contains(Change::BUFFER) {
            let current_revision = view.editor().revision();

            if current_revision == document.saved_revision {
                document.modified = false;
            } else {
                let current_hash = document.buffer.checksum();
                document.modified = current_hash != document.saved_hash;
            }
        }

        Some(change_set)
    }

    fn create_document_tab_and_view_impl(
        document_manager: &mut DocumentManager,
        tab_manager: &mut TabManager,
        view_manager: &mut ViewManager,
        title: Option<String>,
        add_tab_to_history: bool,
        activate_view: bool,
    ) -> (DocumentId, TabId, ViewId) {
        let editor = Editor::default();
        let document_id = document_manager.new_document(title);
        let view_id = view_manager.create_view(document_id, editor);
        let tab_id = tab_manager.add_tab_for_document(document_id, view_id, add_tab_to_history);

        if activate_view {
            // TODO(scarlet): Handle errors
            _ = view_manager.set_active_view(view_id);
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
        let editor = Editor::default();
        let view_id = self.view_manager.create_view(new_document_id, editor);

        self.tab_manager
            .add_tab_for_document(new_document_id, view_id, true);
        // TODO(scarlet): Handle errors
        _ = self.view_manager.set_active_view(view_id);

        Ok(new_document_id)
    }

    // TODO(scarlet): (Tab)HistoryManager needs to be relocated (maybe to the DocumentManager or AppState)?
    pub fn new_document(&mut self, add_tab_to_history: bool) -> DocumentId {
        let new_document_id = self.document_manager.new_document(None);
        let editor = Editor::default();
        let view_id = self.view_manager.create_view(new_document_id, editor);

        self.tab_manager
            .add_tab_for_document(new_document_id, view_id, add_tab_to_history);
        _ = self.view_manager.set_active_view(view_id);

        new_document_id
    }

    pub fn save_document(&mut self, view_id: ViewId) -> DocumentResult<()> {
        let (document_id, current_revision) = {
            let view = self
                .view_manager
                .get_view(view_id)
                .ok_or(DocumentError::ViewNotFound)?;

            let document_id = view.document_id();
            let current_revision = view.editor().revision();

            (document_id, current_revision)
        };

        self.document_manager
            .save_document(document_id, current_revision)?;

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

    pub fn save_document_as(&mut self, view_id: ViewId, path: &Path) -> DocumentResult<()> {
        if path.as_os_str().is_empty() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "Path cannot be empty",
            )
            .into());
        }

        let (document_id, current_revision) = {
            let view = self
                .view_manager
                .get_view(view_id)
                .ok_or(DocumentError::ViewNotFound)?;

            (view.document_id(), view.editor().revision())
        };

        self.document_manager
            .save_document_as(document_id, path, current_revision)?;

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
