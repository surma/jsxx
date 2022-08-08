#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "js_value.hpp"
#include "js_value_binding.hpp"

using std::shared_ptr;

class JSValue;
class JSValueBinding;

class JSUndefined {
  JSValue operator==(JSValue &other);
};

class JSBase {
public:
  JSBase();

  JSValueBinding get_property(JSValue key);

  std::vector<std::pair<JSValue, JSValueBinding>> properties;
};

class JSBool : public JSBase {
public:
  JSBool(bool v);

  bool internal;
};

class JSNumber : public JSBase {
public:
  JSNumber(double v);
  double internal;
};

class JSString : public JSBase {
public:
  JSString(const char *v);
  JSString(std::string v);
  std::string internal;
};

class JSArray : public JSBase {
public:
  JSArray();
  JSArray(std::vector<JSValue> data);

  JSValueBinding operator[](const JSValue idx);
  std::vector<JSValueBinding> internal;

  static JSValue push_impl(JSValue thisArg, std::vector<JSValue> &args);
};

class JSObject : public JSBase {
public:
  JSObject();
  JSObject(std::vector<std::pair<JSValue, JSValue>> data);

  JSValueBinding operator[](const JSValue idx);
  std::vector<std::pair<JSValue, JSValueBinding>> internal;
};

using ExternFunc = std::function<JSValue(JSValue, std::vector<JSValue> &)>;
using ExternFuncPtr = JSValue (*)(JSValue, std::vector<JSValue> &);
class JSFunction : public JSBase {

public:
  JSFunction(ExternFunc f);
  ExternFunc internal;

  JSValue call(JSValue thisArg, std::vector<JSValue> &);
};
