#include <cmath>
#include "js_value.hpp"

JSValue JSValue::operator==(const JSValue &other) {
  if (this->type() == JSValueInternalIndex::DOUBLE) {
    return new JSValue{std::get<JSNumber>(*this->internal).internal ==
                       other.coerce_to_double()};
  }
  if (this->type() == JSValueInternalIndex::STRING) {
    return JSValue{std::get<JSString>(*this->internal).internal ==
                   other.coerce_to_string()};
  }
  return JSValue{"Equality not implemented for this type yet"};
}

JSValue JSValue::operator+(JSValue &other) {
  if (this->type() == JSValueInternalIndex::DOUBLE) {
    return JSValue{std::get<JSNumber>(*this->internal).internal +
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueInternalIndex::STRING) {
    return JSValue{std::get<JSString>(*this->internal).internal +
                   other.coerce_to_string()};
  }
  return new JSValue{"Addition not implemented for this type yet"};
}

// JSValue& operator[](JSValue& index) {
// if(this->type() == JSValueInternalIndex::ARRAY && index.type() ==
// JSValueInternalIndex::DOUBLE) { 	return
// }
// if(this->type() == JSValueInternalIndex::OBJECT) {
// 		JSObject obj = *(std::get<JSObject*>(this->internal));
// 		return obj[index];
// 	}
// 	return *this;
// }

JSValueInternalIndex JSValue::type() const {
  return static_cast<JSValueInternalIndex>(this->internal->index());
}

bool JSValue::is_undefined() const {
  return this->type() == JSValueInternalIndex::UNDEFINED;
}

double JSValue::coerce_to_double() const {
  switch (this->type()) {
  case 1:
    return std::get<JSBool>(*this->internal).internal ? 1 : 0;
  case 2:
    return std::get<JSNumber>(*this->internal).internal;
  case 3:
    return std::stod(std::get<JSString>(*this->internal).internal);
  default:
    return NAN;
  }
}

std::string JSValue::coerce_to_string() const {
  switch (this->type()) {
  case 0:
    return "undefined";
  case 1:
    return std::get<JSBool>(*this->internal).internal ? std::string{"true"}
                                                      : std::string{"false"};
  case 2:
    return std::to_string(std::get<JSNumber>(*this->internal).internal);
  case 3:
    return std::get<JSString>(*this->internal).internal;
  case 4:
    return "[Array]"; // FIXME
  case 5:
    return "[Object object]"; // FIXME?
  }
  return "?";
}

bool JSValue::coerce_to_bool() const {
  switch (this->type()) {
  case 0:
    return false;
  case 1:
    return std::get<JSBool>(*this->internal).internal;
  case 2:
    return std::get<JSNumber>(*this->internal).internal > 0;
  case 3:
    return std::get<JSString>(*this->internal).internal.length() > 0;
  case 4:
    return false; // FIXME
  case 5:
    return true; // FIXME
  }
  return "?";
}

  JSValue::JSValue() : internal{new Box{JSUndefined{}}} {};
  JSValue::JSValue(bool v) : internal{new Box{JSBool{v}}} {};
  // JSValue::JSValue(JSBool v) : internal{v} {};
  JSValue::JSValue(double v) : internal{new Box{JSNumber{v}}} {};
  // JSValue::JSValue(JSNumber v) : internal{v} {};
  JSValue::JSValue(const char *v) : internal{new Box{JSString{v}}} {};
  JSValue::JSValue(std::string v) : internal{new Box{JSString{v}}} {};
  // JSValue::JSValue(JSString v) : internal{v} {};

JSValue JSValue::undefined() {
  return JSValue{};
}
