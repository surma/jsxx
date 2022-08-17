#include "js_value_binding.hpp"

JSValueBinding::JSValueBinding()
    : internal{shared_ptr<JSValue>{new JSValue{}}} {};

JSValueBinding JSValueBinding::with_value(JSValue val) {
  JSValueBinding b;
  b = val;
  return b;
}

JSValueBinding JSValueBinding::with_getter_setter(JSValue getter,
                                                  JSValue setter) {
  JSValueBinding b;
  b = JSValue::undefined();
  b.getter = std::optional{[=](JSValueBinding b) {
    if (getter.type() != JSValueType::FUNCTION)
      return JSValue::undefined();
    auto f = std::get<JSValueType::FUNCTION>(*getter.value);
    std::vector<JSValue> params{};
    return f.call(b.get_parent(), params);
  }};
  b.setter = std::optional{[=](JSValueBinding b, JSValue v) {
    if (setter.type() != JSValueType::FUNCTION)
      return;
    auto f = std::get<JSValueType::FUNCTION>(*setter.value);
    std::vector<JSValue> params{v};
    f.call(b.get_parent(), params);
  }};
  return b;
}

void JSValueBinding::operator=(JSValue other) {
  if (this->setter.has_value()) {
    (*this->setter)(*this, other);
    return;
  }
  *this->internal = JSValue{other};
}

JSValue JSValueBinding::get() {
  if (this->getter.has_value()) {
    return (*this->getter)(*this);
  }
  return *this->internal;
}

void JSValueBinding::set_parent(JSValue parent) {
  this->internal->parent_value =
      std::optional{shared_ptr<JSValue>{new JSValue{parent}}};
}

JSValue JSValueBinding::get_parent() {
  if (!this->internal->parent_value.has_value())
    return JSValue::undefined();
  return *this->internal->parent_value.value();
}
