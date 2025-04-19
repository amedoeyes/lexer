import std;
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
	unknown,
};

auto token_name(token_type token) -> std::string {
	switch (token) {
		using enum token_type;
		case lbrace:   return "lbrace";
		case rbrace:   return "rbrace";
		case lbracket: return "lbracket";
		case rbracket: return "rbracket";
		case colon:    return "colon";
		case comma:    return "comma";
		case string:   return "string";
		case number:   return "number";
		case boolean:  return "boolean";
		case null:     return "null";
		case eof:      return "eof";
		case unknown:  return "unknown";
		default:       std::unreachable();
	}
}

struct json;

using json_number = double;
using json_string = std::string;
using json_boolean = bool;
using json_null = std::nullptr_t;
using json_array = std::vector<json>;
using json_object = std::unordered_map<std::string, json>;

struct json : std::variant<json_number, json_string, json_boolean, json_null, json_array, json_object> {
	using std::variant<json_number, json_string, json_boolean, json_null, json_array, json_object>::variant;
};

class json_parser {
public:
	explicit json_parser(std::span<const lexer::token<token_type>> tokens) : tokens_{tokens} {}

	auto parse() -> std::expected<json, std::string> {
		switch (token().type) {
			using enum token_type;
			case string:   return parse_string();
			case number:   return parse_number();
			case boolean:  return parse_boolean();
			case null:     return parse_null();
			case lbrace:   return parse_object();
			case lbracket: return parse_array();
			case eof:      return error("unexpected end of input");
			case unknown:  return error(std::format("unknown token '{}'", lexeme()));
			default:       return error(std::format("unexpected token '{}'", lexeme()));
		}
	}

private:
	std::span<const lexer::token<token_type>> tokens_;
	std::size_t curr_ = 0;

	auto next() -> void {
		if (curr_ < tokens_.size()) ++curr_;
	}

	[[nodiscard]]
	auto error(std::string_view message) const -> std::unexpected<std::string> {
		return std::unexpected{std::format("{}:{}: {}", token().end_line, token().end_column, message)};
	}

	[[nodiscard]]
	auto token() const -> lexer::token<token_type> {
		return tokens_[curr_];
	}

	[[nodiscard]]
	auto match(token_type type) const -> bool {
		return token().type == type;
	}

	[[nodiscard]]
	auto lexeme() const -> std::string_view {
		return token().lexeme;
	}

	[[nodiscard]]
	auto parse_string() -> json_string {
		const auto str = std::string{lexeme()};
		next();
		return str;
	}

	[[nodiscard]]
	auto parse_number() -> std::expected<json_number, std::string> {
		try {
			const auto number = std::stod(std::string{lexeme()});
			next();
			return number;
		} catch (const std::exception&) {
			return error("invalid number format");
		}
	}

	[[nodiscard]]
	auto parse_boolean() -> json_boolean {
		const auto boolean = lexeme() == "true";
		next();
		return boolean;
	}

	[[nodiscard]]
	auto parse_null() -> json_null {
		next();
		return {};
	}

	[[nodiscard]]
	auto parse_object() -> std::expected<json_object, std::string> {
		next();

		auto object = json_object{};

		while (!match(token_type::eof)) {
			if (!match(token_type::string)) return error("expected string key");
			const auto key = std::string{lexeme()};

			next();
			if (!match(token_type::colon)) return error("expected colon");

			next();
			const auto value = parse();
			if (!value) return std::unexpected{value.error()};

			object[key] = *value;

			if (match(token_type::rbrace)) {
				next();
				return object;
			}

			if (match(token_type::comma)) next();
			else return error("expected comma");
		}

		return error("expected closing brace");
	}

	[[nodiscard]]
	auto parse_array() -> std::expected<json_array, std::string> {
		next();

		auto array = json_array{};

		while (!match(token_type::eof)) {
			const auto value = parse();
			if (!value) return std::unexpected{value.error()};
			array.emplace_back(*value);

			if (match(token_type::rbracket)) {
				next();
				return array;
			}

			if (match(token_type::comma)) next();
			else return error("expected comma");
		}

		return error("expected closing bracket");
	}
};

void print_json(const json& value, std::int32_t indent = 0) {
	auto print_indent = [](auto level) {
		for (auto _ : std::views::iota(0, level)) std::print("  ");
	};

	if (std::holds_alternative<json_string>(value)) {
		std::print("\"{}\"", std::get<json_string>(value));
	} else if (std::holds_alternative<json_number>(value)) {
		std::print("{}", std::get<json_number>(value));
	} else if (std::holds_alternative<json_boolean>(value)) {
		std::print("{}", std::get<json_boolean>(value));
	} else if (std::holds_alternative<json_null>(value)) {
		std::print("null");
	} else if (std::holds_alternative<json_array>(value)) {
		const auto& arr = std::get<json_array>(value);
		if (arr.empty()) {
			std::print("[]");
			return;
		}
		std::println("[");
		for (const auto i : std::views::iota(0ul, arr.size())) {
			print_indent(indent + 1);
			print_json(arr[i], indent + 1);
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
		auto i = 0;
		for (const auto& [key, val] : obj) {
			print_indent(indent + 1);
			std::print("\"{}\": ", key);
			print_json(val, indent + 1);
			if (i < obj.size() - 1) std::print(",");
			std::println("");
			++i;
		}
		print_indent(indent);
		std::print("}}");
	}
}

constexpr auto buffer = R"(
{
  "shopName": "Purrfect Cat Shop",
  "open": true,
  "owner": null,
  "location": {
    "city": "Cat City",
    "zip": 90210
  },
  "products": [
    {
      "id": 1,
      "name": "Feather Wand",
      "price": 9.99,
      "inStock": true
    },
    {
      "id": 2,
      "name": "Catnip Toy",
      "price": 4.5,
      "inStock": false
    }
  ]
}
)";

auto lex(std::string_view buffer) -> std::expected<std::vector<lexer::token<token_type>>, std::string> {
	auto lexer = lexer::lexer<token_type>{buffer};
	lexer.define(lexer::definitions::skip_whitespace<token_type>);
	lexer.define(lexer::definitions::single_char<token_type::lbrace, '{'>);
	lexer.define(lexer::definitions::single_char<token_type::rbrace, '}'>);
	lexer.define(lexer::definitions::single_char<token_type::lbracket, '['>);
	lexer.define(lexer::definitions::single_char<token_type::rbracket, ']'>);
	lexer.define(lexer::definitions::single_char<token_type::colon, ':'>);
	lexer.define(lexer::definitions::single_char<token_type::comma, ','>);
	lexer.define(lexer::definitions::multi_char<token_type::null, 'n', 'u', 'l', 'l'>);
	lexer.define(lexer::definitions::boolean<token_type::boolean>);
	lexer.define(lexer::definitions::string<token_type::string>);
	lexer.define(lexer::definitions::number<token_type::number>);
	lexer.define(lexer::definitions::end_of_file<token_type::eof>);
	lexer.define(lexer::definitions::anything<token_type::unknown>);

	auto tokens = std::vector<lexer::token<token_type>>{};

	while (true) {
		const auto token = lexer.next();
		if (!token) {
			auto error = token.error();
			return std::unexpected{std::format("{}:{}: {}: '{}'", error.line, error.column, error.message, error.ch)};
		}

		tokens.emplace_back(*token);

		if (token->type == token_type::eof) break;
	}

	return tokens;
}

auto parse(std::string_view buffer) -> std::expected<json, std::string> {
	const auto tokens = lex(buffer);
	if (!tokens) {
		return std::unexpected{tokens.error()};
	}
	return json_parser{*tokens}.parse();
}

auto main() -> int {
	auto json = parse(buffer);
	if (!json) {
		std::println(std::cerr, "{}", json.error());
		return 1;
	}
	print_json(*json);
}
