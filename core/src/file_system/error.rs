use std::{fmt, io, time::SystemTimeError};

#[derive(Debug)]
pub enum FileSystemError {
    Io(io::Error),
    MissingName,
    BadSystemTime(SystemTimeError),
}

impl fmt::Display for FileSystemError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            FileSystemError::Io(error) => write!(f, "IO error: {error}"),
            FileSystemError::MissingName => write!(f, "Missing name"),
            FileSystemError::BadSystemTime(error) => write!(f, "Bad system time: {error}"),
        }
    }
}

impl std::error::Error for FileSystemError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            FileSystemError::Io(error) => Some(error),
            FileSystemError::BadSystemTime(error) => Some(error),
            FileSystemError::MissingName => None,
        }
    }
}

impl From<io::Error> for FileSystemError {
    fn from(error: io::Error) -> Self {
        FileSystemError::Io(error)
    }
}
