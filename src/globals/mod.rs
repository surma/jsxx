pub mod io;
pub mod json;
pub mod symbol;

pub struct Global {
    pub name: String,
    pub additional_headers: Option<Vec<String>>,
    pub init: Option<String>,
    pub factory: String,
}
