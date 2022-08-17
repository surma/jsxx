use crate::globals::Global;

pub fn symbol_global() -> Global {
    Global {
        name: "Symbol".into(),
        additional_headers: Some(vec!["runtime/global_symbol.hpp".into()]),
        init: None,
        factory: "create_symbol_global()".into(),
    }
}
