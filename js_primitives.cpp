#include "js_primitives.hpp"

JSBase::JSBase() { this->init_properties(); }

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValue JSBase::operator[](JSValue &key) {
  auto obj = std::find_if(this->properties.begin(), this->properties.end(),
                          [&](std::pair<JSValue, JSValue> &item) -> bool {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == this->properties.end()) {
    return JSValue::undefined();
  }
  return (*obj).second;
}

void JSNumber::init_properties() {
  this->properties.push_back(std::pair{JSValue{"test"}, JSValue{9.0}});
}

JSBool::JSBool(bool v) : internal{v} {};
JSNumber::JSNumber(double v) : internal{v} {};
JSString::JSString(const char *v) : internal{std::string(v)} {};
JSString::JSString(std::string v) : internal{v} {};

JSArray::JSArray() { throw "Unimplemented"; }
JSObject::JSObject() : internal{} {};

JSFunction::JSFunction(JSExternFunc f) : internal{f} {};
