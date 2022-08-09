#include <memory>

#include "global_json.hpp"
#include "global_wasi.hpp"
#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue WASI = create_WASI_global();
  JSValue JSON = create_JSON_global();
  JSValue a = JSON["parse"]({JSValue{"{\"a\": [1, 2, 123]}"}});
  WASI["write_to_stdout"]({JSValue{""} + a["a"].get()[JSValue{0.0}].get()});
  return 0;
}
