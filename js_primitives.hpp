#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "js_value.hpp"

using std::shared_ptr;

class JSValue;

class JSUndefined {
  JSValue operator==(JSValue &other);
};

class JSBase {
public:
  JSBase();

  JSValue operator[](JSValue &key);

  std::vector<std::pair<JSValue, JSValue>> properties;

private:
  virtual void init_properties() {}
};

class JSBool : JSBase {
public:
  JSBool(bool v);

  bool internal;
};

class JSNumber : JSBase {
public:
  JSNumber(double v);
  double internal;

private:
  void init_properties();
};

class JSString : JSBase {
public:
  JSString(const char *v);
  JSString(std::string v);
  std::string internal;
};

class JSArray : JSBase {
public:
  JSArray();
};

class JSObject : JSBase {
public:
  JSObject();

  // JSValue& operator[](JSValue& idx) {
  // 	auto obj = std::find_if(this->internal.begin(), this->internal.end(),
  // [=](std::pair<JSValue, JSValue>& item) { return item.first == idx; });
  // 	if(obj == this->internal.end()) {
  // 		// FIXME
  // 		return idx;
  // 	}
  // 	return (*obj).second;
  // }
  // Can’t be bothered to make JSValue work with std::map for now.
  std::vector<std::pair<JSValue, JSValue>> internal;
};

class JSFunction : JSBase {
  using JSExternFunc = std::function<JSValue(const std::vector<JSValue> &)>;

public:
  JSFunction(JSExternFunc f);
  JSExternFunc internal;
};
