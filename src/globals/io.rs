use crate::globals::Global;

pub fn io_global() -> Global {
    Global {
        name: "IO".into(),
        additional_headers: Some(vec!["runtime/global_io.hpp".into()]),
        init: None,
        factory: "create_IO_global()".into(),
    }
}
