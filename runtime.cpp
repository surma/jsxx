#include <memory>

#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue a = JSValue::new_object({{JSValue{"test"}, JSValue{12.9}}});
  JSValue b{5.0};
  printf("%s\n", a["test"].get().coerce_to_string().c_str());
  a["test"] = JSValue{9.0};
  printf("%s\n", a["test"].get().coerce_to_string().c_str());
  printf("%s\n", b["plusOne"](JSValue{0.0}).coerce_to_string().c_str());

  return 0;
}
