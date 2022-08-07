#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "js_value.hpp"

using std::optional;
using std::shared_ptr;

class JSValue;

class JSValueBinding {
public:
  JSValueBinding();
  // JSValueBinding(JSValue val);

  static JSValueBinding with_value(JSValue val);

  void operator=(JSValue other);
  JSValue operator()(JSValue args...);

  JSValue &get();

  shared_ptr<JSValue> internal;
  optional<shared_ptr<JSValue>> parent_value = std::nullopt;
};
