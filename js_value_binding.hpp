#pragma once

#include <memory>
#include <variant>

#include "js_value.hpp"

using std::shared_ptr;

class JSValue;

class JSValueBinding {
public:
  JSValueBinding();
  // JSValueBinding(JSValue val);

  static JSValueBinding with_value(JSValue val);

  void operator=(JSValue other);

  JSValue &get();

  shared_ptr<JSValue> internal;
};
