#include "global_json.hpp"
#include "exceptions.hpp"
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
  std::string output;
  while (*cur != '"') {
    if (*cur == '\\') {
      cur++;
    }
    output += *cur;
    cur++;
  }
  *input = ++cur;
  return JSValue{output};
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
      js_throw(JSValue{"Expected `:` after property name"});
    (*cur)++;
    eat_whitespace(cur);
    auto value = json_parse_value(cur);
    obj.internal->push_back({key, value});
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
    arr.internal->push_back(value);
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
  if (*input[0] == 't' && *input[1] == 'r' && *input[2] == 'u' &&
      *input[3] == 'e') {
    *input += 4;
    return JSValue{true};
  }
  if (*input[0] == 'f' && *input[1] == 'a' && *input[2] == 'l' &&
      *input[3] == 's' && *input[4] == 'e') {
    *input += 5;
    return JSValue{false};
  }
  js_throw(JSValue{"Unexpected token"});
  return JSValue::undefined(); // unreachable
}

static JSValue json_parse(JSValue thisArg, std::vector<JSValue> &args) {
  if (args[0].type() != JSValueType::STRING)
    js_throw(JSValue{"Can only parse strings"});
  std::string input = args[0].coerce_to_string();
  const char *c = input.c_str();
  return json_parse_value(&c);
}

static std::string json_stringify_value(JSValue v);

static std::string json_stringify_object(JSObject v) {
  std::string result = "{";
  for (auto v : *v.internal) {
    result += json_stringify_value(v.first);
    result += ":";
    result += json_stringify_value(v.second);
    result += ",";
  }
  result = result.substr(0, result.size() - 1);
  result += "}";
  return result;
}

static std::string json_stringify_array(JSArray v) {
  std::string result = "[";
  for (auto v : *v.internal) {
    result += json_stringify_value(v);
    result += ",";
  }
  if (v.internal->size() >= 1) {
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
    return json_stringify_array(*std::get<JSValueType::ARRAY>(*v.value));
  if (v.type() == JSValueType::OBJECT)
    return json_stringify_object(*std::get<JSValueType::OBJECT>(*v.value));
  if (v.type() == JSValueType::NUMBER)
    return json_stringify_number(std::get<JSValueType::NUMBER>(*v.value));
  if (v.type() == JSValueType::STRING)
    return json_stringify_string(std::get<JSValueType::STRING>(*v.value));
  if (v.type() == JSValueType::BOOL)
    return v.coerce_to_string();
  return std::string("<IDK MAN>");
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
