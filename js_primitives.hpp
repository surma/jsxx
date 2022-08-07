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
};

class JSObject : public JSBase {
public:
  JSObject();

  JSValueBinding operator[](const JSValue idx);
  std::vector<std::pair<JSValue, JSValueBinding>> internal;
};

class JSFunction : public JSBase {
  using JSExternFunc = std::function<JSValue(const std::vector<JSValue> &)>;

public:
  JSFunction(JSExternFunc f);
  JSExternFunc internal;
};
