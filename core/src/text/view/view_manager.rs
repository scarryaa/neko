use super::{View, ViewId, Viewport};
use crate::{DocumentId, Editor};
use std::collections::HashMap;

/// Manages a collection of current [`View`]s.
///
/// `ViewManager` is responsible for creating, mutating, and tracking views.
pub struct ViewManager {
    views: HashMap<ViewId, View>,
    next_view_id: ViewId,
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
            next_view_id: ViewId(1),
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
