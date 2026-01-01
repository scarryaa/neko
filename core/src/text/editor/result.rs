pub(super) enum DeleteResult {
    Text {
        invalidate: Option<usize>,
    },
    Newline {
        row: usize,
        invalidate: Option<usize>,
    },
}
