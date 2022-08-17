
#include "global_json.hpp"

JSValue create_symbol_global() {
  JSValue global =
      JSValue::new_object({{JSValue{"iterator"}, iterator_symbol}});

  return global;
}
