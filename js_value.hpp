#pragma once

#include <memory>
#include <variant>

#include "js_primitives.hpp"
#include "js_value_binding.hpp"

using std::shared_ptr;
using std::unique_ptr;

class JSUndefined;
class JSBool;
class JSNumber;
class JSString;
class JSArray;
class JSObject;
class JSFunction;

// Idk what C++ wants from me... This type alias is defined in
// `js_primitives.hpp` but the cyclic includes seem to make it impossible to see
// that here.
using ExternFunc =
    std::function<JSValue(JSValue, const std::vector<JSValue> &)>;

enum JSValueType : char {
  UNDEFINED,
  BOOL,
  NUMBER,
  STRING,
  ARRAY,
  OBJECT,
  FUNCTION,
  EXCEPTION
};

class JSValue {
  using Box = std::variant<JSUndefined, JSBool, JSNumber, JSString,
                           std::shared_ptr<JSArray>, std::shared_ptr<JSObject>,
                           JSFunction, std::shared_ptr<JSObject>>;

public:
  JSValue();
  JSValue(bool v);
  JSValue(JSBool v);
  JSValue(double v);
  JSValue(JSNumber v);
  JSValue(const char *v);
  JSValue(std::string v);
  JSValue(JSString v);
  JSValue(const JSValue &v);
  JSValue(ExternFunc v);
  JSValue(JSFunction v);
  JSValue(JSObject v);
  JSValue(JSArray v);

  JSValue operator=(JSValue other);
  // JSValue& operator=(JSValue& other);
  JSValue operator==(const JSValue other);
  JSValue operator+(JSValue other);
  JSValueBinding operator[](const JSValue index);
  JSValueBinding operator[](const char *index);
  JSValueBinding operator[](const size_t index);
  JSValue operator()(JSValue args...);

  static JSValue new_object(std::vector<std::pair<JSValue, JSValue>>);
  static JSValue new_array(std::vector<JSValue>);
  static JSValue undefined();
  // static JSValue throww();

  JSValueBinding get_property(const JSValue key);
  JSValue apply(JSValue thisArg, std::vector<JSValue> args);

  JSValueType type() const;
  double coerce_to_double() const;
  std::string coerce_to_string() const;
  bool coerce_to_bool() const;

  bool is_undefined() const;

  unique_ptr<Box> internal;
};
