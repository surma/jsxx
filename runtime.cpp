#include <memory>

#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue a{4.0};
  JSValue b{5.0};
  printf("%s\n", a[JSValue{"test"}].coerce_to_string().c_str());
  printf("%s %s %s\n", a.coerce_to_string().c_str(),
         b.coerce_to_string().c_str(), (a + b).coerce_to_string().c_str());

  return 0;
}
