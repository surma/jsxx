#include <memory>

#include "global_wasi.hpp"
#include "js_value.hpp"

using std::shared_ptr;

int main() {
  JSValue WASI = create_WASI_global();
  JSValue a = WASI["stdin"].get() + JSValue{"!!!\n"};
  WASI["write_to_stdout"]({a});
  return 0;
}
