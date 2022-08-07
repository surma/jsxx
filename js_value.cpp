#include "js_value.hpp"
#include <cmath>

JSValue::JSValue()
    : internal{new Box{std::in_place_index<JSValueType::UNDEFINED>,
                       JSUndefined{}}} {};
// Copy
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

JSValue JSValue::undefined() { return JSValue{}; }
JSValue JSValue::new_object(std::vector<std::pair<JSValue, JSValue>> pairs) {
  shared_ptr<JSObject> obj{new JSObject()};

  for (const auto &pair : pairs) {
    obj->internal.push_back(
        {pair.first, JSValueBinding::with_value(pair.second)});
  }
  JSValue val{};
  val.internal->emplace<JSValueType::OBJECT>(obj);
  return val;
}

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

JSValueBinding JSValue::operator[](const JSValue index) {
  if (this->type() == JSValueType::ARRAY &&
      index.type() == JSValueType::NUMBER) {
    return JSValueBinding::with_value(JSValue::undefined());
  }
  JSValueBinding vb;
  if (this->type() == JSValueType::OBJECT) {
    shared_ptr<JSObject> obj = std::get<JSValueType::OBJECT>(*this->internal);
    vb = (*obj)[index];
  } else {
    vb = this->get_property(index);
  }
  vb.parent_value = {shared_ptr<JSValue>{new JSValue{*this}}};
  return vb;
}

JSValueBinding JSValue::operator[](const char *index) {
  return (*this)[JSValue{index}];
}

JSValueBinding JSValue::get_property(const JSValue key) {
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    return JSValueBinding::with_value(JSValue::undefined());
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->internal).get_property(key);
  case JSValueType::NUMBER:
    return std::get<JSValueType::NUMBER>(*this->internal).get_property(key);
  case JSValueType::STRING:
    return std::get<JSValueType::STRING>(*this->internal).get_property(key);
  case JSValueType::ARRAY:
    return std::get<JSValueType::ARRAY>(*this->internal)->get_property(key);
  case JSValueType::OBJECT:
    return std::get<JSValueType::OBJECT>(*this->internal)->get_property(key);
  case JSValueType::EXCEPTION:
    return std::get<JSValueType::EXCEPTION>(*this->internal)->get_property(key);
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
  case JSValueType::EXCEPTION:
    return true; // FIXME
  }
  return "?";
}
