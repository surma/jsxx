#include <memory>

#include "global_json.hpp"
#include "global_wasi.hpp"
#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue WASI = create_WASI_global();
  JSValue JSON = create_JSON_global();
  JSValue input{R"({
    "data": [1, 2, 3, 4],
    "add": 123
  })"};
  JSValue a = JSON["parse"]({input});


  // WASI["write_to_stdout"]({JSValue{""} + a.get_property(JSValue{"add"})});
  // WASI["write_to_stdout"]({JSValue{""} + a.get_property_slot(JSValue{"add"}).get()});
  a.get_property_slot(JSValue{"add"}) = JSValue{444.};

  JSValue b = JSON["stringify"]({a});
  WASI["write_to_stdout"]({JSValue{""} + b});
  // WASI["write_to_stdout"]({JSValue{""} + input});
  return 0;
}
