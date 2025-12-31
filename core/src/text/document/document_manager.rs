use crate::{Buffer, Document, DocumentError, DocumentId, FileIoManager};
use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
};

type Result<T> = std::result::Result<T, DocumentError>;

/// Manages a collection of open [`Document`]s.
///
/// `DocumentManager` is responsible for creating, tracking, and saving
/// documents.
#[derive(Debug)]
pub struct DocumentManager {
    documents: HashMap<DocumentId, Document>,
    next_document_id: DocumentId,
    path_index: HashMap<PathBuf, DocumentId>,
}

impl Default for DocumentManager {
    fn default() -> Self {
        Self::new()
    }
}

impl DocumentManager {
    pub fn new() -> Self {
        Self {
            documents: HashMap::new(),
            next_document_id: DocumentId(1),
            path_index: HashMap::new(),
        }
    }

    fn generate_next_id(&mut self) -> DocumentId {
        self.next_document_id.take_next()
    }

    /// Creates an empty [`Document`] and returns the corresponding [`DocumentId`].
    pub fn new_document(&mut self, title: String) -> DocumentId {
        let id = self.generate_next_id();
        let document = Document {
            id,
            path: None,
            title,
            buffer: Buffer::new(),
            modified: false,
        };

        self.documents.insert(id, document);

        id
    }

    /// Loads a file from disk and creates a new [`Document`], returning the corresponding
    /// [`DocumentId`].
    pub fn open_document(&mut self, path: &Path) -> Result<DocumentId> {
        let canon_path = fs::canonicalize(path)?;

        // If already open, reuse it
        if let Some(document_id) = self.path_index.get(&canon_path).copied() {
            return Ok(document_id);
        }

        // Read file content into a new document
        let file_content = FileIoManager::read(&canon_path)?;
        let document_id = self.generate_next_id();
        let document = Document {
            id: document_id,
            path: Some(canon_path.clone()),
            title: canon_path
                .file_name()
                .and_then(|name| name.to_str())
                .unwrap_or("Untitled")
                .to_string(),
            buffer: Buffer::from(&file_content),
            modified: false,
        };

        self.path_index.insert(canon_path, document_id);
        self.documents.insert(document_id, document);

        Ok(document_id)
    }

    pub fn get_document(&self, document_id: DocumentId) -> Option<&Document> {
        self.documents.get(&document_id)
    }

    pub fn get_document_mut(&mut self, document_id: DocumentId) -> Option<&mut Document> {
        self.documents.get_mut(&document_id)
    }

    pub fn mark_modified(&mut self, document_id: DocumentId) {
        if let Some(document) = self.documents.get_mut(&document_id) {
            document.modified = true;
        }
    }

    /// Attempts to save the [`Document`] with the provided [`DocumentId`] under the associated
    /// path.
    pub fn save_document(&mut self, document_id: DocumentId) -> Result<()> {
        let (path, content) = {
            let document = self
                .documents
                .get(&document_id)
                .expect("Invalid document id");

            let path = document
                .path
                .clone()
                .ok_or(DocumentError::NoPath(document_id))?;
            let content = document.buffer.get_text();

            (path, content)
        };

        FileIoManager::write(&path, &content)?;

        let document = self.documents.get_mut(&document_id).unwrap();
        document.modified = false;

        Ok(())
    }

    // TODO(scarlet): Handle the case where 'save as' occurs and the provided new path matches a different
    // stored Document's path.
    /// Attempts to save the [`Document`] with the provided [`DocumentId`] under the provided path.
    pub fn save_document_as(&mut self, document_id: DocumentId, new_path: &Path) -> Result<()> {
        let canon_new_path = FileIoManager::canonicalize(new_path)?;
        let content = {
            let document = self
                .documents
                .get(&document_id)
                .ok_or(DocumentError::NotFound(document_id))?;

            document.buffer.get_text()
        };

        FileIoManager::write(&canon_new_path, &content)?;

        let document = self.documents.get_mut(&document_id).unwrap();
        if let Some(old_path) = document.path.take() {
            self.path_index.remove(&old_path);
        }

        document.path = Some(canon_new_path.to_path_buf());
        document.title = canon_new_path
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or("Untitled")
            .to_string();
        document.modified = false;

        self.path_index
            .insert(canon_new_path.to_path_buf(), document_id);

        Ok(())
    }
}
