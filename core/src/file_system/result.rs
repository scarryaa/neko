use crate::FileSystemError;

pub type FileSystemResult<T> = Result<T, FileSystemError>;
