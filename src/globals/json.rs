use crate::globals::Global;

pub fn JSONGlobal() -> Global {
    Global {
        name: "JSON".into(),
        additional_headers: Some(vec!["runtime/global_json.hpp".into()]),
        init: None,
        factory: "create_JSON_global()".into(),
    }
}
