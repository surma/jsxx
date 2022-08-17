#include "js_value.hpp"
#include <cmath>

JSValue::JSValue()
    : value{new Box{std::in_place_index<JSValueType::UNDEFINED>,
                    JSUndefined{}}} {};

JSValue::JSValue(bool v)
    : value{new Box{std::in_place_index<JSValueType::BOOL>, JSBool{v}}},
      parent_value{} {};

JSValue::JSValue(JSBool v)
    : value{new Box{std::in_place_index<JSValueType::BOOL>, v}},
      parent_value{} {};

JSValue::JSValue(double v)
    : value{new Box{std::in_place_index<JSValueType::NUMBER>, JSNumber{v}}},
      parent_value{} {};

JSValue::JSValue(JSNumber v)
    : value{new Box{std::in_place_index<JSValueType::NUMBER>, v}},
      parent_value{} {};

JSValue::JSValue(const char *v)
    : value{new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}},
      parent_value{} {};

JSValue::JSValue(std::string v)
    : value{new Box{std::in_place_index<JSValueType::STRING>, JSString{v}}},
      parent_value{} {};

JSValue::JSValue(JSString v)
    : value{new Box{std::in_place_index<JSValueType::STRING>, v}},
      parent_value{} {};

JSValue::JSValue(JSFunction v)
    : value{new Box{std::in_place_index<JSValueType::FUNCTION>, v}},
      parent_value{} {};

JSValue::JSValue(JSObject v)
    : value{new Box{std::in_place_index<JSValueType::OBJECT>,
                    shared_ptr<JSObject>{new JSObject{v}}}},
      parent_value{} {};

JSValue::JSValue(JSArray v)
    : value{new Box{std::in_place_index<JSValueType::ARRAY>,
                    shared_ptr<JSArray>{new JSArray{v}}}},
      parent_value{} {};

JSValue::JSValue(Box v) : value{new Box{v}}, parent_value{} {};

JSValue JSValue::undefined() { return JSValue{}; }

JSValue JSValue::new_object(std::vector<std::pair<JSValue, JSValue>> pairs) {
  return JSValue{JSObject{pairs}};
}

JSValue JSValue::new_array(std::vector<JSValue> values) {
  return JSValue{JSArray{values}};
}

JSValue JSValue::new_function(ExternFunc f) { return JSValue{JSFunction{f}}; }

JSValue JSValue::new_generator_function(CoroutineFunc gen_f) {
  return JSValue::new_function([=](JSValue thisArg,
                                   std::vector<JSValue> &args) mutable
                               -> JSValue {
    std::shared_ptr<std::optional<
        std::experimental::coroutine_handle<JSGeneratorAdapter::promise_type>>>
        corot =
            std::make_shared<std::optional<std::experimental::coroutine_handle<
                JSGeneratorAdapter::promise_type>>>(std::nullopt);
    return JSValue::iterator_from_next_func(JSValue::new_function(
        [corot, gen_f](JSValue thisArg,
                       std::vector<JSValue> &args) mutable -> JSValue {
          if (!corot->has_value()) {
            *corot = std::optional{gen_f(thisArg, args).h};
          } else {
            corot->value()();
          }
          auto v = corot->value().promise().value;
          return JSValue::new_object(
              {{JSValue{"value"},
                *v.value_or(std::make_shared<JSValue>(JSValue::undefined()))},
               {JSValue{"done"}, JSValue{!v.has_value()}}});
        }));
  });
}

JSValue &JSValue::operator++() {
  if (this->type() != JSValueType::NUMBER) {
    *this = JSValue::undefined();
    return *this;
  }
  this->get_number() = this->get_number() + 1.0;
  return *this;
}

JSValue JSValue::operator++(int) {
  if (this->type() != JSValueType::NUMBER) {
    *this = JSValue::undefined();
    return JSValue::undefined();
  }
  JSValue prev{this->get_number()};
  this->get_number() = this->get_number() + 1.0;
  return prev;
}

JSValue &JSValue::operator--() {
  if (this->type() != JSValueType::NUMBER) {
    *this = JSValue::undefined();
    return *this;
  }
  this->get_number() = this->get_number() - 1.0;
  return *this;
}

JSValue JSValue::operator--(int) {
  if (this->type() != JSValueType::NUMBER) {
    *this = JSValue::undefined();
    return JSValue::undefined();
  }
  JSValue prev{this->get_number()};
  this->get_number() = this->get_number() - 1.0;
  return prev;
}

JSValue JSValue::operator=(const Box &other) {
  *this->value = other;
  return other;
}

JSValue JSValue::operator!() { return JSValue{!this->coerce_to_bool()}; }

JSValue JSValue::operator==(const JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->value).internal ==
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->value).internal ==
                   other.coerce_to_string()};
  }
  if (this->type() == JSValueType::BOOL) {
    return JSValue{std::get<JSValueType::BOOL>(*this->value).internal ==
                   other.coerce_to_bool()};
  }
  if (this->type() == JSValueType::ARRAY) {
    if (other.type() != JSValueType::ARRAY)
      return JSValue{false};
    return JSValue{std::get<JSValueType::ARRAY>(*this->value).get() ==
                   std::get<JSValueType::ARRAY>(*other.value).get()};
  }
  if (this->type() == JSValueType::OBJECT) {
    if (other.type() != JSValueType::OBJECT)
      return JSValue{false};
    return JSValue{std::get<JSValueType::OBJECT>(*this->value).get() ==
                   std::get<JSValueType::OBJECT>(*other.value).get()};
  }
  return JSValue{"Equality not implemented for this type yet"};
}

JSValue JSValue::operator<(const JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->value).internal <
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
    return JSValue{std::get<JSValueType::NUMBER>(*this->value).internal +
                   other.coerce_to_double()};
  }
  if (this->type() == JSValueType::STRING) {
    return JSValue{std::get<JSValueType::STRING>(*this->value).internal +
                   other.coerce_to_string()};
  }
  return JSValue{"Addition not implemented for this type yet"};
}

JSValue JSValue::operator*(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{std::get<JSValueType::NUMBER>(*this->value).internal *
                   other.coerce_to_double()};
  }
  return JSValue{"Multiplication not implemented for this type yet"};
}

JSValue JSValue::operator%(JSValue other) {
  if (this->type() == JSValueType::NUMBER) {
    return JSValue{static_cast<double>(
        static_cast<uint32_t>(
            std::get<JSValueType::NUMBER>(*this->value).internal) %
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

JSIterator JSValue::begin() { return JSIterator{(*this)[iterator_symbol]({})}; }

JSIterator JSValue::end() { return JSIterator::end_marker(); }

JSValue JSValue::get_property(const JSValue key) {
  JSValue v;
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    v = JSValue::undefined();
    break;
  case JSValueType::BOOL:
    v = std::get<JSValueType::BOOL>(*this->value).get_property(key);
    break;
  case JSValueType::NUMBER:
    v = std::get<JSValueType::NUMBER>(*this->value).get_property(key);
    break;
  case JSValueType::STRING:
    v = std::get<JSValueType::STRING>(*this->value).get_property(key);
    break;
  case JSValueType::ARRAY:
    v = std::get<JSValueType::ARRAY>(*this->value)->get_property(key);
    break;
  case JSValueType::OBJECT:
    v = std::get<JSValueType::OBJECT>(*this->value)->get_property(key);
    break;
  case JSValueType::FUNCTION:
    v = std::get<JSValueType::FUNCTION>(*this->value).get_property(key);
    break;
  case JSValueType::EXCEPTION:
    v = std::get<JSValueType::EXCEPTION>(*this->value)->get_property(key);
    break;
  };
  // FIXME
  // v.set_parent(*this);
  return v;
}

JSValueType JSValue::type() const {
  return static_cast<JSValueType>(this->value->index());
}

bool JSValue::is_undefined() const {
  return this->type() == JSValueType::UNDEFINED;
}

double JSValue::coerce_to_double() const {
  switch (this->type()) {
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->value).internal ? 1 : 0;
  case JSValueType::NUMBER:
    return std::get<JSValueType::NUMBER>(*this->value).internal;
  case JSValueType::STRING:
    return std::stod(std::get<JSValueType::STRING>(*this->value).internal);
  default:
    return NAN;
  }
}

double &JSValue::get_number() {
  return std::get<JSValueType::NUMBER>(*this->value).internal;
}

std::string JSValue::coerce_to_string() const {
  switch (this->type()) {
  case JSValueType::UNDEFINED:
    return "undefined";
  case JSValueType::BOOL:
    return std::get<JSValueType::BOOL>(*this->value).internal
               ? std::string{"true"}
               : std::string{"false"};
  case JSValueType::NUMBER:
    return std::to_string(std::get<JSValueType::NUMBER>(*this->value).internal);
  case JSValueType::STRING:
    return std::get<JSValueType::STRING>(*this->value).internal;
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
    return std::get<JSBool>(*this->value).internal;
  case JSValueType::NUMBER:
    return std::get<JSNumber>(*this->value).internal > 0;
  case JSValueType::STRING:
    return std::get<JSString>(*this->value).internal.length() > 0;
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
  JSFunction f = std::get<JSValueType::FUNCTION>(*this->value);
  return f.call(thisArg, args);
}

JSIterator::JSIterator() : JSIterator{JSValue::undefined()} {}

JSIterator::JSIterator(JSValue val) {
  this->it = shared_ptr<JSValue>{new JSValue{val}};
}

JSIterator JSIterator::end_marker() {
  JSIterator it{};
  it.last_value =
      std::optional{shared_ptr<JSValue>{new JSValue{JSValue::new_object({
          {JSValue{"value"}, JSValue::undefined()},
          {JSValue{"done"}, JSValue{true}},
      })}}};
  return it;
}

JSValue JSIterator::operator*() { return this->value()["value"]; }

JSIterator JSIterator::operator++() {
  if (!this->it->is_undefined()) {
    this->last_value = std::optional{
        shared_ptr<JSValue>{new JSValue{(*this->it)["next"]({})}}};
  }
  return *this;
}

bool JSIterator::operator!=(const JSIterator &other) {
  if (other.last_value.has_value() != this->last_value.has_value()) {
    return true;
  }
  JSValue left = *this->last_value.value();
  JSValue right = *other.last_value.value();
  bool left_done = left["done"].coerce_to_bool();
  bool right_done = right["done"].coerce_to_bool();
  if (left_done && right_done) {
    return false;
  }
  return left_done != right_done ||
         (left["value"] != right["value"]).coerce_to_bool();
}

JSValue JSIterator::value() {
  if (!this->last_value.has_value()) {
    ++(*this);
  }
  return *this->last_value.value();
}

JSValue JSValue::iterator_from_next_func(JSValue next_func) {
  return JSValue::new_object(
      {{iterator_symbol,
        JSValue::new_function(
            [=](JSValue thisArg, std::vector<JSValue> &args) mutable {
              return JSValue::new_object({{JSValue{"next"}, next_func}});
            })}});
};
