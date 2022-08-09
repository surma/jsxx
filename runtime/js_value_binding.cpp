#include "js_value_binding.hpp"

JSValueBinding::JSValueBinding()
    : internal{shared_ptr<JSValue>{new JSValue{}}} {};

JSValueBinding JSValueBinding::with_value(JSValue val) {
  JSValueBinding b;
  b = val;
  return b;
}

void JSValueBinding::operator=(JSValue other) { this->get() = JSValue{other}; }
JSValue JSValueBinding::operator+(JSValue other) { return this->get() + other; }
JSValue JSValueBinding::operator+(JSValueBinding other) {
  return this->get() + other.get();
}

JSValue JSValueBinding::operator()(std::vector<JSValue> args) {
  JSValue thisArg = *this->parent_value.value_or(
      shared_ptr<JSValue>{new JSValue{JSValue::undefined()}});
  return this->internal->apply(thisArg, args);
}

JSValue &JSValueBinding::get() { return *this->internal; }
