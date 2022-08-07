#pragma once

#include <memory>
#include <variant>

#include "js_primitives.hpp"

using std::shared_ptr;
using std::unique_ptr;

class JSUndefined;
class JSBool;
class JSNumber;
class JSString;
class JSArray;
class JSObject;

enum JSValueInternalIndex {
  UNDEFINED,
  BOOL,
  NUMBER,
  STRING,
  ARRAY,
  OBJECT,
  EXCEPTION
};

class JSValue {
  using Box = std::variant<JSUndefined, JSBool, JSNumber, JSString,
                           std::shared_ptr<JSArray>, std::shared_ptr<JSObject>,
                           std::shared_ptr<JSObject>>;

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

  JSValue operator=(JSValue other);
  // JSValue& operator=(JSValue& other);
  JSValue operator==(const JSValue other);
  JSValue operator+(JSValue other);
  JSValue &operator[](const JSValue index);
  JSValue &operator[](const char *index);

  static JSValue new_object(std::vector<std::pair<JSValue, JSValue>>);
  static JSValue undefined();
  // static JSValue throww();

  JSValue &get_property(const JSValue key);

  JSValueInternalIndex type() const;
  double coerce_to_double() const;
  std::string coerce_to_string() const;
  bool coerce_to_bool() const;

  bool is_undefined() const;

  unique_ptr<Box> internal;
};
