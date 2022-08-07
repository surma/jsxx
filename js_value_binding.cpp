#include "js_value_binding.hpp"

JSValueBinding::JSValueBinding()
    : internal{shared_ptr<JSValue>{new JSValue{}}} {};

JSValueBinding JSValueBinding::with_value(JSValue val) {
  JSValueBinding b;
  b = val;
  return b;
}
void JSValueBinding::operator=(JSValue other) { this->get() = JSValue{other}; }

JSValue &JSValueBinding::get() { return *this->internal; }
