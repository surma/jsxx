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
     JSValueBinding::with_value(JSValue{(ExternFunc)[](
         JSValue thisArg, const std::vector<JSValue> &args){
         if (thisArg.type() != JSValueType::ARRAY) return JSValue::undefined();
         auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
         for (auto v
              : args) {
           arr->internal.push_back(JSValueBinding::with_value(v));
         } return JSValue::undefined();}})
}
, {JSValue{"map"},
     JSValueBinding::with_value(JSValue{(ExternFunc)[](
         JSValue thisArg, const std::vector<JSValue> &args){
         if (thisArg.type() != JSValueType::ARRAY) return JSValue::undefined();
         auto f = args[0];
         if (f.type() != JSValueType::FUNCTION) return JSValue::undefined();
         auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
         JSArray result_arr{};
         for (auto v
              : args) {
  result_arr.internal.push_back(JSValueBinding::with_value(f(v)));
         }
         return JSValue{result_arr};
}
,
})
}
, {JSValue{"join"},
     JSValueBinding::with_value(JSValue{(ExternFunc)[](
         JSValue thisArg, const std::vector<JSValue> &args){
         if (thisArg.type() != JSValueType::ARRAY) return JSValue::undefined();

      	 std::string result = "";
         if (args[0].type() != JSValueType::STRING) return JSValue::undefined();
         auto del = args[0].coerce_to_string();
         auto arr = std::get<JSValueType::ARRAY>(*thisArg.internal);
         for (auto v
              : arr->internal) {
    result += v.get().coerce_to_string() + del;
         }
         return JSValue{result};
}
})
}
}
;

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

JSValue JSFunction::call(JSValue thisArg, const std::vector<JSValue> &args) {
  return this->internal(thisArg, args);
}
