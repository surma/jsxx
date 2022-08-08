#include <memory>

#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue a = JSValue::new_object({{JSValue{"test"}, JSValue{12.9}}});
  JSValue b = JSValue::new_array({JSValue{1.0}, JSValue{2.0}});
  printf("%s\n", a["test"].get().coerce_to_string().c_str());
  printf("%s\n", b["map"](JSValue::new_function(
                     [](JSValue thisArg, std::vector<JSValue> &args) {
                       return JSValue{args[0].coerce_to_double() + 42.0};
                     }))["join"](JSValue{","})
                     .coerce_to_string()
                     .c_str());

  return 0;
}
