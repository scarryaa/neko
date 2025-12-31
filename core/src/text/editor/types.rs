bitflags::bitflags! {
    #[derive(Default, Debug, Clone, Copy)]
    pub(super) struct Change: u32 {
        const NONE       = 0;
        const BUFFER     = 1 << 0; // Text changed
        const CURSOR     = 1 << 1; // Cursor moved
        const SELECTION  = 1 << 2; // Selection changed
        const LINE_COUNT = 1 << 3; // Line count changed
        const WIDTHS     = 1 << 4; // Line widths invalidated
        const VIEWPORT   = 1 << 5; // Scroll/Viewport may need updates
    }
}

#[derive(Default, Debug, Clone)]
pub(crate) struct ChangeSet {
    pub change: Change,
    pub line_count_before: usize,
    pub line_count_after: usize,
    pub dirty_first_row: Option<usize>,
    pub dirty_last_row: Option<usize>,
}

pub(super) enum OpFlags {
    ViewportOnly,
    BufferViewportWidths,
}

pub(super) enum SelectionMode {
    Clear,
    Extend,
    Keep,
}

pub(super) enum CursorMode {
    Single,
    Multiple,
}

pub(super) enum DeleteResult {
    Text {
        invalidate: Option<usize>,
    },
    Newline {
        row: usize,
        invalidate: Option<usize>,
    },
}
