use super::{Buffer, Cursor, Selection};

#[derive(Clone, Debug)]
pub enum Edit {
    Insert {
        pos: usize,
        text: String,
    },
    Delete {
        start: usize,
        end: usize,
        deleted: String,
    },
}

impl Edit {
    pub fn apply(&self, buffer: &mut Buffer) {
        match self {
            Edit::Insert { pos, text } => buffer.insert(*pos, text),
            Edit::Delete { start, end, .. } => _ = buffer.delete_range(*start, *end),
        }
    }

    pub fn invert(&self) -> Edit {
        match self {
            Edit::Insert { pos, text } => Edit::Delete {
                start: *pos,
                end: *pos + text.len(),
                deleted: text.clone(),
            },
            Edit::Delete { start, deleted, .. } => Edit::Insert {
                pos: *start,
                text: deleted.clone(),
            },
        }
    }
}

#[derive(Clone, Debug)]
pub struct ViewState {
    pub cursors: Vec<Cursor>,
    pub selection: Selection,
}

#[derive(Clone, Debug)]
pub struct Transaction {
    pub before: ViewState,
    pub after: ViewState,
    pub edits: Vec<Edit>,
}

#[derive(Default, Debug)]
pub struct UndoHistory {
    pub undo: Vec<Transaction>,
    pub redo: Vec<Transaction>,
    pub current: Option<Transaction>,
}

impl UndoHistory {
    pub fn begin(&mut self, before: ViewState) {
        if self.current.is_none() {
            self.current = Some(Transaction {
                before: before.clone(),
                after: before,
                edits: Vec::new(),
            });
        }
    }

    pub fn record(&mut self, edit: Edit) {
        if let Some(tx) = &mut self.current {
            tx.edits.push(edit);
        }
    }

    pub fn commit(&mut self, after: ViewState) {
        if let Some(mut tx) = self.current.take() {
            tx.after = after;

            if !tx.edits.is_empty() {
                self.undo.push(tx);
                self.redo.clear();
            }
        }
    }

    pub fn undo(&mut self) -> Option<Transaction> {
        self.undo.pop().inspect(|tx| {
            self.redo.push(tx.clone());
        })
    }

    pub fn redo(&mut self) -> Option<Transaction> {
        self.redo.pop().inspect(|tx| {
            self.undo.push(tx.clone());
        })
    }
}
