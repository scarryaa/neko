use crate::DocumentId;
use std::{fmt, io};

#[derive(Debug)]
pub enum DocumentError {
    Io(io::Error),
    InvalidId(u64),
    NoPath(DocumentId),
    NotFound(DocumentId),
}

impl fmt::Display for DocumentError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            DocumentError::Io(error) => write!(f, "IO error: {error}"),
            DocumentError::InvalidId(_) => write!(f, "Document id must not be 0"),
            DocumentError::NoPath(id) => write!(f, "Document {id:?} has no path"),
            DocumentError::NotFound(id) => write!(f, "Document {id:?} not found"),
        }
    }
}

impl std::error::Error for DocumentError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            DocumentError::Io(error) => Some(error),
            _ => None,
        }
    }
}

impl From<io::Error> for DocumentError {
    fn from(error: io::Error) -> Self {
        DocumentError::Io(error)
    }
}
