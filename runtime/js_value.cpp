#include "js_value.hpp"
#include <cmath>

JSValue::JSValue()
    : internal{new Box{std::in_place_index<JSValueType::UNDEFINED>,
                       JSUndefined{}}} {};

JSValue::JSValue(const JSValue &v) : internal{new Box{*v.internal}} {};

JSValue::JSValue(bool v)
    : internal{new Box{std::in_place_index<JSValueType::BOOL>, JSBool{v}}} {};

JSValue::JSValue(JSBool v)
    : internal{new Box{std::in_place_index<JSValueType::BOOL>, v}} {};

JSValue::JSValue(double v)
    : internal{
          new Box{std::in_place_index<JSValueType::NUMBER>, JSNumber{v}}} {};

JSValue::JSValue(JSNumber v)
    : internal{new Box{std::in_place_index<JSValueType::NUMBER>, v}} {};

JSValue::JSValue(const char *v)
    : internal{
          new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}} {};

JSValue::JSValue(std::string v)
    : internal{
          new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}} {};

JSValue::JSValue(JSString v)
    : internal{new Box{std::in_place_index<JSValueType::STRING>, v}} {};

JSValue::JSValue(JSFunction v)
    : internal{new Box{std::in_place_index<JSValueType::FUNCTION>, v}} {};

JSValue::JSValue(JSObject v)
    : internal{new Box{std::in_place_index<JSValueType::OBJECT>,
                       shared_ptr<JSObject>{new JSObject{v}}}} {};

JSValue::JSValue(JSArray v)
    : internal{new Box{std::in_place_index<JSValueType::ARRAY>,
                       shared_ptr<JSArray>{new JSArray{v}}}} {};

JSValue::JSValue(JSValueBinding v) : internal{new Box{*v.get().internal}} {};

JSValue JSValue::undefined() { return JSValue{}; }
JSValue JSValue::new_object(std::vector<std::pair<JSValue, JSValue>> pairs) {
  return JSValue{JSObject{pairs}};
}

JSValue JSValue::new_array(std::vector<JSValue> values) {
  return JSValue{JSArray{values}};
}

JSValue JSValue::new_function(ExternFunc f) { return JSValue{JSFunction{f}}; }

JSValue JSValue::operator=(JSValue other) {
  this->internal = unique_ptr<JSValue::Box>{new JSValue::Box{*other.internal}};
  return other;
}

// JSValue& JSValue::operator=(JSValue& other) {
//   this->internal = unique_ptr<JSValue::Box>{new
//   JSValue::Box{other.internal.get()}}; return other;
// }

JSValue JSValue::operator==(const JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return new JSValue{
        std::get<JSValueType::NUMBER>(*this->internal).internal ==
        other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->internal).internal ==
                   other.coerce_to_string()};
  }
  return JSValue{"Equality not implemented for this type yet"};
}

JSValue JSValue::operator+(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal +
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->internal).internal +
                   other.coerce_to_string()};
  }
  return new JSValue{"Addition not implemented for this type yet"};
}

JSValue JSValue::operator*(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal *
                   other.coerce_to_double()};
  }
  return new JSValue{"Multiplication not implemented for this type yet"};
}

JSValue JSValue::operator[](const JSValue key) {
  return this->get_property(key);
}

JSValue JSValue::operator[](const char *index) {
  return (*this)[JSValue{index}];
}

JSValue JSValue::operator[](const size_t index) {
  return (*this)[JSValue{static_cast<double>(index)}];
}

JSValue JSValue::operator()(std::vector<JSValue> args) {
  return this->apply(JSValue::undefined(), args);
}

JSValue JSValue::get_property(const JSValue key) {
  return this->get_property_slot(key).get();
}

JSValueBinding JSValue::get_property_slot(const JSValue key) {
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    return JSValueBinding::with_value(JSValue::undefined());
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->internal).get_property_slot(key);
  case JSValueType::NUMBER:
    return std::get<JSValueType::NUMBER>(*this->internal).get_property_slot(key);
  case JSValueType::STRING:
    return std::get<JSValueType::STRING>(*this->internal).get_property_slot(key);
  case JSValueType::ARRAY:
    return std::get<JSValueType::ARRAY>(*this->internal)->get_property_slot(key);
  case JSValueType::OBJECT:
    return std::get<JSValueType::OBJECT>(*this->internal)->get_property_slot(key);
  case JSValueType::FUNCTION:
    return std::get<JSValueType::FUNCTION>(*this->internal).get_property_slot(key);
  case JSValueType::EXCEPTION:
    return std::get<JSValueType::EXCEPTION>(*this->internal)->get_property_slot(key);
  }
}

JSValueType JSValue::type() const {
  return static_cast<JSValueType>(this->internal->index());
}

bool JSValue::is_undefined() const {
  return this->type() == JSValueType::UNDEFINED;
}

double JSValue::coerce_to_double() const {
  switch (this->type()) {
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->internal).internal ? 1 : 0;
  case JSValueType::NUMBER:
    return std::get<JSValueType::NUMBER>(*this->internal).internal;
  case JSValueType::STRING:
    return std::stod(std::get<JSValueType::STRING>(*this->internal).internal);
  default:
    return NAN;
  }
}

std::string JSValue::coerce_to_string() const {
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    return "undefined";
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->internal).internal
               ? std::string{"true"}
               : std::string{"false"};
  case JSValueType::NUMBER:
    return std::to_string(
        std::get<JSValueType::NUMBER>(*this->internal).internal);
  case JSValueType::STRING:
    return std::get<JSValueType::STRING>(*this->internal).internal;
  case JSValueType::ARRAY:
    return "[Array]"; // FIXME
  case JSValueType::OBJECT:
    return "[Object object]"; // FIXME?
  case JSValueType::FUNCTION:
    return "<function>"; // FIXME?
  case JSValueType::EXCEPTION:
    return "[EXCEPTION]"; // FIXME?
  }
  return "?";
}

bool JSValue::coerce_to_bool() const {
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    return false;
  case JSValueType::BOOL:
    return std::get<JSBool>(*this->internal).internal;
  case JSValueType::NUMBER:
    return std::get<JSNumber>(*this->internal).internal > 0;
  case JSValueType::STRING:
    return std::get<JSString>(*this->internal).internal.length() > 0;
  case JSValueType::ARRAY:
    return false; // FIXME
  case JSValueType::OBJECT:
    return true; // FIXME
  case JSValueType::FUNCTION:
    return true; // FIXME
  case JSValueType::EXCEPTION:
    return true; // FIXME
  }
  return "?";
}

JSValue JSValue::apply(JSValue thisArg, std::vector<JSValue> args) {
  if (this->type() != JSValueType::FUNCTION) {
    return JSValue::undefined(); // FIXME
  }
  JSFunction f = std::get<JSValueType::FUNCTION>(*this->internal);
  return f.call(thisArg, args);
}
