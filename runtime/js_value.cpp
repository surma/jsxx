#include "js_value.hpp"
#include <cmath>

JSValue::JSValue()
    : internal{new Box{std::in_place_index<JSValueType::UNDEFINED>,
                       JSUndefined{}}} {};

JSValue::JSValue(const JSValue &v)
    : internal{new Box{*v.internal}}, parent_value{v.parent_value} {};

JSValue::JSValue(const JSValue *v)
    : internal{new Box{*v->internal}}, parent_value{v->parent_value} {};

JSValue::JSValue(bool v)
    : internal{new Box{std::in_place_index<JSValueType::BOOL>, JSBool{v}}},
      parent_value{} {};

JSValue::JSValue(JSBool v)
    : internal{new Box{std::in_place_index<JSValueType::BOOL>, v}},
      parent_value{} {};

JSValue::JSValue(double v)
    : internal{new Box{std::in_place_index<JSValueType::NUMBER>, JSNumber{v}}},
      parent_value{} {};

JSValue::JSValue(JSNumber v)
    : internal{new Box{std::in_place_index<JSValueType::NUMBER>, v}},
      parent_value{} {};

JSValue::JSValue(const char *v)
    : internal{new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}},
      parent_value{} {};

JSValue::JSValue(std::string v)
    : internal{new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}},
      parent_value{} {};

JSValue::JSValue(JSString v)
    : internal{new Box{std::in_place_index<JSValueType::STRING>, v}},
      parent_value{} {};

JSValue::JSValue(JSFunction v)
    : internal{new Box{std::in_place_index<JSValueType::FUNCTION>, v}},
      parent_value{} {};

JSValue::JSValue(JSObject v)
    : internal{new Box{std::in_place_index<JSValueType::OBJECT>,
                       shared_ptr<JSObject>{new JSObject{v}}}},
      parent_value{} {};

JSValue::JSValue(JSArray v)
    : internal{new Box{std::in_place_index<JSValueType::ARRAY>,
                       shared_ptr<JSArray>{new JSArray{v}}}},
      parent_value{} {};

JSValue::JSValue(JSValueBinding v)
    : internal{new Box{*v.get().internal}}, parent_value{} {};

JSValue JSValue::undefined() { return JSValue{}; }
JSValue
JSValue::new_object(std::vector<std::pair<JSValue, JSValueBinding>> pairs) {
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

JSValue JSValue::operator!() { return JSValue{!this->coerce_to_bool()}; }

JSValue JSValue::operator==(const JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal ==
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->internal).internal ==
                   other.coerce_to_string()};
  }
  if (this->type() == JSValueType::BOOL) {
    return JSValue{std::get<JSValueType::BOOL>(*this->internal).internal ==
                   other.coerce_to_bool()};
  }
  return JSValue{"Equality not implemented for this type yet"};
}

JSValue JSValue::operator<(const JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal <
                   other.coerce_to_double()};
  }
  return JSValue{false};
}

JSValue JSValue::operator&&(const JSValue other) {
  if (!this->coerce_to_bool())
    return *this;
  return other;
}

JSValue JSValue::operator||(const JSValue other) {
  if (!this->coerce_to_bool())
    return other;
  return *this;
}

JSValue JSValue::operator<=(const JSValue other) {
  return *this == other || *this < other;
}

JSValue JSValue::operator>(const JSValue other) { return !(*this <= other); }

JSValue JSValue::operator!=(const JSValue other) { return !(*this == other); }

JSValue JSValue::operator>=(const JSValue other) { return !(*this < other); }

JSValue JSValue::operator+(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal +
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->internal).internal +
                   other.coerce_to_string()};
  }
  return JSValue{"Addition not implemented for this type yet"};
}

JSValue JSValue::operator*(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->internal).internal *
                   other.coerce_to_double()};
  }
  return JSValue{"Multiplication not implemented for this type yet"};
}

JSValue JSValue::operator%(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{static_cast<double>(
        static_cast<uint32_t>(
            std::get<JSValueType::NUMBER>(*this->internal).internal) %
        static_cast<uint32_t>(other.coerce_to_double()))};
  }
  return JSValue{"Modulo not implemented for this type yet"};
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
  auto this_arg_ptr = this->parent_value.value_or(
      shared_ptr<JSValue>{new JSValue{JSValue::undefined()}});
  return this->apply(*this_arg_ptr, args);
}

JSValue JSValue::get_property(const JSValue key) {
  return this->get_property_slot(key).get();
}

JSValueBinding JSValue::get_property_slot(const JSValue key) {
  JSValueBinding v;
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    v = JSValueBinding::with_value(JSValue::undefined());
    break;
  case JSValueType::BOOL:
    v = std::get<JSValueType::BOOL>(*this->internal).get_property_slot(key);
    break;
  case JSValueType::NUMBER:
    v = std::get<JSValueType::NUMBER>(*this->internal).get_property_slot(key);
    break;
  case JSValueType::STRING:
    v = std::get<JSValueType::STRING>(*this->internal).get_property_slot(key);
    break;
  case JSValueType::ARRAY:
    v = std::get<JSValueType::ARRAY>(*this->internal)->get_property_slot(key);
    break;
  case JSValueType::OBJECT:
    v = std::get<JSValueType::OBJECT>(*this->internal)->get_property_slot(key);
    break;
  case JSValueType::FUNCTION:
    v = std::get<JSValueType::FUNCTION>(*this->internal).get_property_slot(key);
    break;
  case JSValueType::EXCEPTION:
    v = std::get<JSValueType::EXCEPTION>(*this->internal)
            ->get_property_slot(key);
    break;
  };
  v.set_parent(*this);
  return v;
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
