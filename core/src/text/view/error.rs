use std::fmt;

#[derive(Debug)]
pub enum ViewError {
    InvalidId(u64),
}

impl fmt::Display for ViewError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ViewError::InvalidId(_) => write!(f, "View id must not be 0"),
        }
    }
}
