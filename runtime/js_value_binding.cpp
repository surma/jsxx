#include "js_value_binding.hpp"

JSValueBinding::JSValueBinding()
    : internal{shared_ptr<JSValue>{new JSValue{}}} {};

JSValueBinding JSValueBinding::with_value(JSValue val) {
  JSValueBinding b;
  b = val;
  return b;
}

void JSValueBinding::operator=(JSValue other) {
  if (this->setter.has_value()) {
    (this->setter.value())(*this, other);
    return;
  }
  *this->internal = JSValue{other};
}

JSValue JSValueBinding::get() {
  if (this->getter.has_value()) {
    return (*this->getter)(*this);
    // return (this->getter.value())(*this);
  }
  return *this->internal;
}

void JSValueBinding::set_parent(JSValue parent) {
  this->internal->parent_value =
      std::optional{shared_ptr<JSValue>{new JSValue{parent}}};
}
