#include <memory>

#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue a = JSValue::new_object({
    {JSValue{"test"}, JSValue{12.9}}
  });
  JSValue b{5.0};
  printf("%s\n", a["test"].coerce_to_string().c_str());
  printf("%s %s %s\n", a.coerce_to_string().c_str(),
         b.coerce_to_string().c_str(), (a["test"] + b).coerce_to_string().c_str());

  return 0;
}
