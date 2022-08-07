#include "js_primitives.hpp"

static JSValue global_undefined;

JSBase::JSBase() {}

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValue &JSBase::get_property(JSValue key) {
  auto obj = std::find_if(this->properties.begin(), this->properties.end(),
                          [&](std::pair<JSValue, JSValue> &item) -> bool {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == this->properties.end()) {
    return global_undefined;
  }
  return (*obj).second;
}

JSBool::JSBool(bool v) : JSBase(), internal{v} {};

std::vector<std::pair<JSValue, JSValue>> JSNumber_prototype{
    {JSValue{"test"}, JSValue{9.0}}};
JSNumber::JSNumber(double v) : JSBase(), internal{v} {
  for (const auto &entry : JSNumber_prototype) {
    this->properties.push_back(entry);
  }
};

JSString::JSString(const char *v) : JSBase(), internal{std::string(v)} {};

JSString::JSString(std::string v) : JSBase(), internal{v} {};

JSArray::JSArray() : JSBase(){};
JSObject::JSObject() : JSBase(), internal{} {};
JSValue &JSObject::operator[](const JSValue idx) {
  auto obj = std::find_if(this->internal.begin(), this->internal.end(),
                          [=](std::pair<JSValue, JSValue> &item) {
                            return (item.first == idx).coerce_to_bool();
                          });
  if (obj == this->internal.end()) {
    return global_undefined;
  }
  return (*obj).second;
}

JSFunction::JSFunction(JSExternFunc f) : JSBase(), internal{f} {};
