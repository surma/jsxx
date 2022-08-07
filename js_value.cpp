#include "js_value.hpp"
#include <cmath>

static JSValue global_undefined{};

JSValue::JSValue() : internal{new Box{JSUndefined{}}} {};
// Copy
JSValue::JSValue(const JSValue &v) : internal{new Box{*v.internal}} {};

JSValue::JSValue(bool v) : internal{new Box{JSBool{v}}} {};
JSValue::JSValue(JSBool v) : internal{new Box{v}} {};
JSValue::JSValue(double v) : internal{new Box{JSNumber{v}}} {};
JSValue::JSValue(JSNumber v) : internal{new Box{v}} {};
JSValue::JSValue(const char *v) : internal{new Box{JSString{v}}} {};
JSValue::JSValue(std::string v) : internal{new Box{JSString{v}}} {};
JSValue::JSValue(JSString v) : internal{new Box{v}} {};

JSValue JSValue::undefined() { return JSValue{}; }
JSValue JSValue::new_object(std::vector<std::pair<JSValue, JSValue>> pairs) {
  shared_ptr<JSObject> obj{new JSObject()};

  for (const auto &pair : pairs) {
    obj->internal.push_back(pair);
  }
  JSValue val{};
  val.internal->emplace<JSValueInternalIndex::OBJECT>(obj);
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
  if (this->type() == JSValueInternalIndex::NUMBER) {
    return new JSValue{
        std::get<JSValueInternalIndex::NUMBER>(*this->internal).internal ==
        other.coerce_to_double()};
  }
  if (this->type() == JSValueInternalIndex::STRING) {
    return JSValue{
        std::get<JSValueInternalIndex::STRING>(*this->internal).internal ==
        other.coerce_to_string()};
  }
  return JSValue{"Equality not implemented for this type yet"};
}

JSValue JSValue::operator+(JSValue other) {
  if (this->type() == JSValueInternalIndex::NUMBER) {
    return JSValue{
        std::get<JSValueInternalIndex::NUMBER>(*this->internal).internal +
        other.coerce_to_double()};
  }
  if (this->type() == JSValueInternalIndex::STRING) {
    return JSValue{
        std::get<JSValueInternalIndex::STRING>(*this->internal).internal +
        other.coerce_to_string()};
  }
  return new JSValue{"Addition not implemented for this type yet"};
}

JSValue &JSValue::operator[](const JSValue index) {
  if (this->type() == JSValueInternalIndex::ARRAY &&
      index.type() == JSValueInternalIndex::NUMBER) {
    return global_undefined;
  }
  if (this->type() == JSValueInternalIndex::OBJECT) {
    shared_ptr<JSObject> obj =
        std::get<JSValueInternalIndex::OBJECT>(*this->internal);
    return (*obj)[index];
  }
  return this->get_property(index);
}

JSValue &JSValue::operator[](const char *index) {
  return (*this)[JSValue{index}];
}

JSValue &JSValue::get_property(const JSValue key) {
  switch (this->type()) {
  case JSValueInternalIndex::UNDEFINED:
    return global_undefined;
  case JSValueInternalIndex::BOOL:
    return std::get<JSValueInternalIndex::BOOL>(*this->internal)
        .get_property(key);
  case JSValueInternalIndex::NUMBER:
    return std::get<JSValueInternalIndex::NUMBER>(*this->internal)
        .get_property(key);
  case JSValueInternalIndex::STRING:
    return std::get<JSValueInternalIndex::STRING>(*this->internal)
        .get_property(key);
  case JSValueInternalIndex::ARRAY:
    return std::get<JSValueInternalIndex::ARRAY>(*this->internal)
        ->get_property(key);
  case JSValueInternalIndex::OBJECT:
    return std::get<JSValueInternalIndex::OBJECT>(*this->internal)
        ->get_property(key);
  case JSValueInternalIndex::EXCEPTION:
    return std::get<JSValueInternalIndex::EXCEPTION>(*this->internal)
        ->get_property(key);
  }
}

JSValueInternalIndex JSValue::type() const {
  return static_cast<JSValueInternalIndex>(this->internal->index());
}

bool JSValue::is_undefined() const {
  return this->type() == JSValueInternalIndex::UNDEFINED;
}

double JSValue::coerce_to_double() const {
  switch (this->type()) {
  case JSValueInternalIndex::BOOL:
    return std::get<JSValueInternalIndex::BOOL>(*this->internal).internal ? 1
                                                                          : 0;
  case JSValueInternalIndex::NUMBER:
    return std::get<JSValueInternalIndex::NUMBER>(*this->internal).internal;
  case JSValueInternalIndex::STRING:
    return std::stod(
        std::get<JSValueInternalIndex::STRING>(*this->internal).internal);
  default:
    return NAN;
  }
}

std::string JSValue::coerce_to_string() const {
  switch (this->type()) {
  case JSValueInternalIndex::UNDEFINED:
    return "undefined";
  case JSValueInternalIndex::BOOL:
    return std::get<JSValueInternalIndex::BOOL>(*this->internal).internal
               ? std::string{"true"}
               : std::string{"false"};
  case JSValueInternalIndex::NUMBER:
    return std::to_string(
        std::get<JSValueInternalIndex::NUMBER>(*this->internal).internal);
  case JSValueInternalIndex::STRING:
    return std::get<JSValueInternalIndex::STRING>(*this->internal).internal;
  case JSValueInternalIndex::ARRAY:
    return "[Array]"; // FIXME
  case JSValueInternalIndex::OBJECT:
    return "[Object object]"; // FIXME?
  case JSValueInternalIndex::EXCEPTION:
    return "[EXCEPTION]"; // FIXME?
  }
  return "?";
}

bool JSValue::coerce_to_bool() const {
  switch (this->type()) {
  case JSValueInternalIndex::UNDEFINED:
    return false;
  case JSValueInternalIndex::BOOL:
    return std::get<JSBool>(*this->internal).internal;
  case JSValueInternalIndex::NUMBER:
    return std::get<JSNumber>(*this->internal).internal > 0;
  case JSValueInternalIndex::STRING:
    return std::get<JSString>(*this->internal).internal.length() > 0;
  case JSValueInternalIndex::ARRAY:
    return false; // FIXME
  case JSValueInternalIndex::OBJECT:
    return true; // FIXME
  case JSValueInternalIndex::EXCEPTION:
    return true; // FIXME
  }
  return "?";
}
