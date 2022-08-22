#include "js_primitives.hpp"
#include "exceptions.hpp"

JSBase::JSBase() {}

JSValue JSUndefined::operator==(JSValue &other) {
  return JSValue{other.is_undefined()};
}

JSValue JSBase::get_property(JSValue key, JSValue parent) {
  auto result = this->get_property_from_list(this->properties, key, parent);
  if (result.has_value()) {
    return result.value();
  }

  return JSValue::with_getter_setter(
      JSValue::new_function(
          [](JSValue thisArg, std::vector<JSValue> &args) mutable -> JSValue {
            return JSValue::undefined();
          }),
      JSValue::new_function(
          [=, that = this](JSValue thisArg,
                           std::vector<JSValue> &args) mutable -> JSValue {
            JSValue v = JSValue::undefined();
            if (args.size() > 0) {
              v = args[0];
            }
            that->properties.push_back({key, v});
            return JSValue::undefined();
          }));
}

std::optional<JSValue> JSBase::get_property_from_list(
    const std::vector<std::pair<JSValue, JSValue>> &list, JSValue key,
    JSValue parent) {
  auto obj = std::find_if(list.begin(), list.end(),
                          [&](const std::pair<JSValue, JSValue> &item) -> bool {
                            return (item.first == key).coerce_to_bool();
                          });
  if (obj == list.end()) {
    return std::nullopt;
  }
  JSValue v = (*obj).second;
  if (v.getter.has_value()) {
    *v.value = (*v.getter)(parent).boxed_value();
  }
  return std::optional{v};
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
  this->properties.push_back(
      {iterator_symbol, JSValue::new_function(&JSArray::iterator_impl)});
};

JSArray::JSArray(std::vector<JSValue> data) : JSArray() {
  for (auto v : data) {
    this->internal->push_back(v);
  }
}

JSValue JSArray::push_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    js_throw(JSValue{"Called push on non-array"});
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
  for (auto v : args) {
    arr->internal->push_back(v);
  }
  return JSValue::undefined();
}

JSValue JSArray::map_impl(JSValue thisArg, std::vector<JSValue> &args) {
  if (thisArg.type() != JSValueType::ARRAY)
    js_throw(JSValue{"Called map on non-array"});
  JSValue f = args[0];
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
    js_throw(JSValue{"Called filter on non-array"});
  JSValue f = args[0];
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
    js_throw(JSValue{"Called reduce on non-array"});
  auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);

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
    js_throw(JSValue{"Called join on non-array"});

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

JSValue JSArray::iterator_impl(JSValue thisArg, std::vector<JSValue> &args) {
  auto gen = JSValue::new_generator_function(
      [=](JSValue thisArg,
          std::vector<JSValue> &args) mutable -> JSGeneratorAdapter {
        if (thisArg.type() != JSValueType::ARRAY) {
          js_throw(JSValue{"Called array iterator with a non-array value"});
        }
        auto arr = std::get<JSValueType::ARRAY>(*thisArg.value);
        for (auto value : *arr->internal) {
          co_yield value;
        }
        co_return;
      });
  gen.set_parent(thisArg);
  return gen(args);
}

JSValue JSArray::get_property(const JSValue key, JSValue parent) {
  if (key.type() == JSValueType::NUMBER) {
    auto idx = static_cast<size_t>(key.coerce_to_double());
    if (idx >= this->internal->size())
      js_throw(JSValue{"Array access out of bounds"});
    return (*this->internal)[idx];
  }
  return JSBase::get_property(key, parent);
}

JSObject::JSObject()
    : JSBase(), internal{new std::vector<std::pair<JSValue, JSValue>>{}} {};

JSObject::JSObject(std::vector<std::pair<JSValue, JSValue>> data) : JSObject() {
  *this->internal = data;
};

JSValue JSObject::get_property(const JSValue key, JSValue parent) {
  auto v = this->get_property_from_list(*this->internal, key, parent);
  if (v.has_value()) {
    return v.value();
  }
  auto proto_v = JSBase::get_property_from_list(this->properties, key, parent);
  if (proto_v.has_value()) {
    return proto_v.value();
  }
  return JSValue::with_getter_setter(
      JSValue::new_function(
          [](JSValue thisArg, std::vector<JSValue> &args) mutable -> JSValue {
            return JSValue::undefined();
          }),
      JSValue::new_function(
          [=, that = *this](JSValue thisArg,
                            std::vector<JSValue> &args) mutable -> JSValue {
            JSValue v = JSValue::undefined();
            if (args.size() > 0) {
              v = args[0];
            }
            that.internal->push_back({key, v});
            return JSValue::undefined();
          }));
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

void JSGeneratorAdapter::promise_type::unhandled_exception() {
  auto ptr = std::current_exception();
  if (ptr) {
    std::rethrow_exception(ptr);
  }
}

std::experimental::suspend_always
JSGeneratorAdapter::promise_type::yield_value(JSValue value) {
  this->value =
      std::optional<shared_ptr<JSValue>>{std::make_shared<JSValue>(value)};
  return {};
}

JSValue iterator_symbol = JSValue::new_object({});

JSIterator::JSIterator() : JSIterator{JSValue::undefined()} {}

JSIterator::JSIterator(JSValue val) : it{std::make_shared<JSValue>(val)} {}

JSIterator::JSIterator(JSValue val, JSValue parent) : JSIterator(val) {
  this->parent = {std::make_shared<JSValue>(parent)};
}

JSIterator JSIterator::end_marker() {
  JSIterator it{};
  it.last_value =
      std::optional{shared_ptr<JSValue>{new JSValue{JSValue::new_object({
          {JSValue{"value"}, JSValue::undefined()},
          {JSValue{"done"}, JSValue{true}},
      })}}};
  return it;
}

JSValue JSIterator::operator*() { return this->value()["value"]; }

JSIterator JSIterator::operator++() {
  if (!this->it->is_undefined()) {
    if (this->parent.has_value()) {
      this->last_value = std::optional{std::make_shared<JSValue>(
          (*this->it)["next"].apply(*this->parent.value(), {}))};
    } else {
      this->last_value =
          std::optional{std::make_shared<JSValue>((*this->it)["next"]({}))};
    }
  }
  return *this;
}

bool JSIterator::operator!=(const JSIterator &other) {
  if (other.last_value.has_value() != this->last_value.has_value()) {
    return true;
  }
  JSValue left = *this->last_value.value();
  JSValue right = *other.last_value.value();
  bool left_done = left["done"].coerce_to_bool();
  bool right_done = right["done"].coerce_to_bool();
  if (left_done && right_done) {
    return false;
  }
  return left_done != right_done ||
         (left["value"] != right["value"]).coerce_to_bool();
}

JSValue JSIterator::value() {
  if (!this->last_value.has_value()) {
    ++(*this);
  }
  return *this->last_value.value();
}
