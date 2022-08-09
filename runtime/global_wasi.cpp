
#include "global_wasi.hpp"

#include <unistd.h>
#include <vector>

static JSValue write_to_stdout(JSValue thisArg, std::vector<JSValue> &args) {
  JSValue data = args[0];
  if (data.type() != JSValueType::STRING)
    return JSValue::undefined();
  std::string str = data.coerce_to_string();
  write(1, str.c_str(), str.size());
  return JSValue{true};
}

static JSValue read_from_stdin(JSValue thisArg, std::vector<JSValue> &args) {
  char buf[1024];
  std::string input{};
  while (true) {
    auto n = read(0, buf, 1023);
    buf[n] = 0;
    input += std::string{buf};
    if (n < 1023)
      break;
  };
  return JSValue{input};
}

JSValue create_WASI_global() {
  JSValue global = JSValue::new_object(
      {{JSValue{"read_from_stdin"}, JSValue::new_function(read_from_stdin)},
       {JSValue{"write_to_stdout"}, JSValue::new_function(write_to_stdout)}});

  return global;
}
