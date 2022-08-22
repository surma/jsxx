#pragma once

#include <memory>
#include <optional>
#include <variant>

#include "js_primitives.hpp"

using std::optional;
using std::shared_ptr;
using std::unique_ptr;

class JSUndefined;
class JSBool;
class JSNumber;
class JSString;
class JSArray;
class JSObject;
class JSFunction;
class JSIterator;
class JSGeneratorAdapter;
class JSValue;

// Idk what C++ wants from me... This type alias is defined in
// `js_primitives.hpp` but the cyclic includes seem to make it impossible to see
// that here.
using ExternFunc = std::function<JSValue(JSValue, std::vector<JSValue> &)>;
using CoroutineFunc =
    std::function<JSGeneratorAdapter(JSValue, std::vector<JSValue> &)>;

enum JSValueType : char {
  UNDEFINED,
  BOOL,
  NUMBER,
  STRING,
  ARRAY,
  OBJECT,
  FUNCTION
};

class JSValue {
  using Box = std::variant<JSUndefined, JSBool, JSNumber, JSString,
                           std::shared_ptr<JSArray>, std::shared_ptr<JSObject>,
                           JSFunction, std::shared_ptr<JSObject>>;

  using Getter = std::function<JSValue(JSValue)>;
  using Setter = std::function<JSValue(JSValue, JSValue)>;

public:
  JSValue();
  JSValue(bool v);
  JSValue(JSBool v);
  JSValue(double v);
  JSValue(JSNumber v);
  JSValue(const char *v);
  JSValue(std::string v);
  JSValue(JSString v);
  JSValue(JSFunction v);
  JSValue(JSObject v);
  JSValue(JSArray v);
  JSValue(Box v);

  JSValue operator=(const Box &other);
  JSValue &operator++();   // Prefix
  JSValue operator++(int); // Postfix
  JSValue &operator--();   // Prefix
  JSValue operator--(int); // Postfix
  JSValue operator==(const JSValue other) const;
  JSValue operator!();
  JSValue operator<(const JSValue other);
  JSValue operator<=(const JSValue other);
  JSValue operator>(const JSValue other);
  JSValue operator!=(const JSValue other);
  JSValue operator>=(const JSValue other);
  JSValue operator&&(const JSValue other);
  JSValue operator||(const JSValue other);
  JSValue operator+(JSValue other);
  JSValue operator*(JSValue other);
  JSValue operator%(JSValue other);
  JSValue operator[](const JSValue index);
  JSValue operator[](const char *index);
  JSValue operator[](const size_t index);
  JSValue operator()(std::vector<JSValue> args);

  JSIterator begin();
  JSIterator end();

  static JSValue new_object(std::vector<std::pair<JSValue, JSValue>>);
  static JSValue new_array(std::vector<JSValue>);
  static JSValue new_function(ExternFunc f);
  static JSValue new_generator_function(CoroutineFunc gen_f);
  static JSValue undefined();
  static JSValue iterator_from_next_func(JSValue next_func);
  static JSValue with_getter_setter(JSValue getter, JSValue setter);

  JSValue get_property(const JSValue key, JSValue parent);
  JSValue apply(JSValue thisArg, std::vector<JSValue> args);

  JSValueType type() const;
  double coerce_to_double() const;
  std::string coerce_to_string() const;
  bool coerce_to_bool() const;

  bool is_undefined() const;
  double &get_number();
  void set_parent(JSValue parent_value);
  JSValue get_parent();
  const Box &boxed_value() const;

  shared_ptr<Box> value;
  optional<shared_ptr<JSValue>> parent_value;

  std::optional<Getter> getter = std::nullopt;
  std::optional<Setter> setter = std::nullopt;
};
