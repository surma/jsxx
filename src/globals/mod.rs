pub mod json;
pub mod wasi;

pub struct Global {
    pub name: String,
    pub additional_headers: Option<Vec<String>>,
    pub init: Option<String>,
    pub factory: String,
}
