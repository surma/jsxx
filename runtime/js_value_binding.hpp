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

  JSValue get();
  void set_parent(JSValue parent);

  std::optional<shared_ptr<JSValue>> getter = std::nullopt;
  std::optional<shared_ptr<JSValue>> setter = std::nullopt;
  shared_ptr<JSValue> internal;
};
