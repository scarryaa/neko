use crate::DocumentError;

pub type DocumentResult<T> = std::result::Result<T, DocumentError>;
