#pragma once

#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include "js_value.hpp"

using std::optional;
using std::shared_ptr;

class JSValue;
class JSValueBinding;

using Getter = std::function<JSValue(JSValueBinding)>;
using Setter = std::function<void(JSValueBinding, JSValue)>;

class JSValueBinding {
public:
  JSValueBinding();

  static JSValueBinding with_value(JSValue val);
  static JSValueBinding with_getter_setter(JSValue getter, JSValue setter);

  void operator=(JSValue other);

  JSValue get();
  void set_parent(JSValue parent);
  JSValue get_parent();

  std::optional<Getter> getter = std::nullopt;
  std::optional<Setter> setter = std::nullopt;
  shared_ptr<JSValue> internal;
};
