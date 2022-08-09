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
  // JSValueBinding(JSValue val);

  static JSValueBinding with_value(JSValue val);

  void operator=(JSValue other);
  JSValue operator()(std::vector<JSValue> args);
  JSValueBinding operator[](const JSValue index);
  JSValue operator+(JSValue other);
  JSValue operator+(JSValueBinding other);

  JSValue &get();

  shared_ptr<JSValue> internal;
  optional<shared_ptr<JSValue>> parent_value = std::nullopt;
};
