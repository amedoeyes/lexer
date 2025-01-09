#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <print>
#include <ranges>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

import lexer;

enum class token_type : std::uint8_t {
	lbrace,
	rbrace,
	lbracket,
	rbracket,
	colon,
	comma,

	string,
	number,
	boolean,
	null,

	eof,
};

auto token_name(token_type token) -> std::string {
	switch (token) {
		using enum token_type;
		case lbrace: return "lbrace";
		case rbrace: return "rbrace";
		case lbracket: return "lbracket";
		case rbracket: return "rbracket";
		case colon: return "colon";
		case comma: return "comma";
		case string: return "string";
		case number: return "number";
		case boolean: return "boolean";
		case null: return "null";
		case eof: return "eof";
	}
	std::unreachable();
}

using json_number = double;
using json_string = std::string;
using json_boolean = bool;
using json_null = std::nullptr_t;
struct json_array;
struct json_object;

using json_value = std::variant<json_number, json_string, json_boolean, json_null, json_array, json_object>;

struct json_array : std::vector<json_value> {
	using std::vector<json_value>::vector;
};

struct json_object : std::unordered_map<std::string, json_value> {
	using std::unordered_map<std::string, json_value>::unordered_map;
};

void print_json_value(const json_value& value, int indent = 0) {
	auto print_indent = [](int level) {
		for (int i = 0; i < level; ++i) std::print("  ");
	};
	if (std::holds_alternative<json_string>(value)) {
		std::print("\"{}\"", std::get<json_string>(value));
	} else if (std::holds_alternative<json_number>(value)) {
		std::print("{}", std::get<json_number>(value));
	} else if (std::holds_alternative<json_boolean>(value)) {
		std::print("{}", std::get<json_boolean>(value) ? "true" : "false");
	} else if (std::holds_alternative<json_null>(value)) {
		std::print("null");
	} else if (std::holds_alternative<json_array>(value)) {
		const auto& arr = std::get<json_array>(value);
		if (arr.empty()) {
			std::print("[]");
			return;
		}
		std::println("[");
		for (std::size_t i = 0; i < arr.size(); ++i) {
			print_indent(indent + 1);
			print_json_value(arr[i], indent + 1);
			if (i < arr.size() - 1) std::print(",");
			std::println("");
		}
		print_indent(indent);
		std::print("]");
	} else if (std::holds_alternative<json_object>(value)) {
		const auto& obj = std::get<json_object>(value);
		if (obj.empty()) {
			std::print("{{}}");
			return;
		}
		std::println("{{");
		std::size_t i = 0;
		for (const auto& [key, val] : obj) {
			print_indent(indent + 1);
			std::print("\"{}\": ", key);
			print_json_value(val, indent + 1);
			if (i < obj.size() - 1) std::print(",");
			std::println("");
			i++;
		}
		print_indent(indent);
		std::print("}}");
	}
}

// TODO: make this a parser object
// TODO: somehow make the object generic
static auto parse_json(std::span<lexer::token<token_type>> tokens, std::size_t& pos) -> json_value {
	auto throw_syntax_error = [&](std::string_view message) {
		if (pos < tokens.size()) {
			const auto& token = tokens[pos];
			throw std::runtime_error(std::format("{}:{}: {}", token.line, token.column, message));
		}
		throw std::runtime_error(std::format("unexpected end of input: {}", message));
	};
	if (pos >= tokens.size()) throw_syntax_error("unexpected end of input");
	const auto& token = tokens[pos];
	++pos;
	switch (token.type) {
		using enum token_type;
		case string: return json_string{token.value};
		case number: try { return json_number{std::stod(token.value)};
			} catch (const std::exception&) {
				throw_syntax_error("invalid number format");
			}
		case boolean: return json_boolean{token.value == "true"};
		case null: return json_null{};
		case lbrace: {
			auto object = json_object{};
			bool expecting_key = true;
			while (pos < tokens.size()) {
				if (tokens[pos].type == rbrace) {
					if (tokens[pos - 1].type == comma) throw_syntax_error("trailing comma");
					++pos;
					return object;
				}
				if (!expecting_key) throw_syntax_error("expected comma");
				if (tokens[pos].type != string) throw_syntax_error("expected string key");
				const auto& key = tokens[pos].value;
				++pos;
				if (pos >= tokens.size() || tokens[pos].type != colon) throw_syntax_error("expected colon");
				++pos;
				object[key] = parse_json(tokens, pos);
				expecting_key = false;
				if (pos < tokens.size() && tokens[pos].type == comma) {
					++pos;
					expecting_key = true;
				}
			}
			throw_syntax_error("expected closing brace");
		}
		case lbracket: {
			auto array = json_array{};
			bool expecting_value = true;
			while (pos < tokens.size()) {
				if (tokens[pos].type == rbracket) {
					if (tokens[pos - 1].type == comma) throw_syntax_error("trailing comma");
					++pos;
					return array;
				}
				if (!expecting_value) throw_syntax_error("expected comma");
				array.emplace_back(parse_json(tokens, pos));
				expecting_value = false;
				if (pos < tokens.size() && tokens[pos].type == comma) {
					++pos;
					expecting_value = true;
				}
			}
			throw_syntax_error("expected closing bracket");
		}
		default: throw_syntax_error(std::format("unexpected token '{}'", token.value));
	}
	std::unreachable();
}

auto main() -> int {
	const auto* file = "./tests/file.json";
	auto input = (std::ostringstream() << std::ifstream(file).rdbuf()).str();
	auto lexer = lexer::lexer<token_type>(input);
	lexer.define_token(
		[](auto ch) { return std::isspace(ch) != 0; },
		[](auto& ctx) {
			while (std::isspace(ctx.curr())) ctx.next();
			return std::nullopt;
		}
	);
	lexer.define_token(token_type::lbrace, '{');
	lexer.define_token(token_type::rbrace, '}');
	lexer.define_token(token_type::lbracket, '[');
	lexer.define_token(token_type::rbracket, ']');
	lexer.define_token(token_type::colon, ':');
	lexer.define_token(token_type::comma, ',');
	lexer.define_token(token_type::eof, '\0');
	lexer.define_token(
		[](auto ch) { return ch == '"'; },
		[](auto& ctx) {
			auto value = std::string();
			ctx.next();
			while (ctx.curr() != '"') {
				if (ctx.curr() == '\n') {
					throw std::runtime_error("unexpected end of string");
				}
				if (ctx.curr() == '\\') {
					ctx.next();
					if (ctx.curr() == '"' || ctx.curr() == '\\' || ctx.curr() == '/' || ctx.curr() == 'b' || ctx.curr() == 'f'
							|| ctx.curr() == 'n' || ctx.curr() == 'r' || ctx.curr() == 't') {
						value += '\\';
					} else {
						throw std::runtime_error("invalid escape character");
					}
				}
				value += ctx.curr();
				ctx.next();
			}
			ctx.next();
			return lexer::token{.type = token_type::string, .value = value};
		}
	);
	lexer.define_token(
		[](auto ch) { return (std::isdigit(ch) != 0); },
		[](auto& ctx) {
			auto value = std::string();
			while (std::isdigit(ctx.curr()) != 0 || ctx.curr() == '.') {
				value += ctx.curr();
				ctx.next();
			}
			return lexer::token{.type = token_type::number, .value = value};
		}
	);
	lexer.define_token(
		[](auto ch) { return ch == 't' || ch == 'f'; },
		[](auto& ctx) {
			auto value = std::string();
			while (std::isalpha(ctx.curr())) {
				value += ctx.curr();
				ctx.next();
			}
			if (value == "true" || value == "false") {
				return std::optional<lexer::token<token_type>>{{.type = token_type::boolean, .value = value}};
			}
			for (auto _ : std::views::iota(0ul, value.size())) ctx.prev();
			return std::optional<lexer::token<token_type>>{};
		}
	);
	lexer.define_token(
		[](auto ch) { return ch == 'n'; },
		[](auto& ctx) {
			auto value = std::string();
			while (std::isalpha(ctx.curr())) {
				value += ctx.curr();
				ctx.next();
			}
			if (value == "null") return std::optional<lexer::token<token_type>>{{.type = token_type::null, .value = value}};
			for (auto _ : std::views::iota(0ul, value.size())) ctx.prev();
			return std::optional<lexer::token<token_type>>{};
		}
	);

	auto tokens = std::vector<lexer::token<token_type>>();

	try {
		auto token = lexer.next_token();
		while (token.type != token_type::eof) {
			tokens.emplace_back(token);
			token = lexer.next_token();
		}
	} catch (const std::runtime_error& e) {
		std::println(std::cerr, "{}:{}", file, e.what());
		return 1;
	}

	try {
		auto pos = 0ul;
		auto json = parse_json(tokens, pos);
		if (pos < tokens.size()) {
			const auto& token = tokens[pos];
			std::println(std::cerr, "{}:{}:{} expected end of file", file, token.line, token.column);
			return 1;
		}
		print_json_value(json);
	} catch (const std::runtime_error& e) {
		std::println(std::cerr, "{}:{}", file, e.what());
		return 1;
	}
}
