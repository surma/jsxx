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

  JSValue b = JSON["stringify"]({a});
  WASI["write_to_stdout"]({JSValue{""} + b});
  // WASI["write_to_stdout"]({JSValue{""} + input});
  return 0;
}
