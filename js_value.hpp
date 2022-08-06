#pragma once

#include <memory>
#include <variant>

#include "js_primitives.hpp"

using std::shared_ptr;

class JSUndefined;
class JSBool;
class JSNumber;
class JSString;
class JSArray;
class JSObject;

enum JSValueInternalIndex { UNDEFINED, BOOL, DOUBLE, STRING, ARRAY, OBJECT };

class JSValue {
  using Box =
      std::variant<JSUndefined, JSBool, JSNumber, JSString, JSArray, JSObject>;

public:
  JSValue();
  JSValue(bool v);
  // JSValue(JSBool v);
  JSValue(double v);
  // JSValue(JSNumber v);
  JSValue(const char *v);
  JSValue(std::string v);
  // JSValue(JSString v);

  JSValue operator==(const JSValue &other);
  JSValue operator+(JSValue &other);

  static JSValue undefined();

  // JSValue& operator[](JSValue& index);
  JSValueInternalIndex type() const;
  double coerce_to_double() const;
  std::string coerce_to_string() const;
  bool coerce_to_bool() const;

  bool is_undefined() const;

  std::shared_ptr<Box> internal;
};
