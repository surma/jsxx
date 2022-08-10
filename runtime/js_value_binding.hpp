#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "js_value.hpp"

using std::optional;
using std::shared_ptr;

class JSValue;

class JSValueBinding {
public:
  JSValueBinding();

  static JSValueBinding with_value(JSValue val);

  void operator=(JSValue other);

  JSValue &get();

  shared_ptr<JSValue> internal;
};
