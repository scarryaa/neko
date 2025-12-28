use super::Buffer;

pub fn create_buffer_from(content: &str) -> Buffer {
    Buffer::from(content)
}

pub fn create_empty_buffer() -> Buffer {
    Buffer::new()
}
