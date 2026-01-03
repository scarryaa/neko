use crate::{DocumentId, Editor, View, ViewError, ViewId, Viewport};
use std::collections::HashMap;

/// Manages a collection of current [`View`]s.
///
/// `ViewManager` is responsible for creating, mutating, and tracking views.
#[derive(Debug)]
pub struct ViewManager {
    views: HashMap<ViewId, View>,
    next_view_id: ViewId,
    active_view_id: Option<ViewId>,
}

impl Default for ViewManager {
    fn default() -> Self {
        Self::new()
    }
}

impl ViewManager {
    pub fn new() -> Self {
        Self {
            views: HashMap::new(),
            next_view_id: ViewId::new(1).expect("next_view_id cannot be 0"),
            active_view_id: None,
        }
    }

    fn generate_next_id(&mut self) -> ViewId {
        self.next_view_id.take_next()
    }

    pub fn get_view(&self, view_id: ViewId) -> Option<&View> {
        self.views.get(&view_id)
    }

    pub fn get_view_mut(&mut self, view_id: ViewId) -> Option<&mut View> {
        self.views.get_mut(&view_id)
    }

    pub fn active_view(&self) -> Option<ViewId> {
        self.active_view_id
    }

    pub fn set_active_view(&mut self, id: ViewId) -> Result<(), ViewError> {
        if self.views.contains_key(&id) {
            self.active_view_id = Some(id);
            Ok(())
        } else {
            Err(ViewError::NotFound(id))
        }
    }

    pub fn clear_active_view(&mut self) {
        self.active_view_id = None;
    }

    /// Checks if any remaining view is referencing the given document.
    pub fn has_views_for_document(&self, document_id: DocumentId) -> bool {
        self.views
            .values()
            .any(|view| view.document_id() == document_id)
    }

    /// Creates a new [`View`] and returns the corresponding [`ViewId`].
    pub fn create_view(&mut self, document_id: DocumentId, editor: Editor) -> ViewId {
        let id = self.generate_next_id();
        let view = View::new(id, document_id, editor);

        self.views.insert(id, view);

        id
    }

    /// Updates the [`Viewport`] of a given [`View`].
    pub fn update_viewport(&mut self, view_id: ViewId, viewport: Viewport) {
        if let Some(view) = self.views.get_mut(&view_id) {
            view.set_viewport(viewport);
        }
    }

    /// Removes a given [`View`] and returns it if it exists, or None.
    pub fn remove_view(&mut self, view_id: ViewId) -> Option<View> {
        self.views.remove(&view_id)
    }
}
