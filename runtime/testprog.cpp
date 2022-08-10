#include <memory>

#include "global_json.hpp"
#include "global_wasi.hpp"
#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue nested = JSValue::new_object({
    {JSValue{"test"}, JSValue::new_function([](JSValue thisArg, std::vector<JSValue>& args) {
      return thisArg["marker"];
    })},
    {JSValue{"marker"}, JSValue{"nested"}}
  });
  JSValue input = JSValue::new_object({
    {JSValue{"nested"}, nested},
    {JSValue{"marker"}, JSValue{"input"}}
  });
  JSValue a = input["nested"]["test"]({});
  printf(">> %s\n", a.coerce_to_string().c_str());


  // WASI["write_to_stdout"]({JSValue{""} + a.get_property(JSValue{"add"})});
  // WASI["write_to_stdout"]({JSValue{""} + a.get_property_slot(JSValue{"add"}).get()});
  // a.get_property_slot(JSValue{"add"}) = JSValue{444.};

  // JSValue b = JSON["stringify"]({a});
  // WASI["write_to_stdout"]({JSValue{""} + b});
  // WASI["write_to_stdout"]({JSValue{""} + input});
  return 0;
}
