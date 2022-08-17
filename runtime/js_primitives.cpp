#include "js_primitives.hpp"

JSBase::JSBase() {}

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValue JSBase::get_property(JSValue key) {
  auto obj = std::find_if(this->properties.begin(), this->properties.end(),
                          [&](std::pair<JSValue, JSValue> &item) -> bool {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == this->properties.end()) {
    return JSValue::undefined();
  }
  return (*obj).second;
}

JSBool::JSBool(bool v) : JSBase(), internal{v} {};

JSNumber::JSNumber(double v) : JSBase(), internal{v} {};

JSString::JSString(const char *v) : JSBase(), internal{std::string(v)} {};

JSString::JSString(std::string v) : JSBase(), internal{v} {};

std::vector<std::pair<JSValue, JSValue>> JSArray_prototype{
    {JSValue{"push"}, JSValue::new_function(&JSArray::push_impl)},
    {JSValue{"map"}, JSValue::new_function(&JSArray::map_impl)},
    {JSValue{"filter"}, JSValue::new_function(&JSArray::filter_impl)},
    {JSValue{"reduce"}, JSValue::new_function(&JSArray::reduce_impl)},
    {JSValue{"join"}, JSValue::new_function(&JSArray::join_impl)},

};

JSArray::JSArray() : JSBase(), internal{new std::vector<JSValue>{}} {
  for (const auto &entry : JSArray_prototype) {
    this->properties.push_back(entry);
  }
  std::vector<JSValue> *data = &(*this->internal);
  auto length_prop = JSValue::with_getter_setter(
      JSValue::new_function(
          [=](JSValue thisArg, std::vector<JSValue> &args) mutable -> JSValue {
            return JSValue{static_cast<double>(data->size())};
          }),
      JSValue::new_function(
          [=](JSValue thisArg, std::vector<JSValue> &args) mutable -> JSValue {
            if (args.size() < 1 || args[0].type() != JSValueType::NUMBER)
              return JSValue::undefined();
            JSValue v = args[0];
            data->resize(static_cast<size_t>(v.coerce_to_double()),
                         JSValue::undefined());
            return JSValue::undefined();
          }));
  this->properties.push_back({JSValue{"length"}, length_prop});
};

JSArray::JSArray(std::vector<JSValue> data) : JSArray() {
  for (auto v : data) {
    this->internal->push_back(v);
  }
}

JSValue JSArray::push_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
  for (auto v : args) {
    arr->internal->push_back(v);
  }
  return JSValue::undefined();
}

JSValue JSArray::map_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  JSValue f = args[0];
  if (f.type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
  JSArray result_arr{};
  for (int i = 0; i < arr->internal->size(); i++) {
    result_arr.internal->push_back(
        f({(*arr->internal)[i], JSValue{static_cast<double>(i)}}));
  }
  return JSValue{result_arr};
}

JSValue JSArray::filter_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  JSValue f = args[0];
  if (f.type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
  JSArray result_arr{};
  for (int i = 0; i < arr->internal->size(); i++) {
    if (f({(*arr->internal)[i], JSValue{static_cast<double>(i)}})
            .coerce_to_bool()) {
      result_arr.internal->push_back((*arr->internal)[i]);
    }
  }
  return JSValue{result_arr};
}

JSValue JSArray::reduce_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    return JSValue::undefined();
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);

  if (args[0].type() != JSValueType::FUNCTION)
    return JSValue::undefined();
  JSValue f = args[0];

  int i;
  JSValue acc;
  if (args.size() >= 2 && !args[1].is_undefined()) {
    i = 0;
    acc = args[1];
  } else if (arr->internal->size() >= 1) {
    i = 1;
    acc = (*arr->internal)[0];
  }

  for (; i < arr->internal->size(); i++) {
    acc = f({acc, (*arr->internal)[i], JSValue{static_cast<double>(i)}});
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
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
  for (auto v : *arr->internal) {
    result += v.coerce_to_string() + delimiter;
  }
  result = result.substr(0, result.size() - delimiter.size());
  return JSValue{result};
}

JSValue JSArray::get_property(const JSValue key) {
  if (key.type() == JSValueType::NUMBER) {
    auto idx = static_cast<size_t>(key.coerce_to_double());
    if (idx >= this->internal->size())
      return JSValue::undefined();
    return (*this->internal)[idx];
  }
  return JSBase::get_property(key);
}

JSObject::JSObject()
    : JSBase(), internal{new std::vector<std::pair<JSValue, JSValue>>{}} {};

JSObject::JSObject(std::vector<std::pair<JSValue, JSValue>> data) : JSObject() {
  *this->internal = data;
};

JSValue JSObject::get_property(const JSValue key) {
  auto obj = std::find_if(this->internal->begin(), this->internal->end(),
                          [=](std::pair<JSValue, JSValue> &item) {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == this->internal->end()) {
    return JSBase::get_property(key);
  }
  return (*obj).second;
}

JSFunction::JSFunction(ExternFunc f) : JSBase(), internal{f} {};

JSValue JSFunction::call(JSValue thisArg, std::vector<JSValue> &args) {
  return this->internal(thisArg, args);
}

JSGeneratorAdapter JSGeneratorAdapter::promise_type::get_return_object() {
  return {.h = std::experimental::coroutine_handle<promise_type>::from_promise(
              *this)};
}

std::experimental::suspend_never
JSGeneratorAdapter::promise_type::initial_suspend() {
  return {};
}

std::experimental::suspend_never
JSGeneratorAdapter::promise_type::final_suspend() noexcept {
  return {};
}

void JSGeneratorAdapter::promise_type::return_void() noexcept {
  this->value = std::nullopt;
}

void JSGeneratorAdapter::promise_type::unhandled_exception() {}

std::experimental::suspend_always
JSGeneratorAdapter::promise_type::yield_value(JSValue value) {
  this->value =
      std::optional<shared_ptr<JSValue>>{std::make_shared<JSValue>(value)};
  return {};
}

JSValue iterator_symbol = JSValue::new_object({});
