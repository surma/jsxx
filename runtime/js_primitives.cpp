#include "js_primitives.hpp"

JSBase::JSBase() {}

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValueBinding JSBase::get_property(JSValue key) {
  auto obj =
      std::find_if(this->properties.begin(), this->properties.end(),
                   [&](std::pair<JSValue, JSValueBinding> &item) -> bool {
                     return (item.first == key).coerce_to_bool();
                   });
  if (obj == this->properties.end()) {
    return JSValueBinding::with_value(JSValue::undefined());
  }
  return (*obj).second;
}

JSBool::JSBool(bool v) : JSBase(), internal{v} {};

JSNumber::JSNumber(double v) : JSBase(), internal{v} {};

JSString::JSString(const char *v) : JSBase(), internal{std::string(v)} {};

JSString::JSString(std::string v) : JSBase(), internal{v} {};

std::vector<std::pair<JSValue, JSValueBinding>> JSArray_prototype{
    {JSValue{"push"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::push_impl))},
    {JSValue{"map"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::map_impl))},
    {JSValue{"join"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::join_impl))},
};

JSArray::JSArray() : JSBase() {
  for (const auto &entry : JSArray_prototype) {
    this->properties.push_back(entry);
  }
};

JSArray::JSArray(std::vector<JSValue> data) : JSArray() {
  for (auto v : data) {
    this->internal.push_back(JSValueBinding::with_value(v));
  }
}

JSValue JSArray::push_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
  for (auto v : args) {
    arr->internal.push_back(JSValueBinding::with_value(v));
  }
  return JSValue::undefined();
}

JSValue JSArray::map_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  JSValue f = args[0];
  if (f.type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
  JSArray result_arr{};
  for (int i = 0; i < arr->internal.size(); i++) {
    result_arr.internal.push_back(JSValueBinding::with_value(
        f({arr->internal[i].get(), JSValue{static_cast<double>(i)}})));
  }
  return JSValue{result_arr};
}

JSValue JSArray::join_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();

  std::string delimiter = "";
  std::string result = "";
  if (args.size() > 0 && args[0].type() == JSValueType::STRING) {
    delimiter = args[0].coerce_to_string();
  }
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
  for (auto v : arr->internal) {
    result += v.get().coerce_to_string() + delimiter;
  }
  result = result.substr(0, result.size() - delimiter.size());
  return JSValue{result};
}

JSValueBinding JSArray::operator[](const JSValue idx) {
  if (idx.type() != JSValueType::NUMBER) {
    return this->get_property(idx);
  }
  return this->internal[static_cast<size_t>(idx.coerce_to_double())];
}

JSObject::JSObject() : JSBase(), internal{} {};
JSObject::JSObject(std::vector<std::pair<JSValue, JSValue>> data) : JSObject() {
  for (auto v : data) {
    this->internal.push_back({v.first, JSValueBinding::with_value(v.second)});
  }
};

JSValueBinding JSObject::operator[](const JSValue idx) {
  auto obj = std::find_if(this->internal.begin(), this->internal.end(),
                          [=](std::pair<JSValue, JSValueBinding> &item) {
                            return (item.first == idx).coerce_to_bool();
                          });
  if (obj == this->internal.end()) {
    return this->get_property(idx);
  }
  return (*obj).second;
}

JSFunction::JSFunction(ExternFunc f) : JSBase(), internal{f} {};

JSValue JSFunction::call(JSValue thisArg, std::vector<JSValue> &args) {
  return this->internal(thisArg, args);
}