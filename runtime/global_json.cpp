
#include "global_wasi.hpp"

#include <variant>
#include <vector>

static JSValue json_parse_value(const char **input);

static void eat_whitespace(const char **cur) {
  while (true) {
    bool is_whitespace = **cur == ' ' || **cur == '\n' || **cur == '\t';
    if (!is_whitespace)
      break;
    (*cur)++;
  }
}

static JSValue json_parse_string(const char **input) {
  const char *cur = *input + 1;
  while (*cur != '"') {
    if (*cur == '\\')
      cur++;
    cur++;
  }
  JSValue result{std::string(*input + 1, cur - (*input + 1))};
  *input = ++cur;
  return result;
}

static bool is_digit(const char x) { return x >= '0' && x <= '9'; }

static JSValue json_parse_number(const char **input) {
  const char *cur = *input;
  if (*cur == '-')
    cur++;
  while (true) {
    bool is_valid_number_char = is_digit(*cur) || *cur == '.';
    if (!is_valid_number_char)
      break;
    cur++;
  }
  JSValue result{std::stod(std::string(*input, cur - *input))};
  *input = cur;
  return result;
}

static JSValue json_parse_object(const char **cur) {
  JSObject obj{};
  (*cur)++;
  eat_whitespace(cur);
  while (**cur != '}') {
    auto key = json_parse_string(cur);
    eat_whitespace(cur);
    if (**cur != ':')
      return JSValue::undefined();
    (*cur)++;
    eat_whitespace(cur);
    auto value = json_parse_value(cur);
    obj.internal.push_back({key, JSValueBinding::with_value(value)});
    eat_whitespace(cur);
    if (**cur == ',')
      (*cur)++;
    eat_whitespace(cur);
  }
  (*cur)++;
  return JSValue{obj};
}

static JSValue json_parse_array(const char **cur) {
  JSArray arr{};
  (*cur)++;
  eat_whitespace(cur);
  while (**cur != ']') {
    auto value = json_parse_value(cur);
    arr.internal.push_back(JSValueBinding::with_value(value));
    eat_whitespace(cur);
    if (**cur == ',')
      (*cur)++;
    eat_whitespace(cur);
  }
  (*cur)++;
  return JSValue{arr};
}

static JSValue json_parse_value(const char **input) {
  eat_whitespace(input);
  if (**input == '"')
    return json_parse_string(input);
  if (**input == '{')
    return json_parse_object(input);
  if (**input == '[')
    return json_parse_array(input);
  if (is_digit(**input) || **input == '.' || **input == '-')
    return json_parse_number(input);
  return JSValue::undefined();
}

static JSValue json_parse(JSValue thisArg, std::vector<JSValue> &args) {
  if (args[0].type() != JSValueType::STRING)
    return JSValue::undefined();
  std::string input = args[0].coerce_to_string();
  const char *c = input.c_str();
  return json_parse_value(&c);
}

static std::string json_stringify_value(JSValue v);

static std::string json_stringify_object(JSObject v) {
  std::string result = "{";
  for (auto v : v.internal) {
    result += json_stringify_value(v.first);
    result += ":";
    result += json_stringify_value(v.second.get());
    result += ",";
  }
  result = result.substr(0, result.size() - 1);
  result += "}";
  return result;
}

static std::string json_stringify_array(JSArray v) {
  std::string result = "[";
  for (auto v : v.internal) {
    result += json_stringify_value(v.get());
    result += ",";
  }
  if (v.internal.size() >= 1) {
    result = result.substr(0, result.size() - 1);
  }
  result += "]";
  return result;
}

static std::string json_stringify_number(JSNumber v) {
  return std::to_string(v.internal);
}

static std::string json_stringify_string(JSString v) {
  return std::string("\"" + v.internal + "\"");
}

static std::string json_stringify_value(JSValue v) {
  if (v.type() == JSValueType::ARRAY)
    return json_stringify_array(*std::get<JSValueType::ARRAY>(*v.internal));
  if (v.type() == JSValueType::OBJECT)
    return json_stringify_object(*std::get<JSValueType::OBJECT>(*v.internal));
  if (v.type() == JSValueType::NUMBER)
    return json_stringify_number(std::get<JSValueType::NUMBER>(*v.internal));
  if (v.type() == JSValueType::STRING)
    return json_stringify_string(std::get<JSValueType::STRING>(*v.internal));
  return "<IDK MAN>";
}

static JSValue json_stringify(JSValue thisArg, std::vector<JSValue> &args) {
  return JSValue{json_stringify_value(args[0])};
}

JSValue create_JSON_global() {
  JSValue global = JSValue::new_object(
      {{JSValue{"parse"}, JSValue::new_function(&json_parse)},
       {JSValue{"stringify"}, JSValue::new_function(&json_stringify)}});

  return global;
}
