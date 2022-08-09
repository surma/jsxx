use crate::globals::Global;

pub fn WASIGlobal() -> Global {
    Global {
        name: "WASI".into(),
        additional_headers: Some(vec!["runtime/global_wasi.hpp".into()]),
        init: None,
        factory: "create_WASI_global()".into(),
    }
}
