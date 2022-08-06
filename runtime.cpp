#include <memory>

#include "js_value.hpp"

using std::shared_ptr;

int main() {
  // JSValue a{new JSObject{}};
  // JSValue k1{"hello"};
  // JSValue k2{"hell2"};
  // a[k1] = JSValue{1.0};
  // a[k2] = JSValue{2.0};

  // a.
  shared_ptr<JSValue> a{new JSValue{4.0}};
  shared_ptr<JSValue> b{new JSValue{5.0}};
  printf("%s %s %s\n", a->coerce_to_string().c_str(),
         b->coerce_to_string().c_str(), (*a + *b).coerce_to_string().c_str());

  return 0;
}
