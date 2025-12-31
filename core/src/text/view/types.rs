use crate::{DocumentId, Editor};

/// Represents the viewport associated with an open [`View`].
#[derive(Debug, Clone, Copy, Default)]
pub struct Viewport {
    pub first_visible_row: usize,
    pub visible_row_count: usize,
    pub horizontal_offset_cols: usize,
}

/// Represents the id associated with a given [`View`].
#[derive(Eq, Hash, PartialEq, Clone, Copy, Debug)]
pub struct ViewId(pub u64);

impl ViewId {
    pub fn take_next(&mut self) -> Self {
        let current_id = *self;
        self.0 = self.0.saturating_add(1);
        current_id
    }
}

/// Represents an open view in the app.
///
/// A `View` owns the [`Editor`] associated with the view, along with other metadata like the associated
/// [`DocumentId`] and [`Viewport`].
#[derive(Debug)]
pub struct View {
    id: ViewId,
    viewport: Viewport,
    document_id: DocumentId,
    editor: Editor,
}

impl View {
    pub fn new(id: ViewId, document_id: DocumentId, editor: Editor) -> Self {
        Self {
            id,
            viewport: Viewport::default(),
            document_id,
            editor,
        }
    }

    pub fn id(&self) -> ViewId {
        self.id
    }

    pub fn viewport(&self) -> Viewport {
        self.viewport
    }

    pub fn document_id(&self) -> DocumentId {
        self.document_id
    }

    pub fn editor(&self) -> &Editor {
        &self.editor
    }

    pub fn editor_mut(&mut self) -> &mut Editor {
        &mut self.editor
    }

    pub fn set_viewport(&mut self, viewport: Viewport) {
        self.viewport = viewport;
    }
}
