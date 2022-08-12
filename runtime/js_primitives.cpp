#include "js_primitives.hpp"

JSBase::JSBase() {}

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValue JSBase::get_property(JSValue key) {
  return this->get_property_slot(key).get();
}

JSValueBinding JSBase::get_property_slot(JSValue key) {
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
    {JSValue{"filter"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::filter_impl))},
    {JSValue{"reduce"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::reduce_impl))},
    {JSValue{"join"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::join_impl))},
    {JSValue{"length"},
     JSValueBinding::with_value(JSValue::new_function(&JSArray::length_impl))},
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

JSValue JSArray::length_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
  return JSValue{static_cast<double>(arr->internal.size())};
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

JSValue JSArray::filter_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  JSValue f = args[0];
  if (f.type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
  JSArray result_arr{};
  for (int i = 0; i < arr->internal.size(); i++) {
    if (f({arr->internal[i], JSValue{static_cast<double>(i)}})
            .coerce_to_bool()) {
      result_arr.internal.push_back(arr->internal[i]);
    }
  }
  return JSValue{result_arr};
}

JSValue JSArray::reduce_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);

  if (args[0].type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  JSValue f = args[0];

  int i;
  JSValue acc;
  if (args.size() >= 2 && !args[1].is_undefined()) {
    i = 0;
    acc = args[1];
  } else if (arr->internal.size() >= 1) {
    i = 1;
    acc = arr->internal[0];
  }

  for (; i < arr->internal.size(); i++) {
    acc = f({acc, arr->internal[i], JSValue{static_cast<double>(i)}});
  }
  return acc;
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

JSValueBinding JSArray::get_property_slot(const JSValue key) {
  if (key.type() == JSValueType::NUMBER) {
    auto idx = static_cast<size_t>(key.coerce_to_double());
    if (idx >= this->internal.size())
      return JSValueBinding::with_value(JSValue::undefined());
    return this->internal[idx];
  }
  return JSBase::get_property_slot(key);
}

JSObject::JSObject() : JSBase(), internal{} {};
JSObject::JSObject(std::vector<std::pair<JSValue, JSValue>> data) : JSObject() {
  for (auto v : data) {
    this->internal.push_back({v.first, JSValueBinding::with_value(v.second)});
  }
};

JSValueBinding JSObject::get_property_slot(const JSValue key) {
  auto obj = std::find_if(this->internal.begin(), this->internal.end(),
                          [=](std::pair<JSValue, JSValueBinding> &item) {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == this->internal.end()) {
    return JSBase::get_property_slot(key);
  }
  return (*obj).second;
}

JSFunction::JSFunction(ExternFunc f) : JSBase(), internal{f} {};

JSValue JSFunction::call(JSValue thisArg, std::vector<JSValue> &args) {
  return this->internal(thisArg, args);
}
