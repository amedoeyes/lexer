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
	case unknown: return "unknown";
	default: std::unreachable();
	}
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

class json_parser {
public:
	explicit json_parser(const std::string& input) : lexer_{input} {
		lexer_.define(lexer::definitions::skip_whitespace<token_type>);
		lexer_.define(lexer::definitions::single_char<token_type::lbrace, '{'>);
		lexer_.define(lexer::definitions::single_char<token_type::rbrace, '}'>);
		lexer_.define(lexer::definitions::single_char<token_type::lbracket, '['>);
		lexer_.define(lexer::definitions::single_char<token_type::rbracket, ']'>);
		lexer_.define(lexer::definitions::single_char<token_type::colon, ':'>);
		lexer_.define(lexer::definitions::single_char<token_type::comma, ','>);
		lexer_.define(lexer::definitions::multi_char<token_type::null, 'n', 'u', 'l', 'l'>);
		lexer_.define(lexer::definitions::boolean<token_type::boolean>);
		lexer_.define(lexer::definitions::string<token_type::string>);
		lexer_.define(lexer::definitions::number<token_type::number>);
		lexer_.define(lexer::definitions::end_of_file<token_type::eof>);
		lexer_.define(lexer::definitions::anything<token_type::unknown>);
	}

	auto parse() -> std::expected<json_value, std::string> {
		if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};
		return parse_value();
	}

private:
	lexer::lexer<token_type> lexer_;
	lexer::token<token_type> token_;

	auto advance() -> std::expected<void, std::string> {
		auto result = lexer_.next();
		if (!result) {
			const auto& error = result.error();
			return std::unexpected{std::format("{}:{}: {}: '{}'", error.line, error.column, error.message, error.ch)};
		}
		token_ = *result;
		return {};
	}

	[[nodiscard]]
	auto error(std::string_view message) const -> std::unexpected<std::string> {
		return std::unexpected{std::format("{}:{}: {}", token_.end_line, token_.end_column, message)};
	}

	[[nodiscard]]
	auto match(token_type type) const -> bool {
		return token_.type == type;
	}

	[[nodiscard]]
	auto curr() const -> std::string_view {
		return token_.lexeme;
	}

	[[nodiscard]]
	auto parse_string() const -> json_string {
		return std::string{curr()};
	}

	[[nodiscard]]
	auto parse_number() const -> std::expected<json_number, std::string> {
		try {
			return std::stod(std::string{curr()});
		} catch (const std::exception&) {
			return error("invalid number format");
		}
	}

	[[nodiscard]]
	auto parse_boolean() const -> json_boolean {
		return curr() == "true";
	}

	[[nodiscard]]
	auto parse_null() -> json_null {
		return {};
	}

	[[nodiscard]]
	auto parse_object() -> std::expected<json_object, std::string> {
		auto object = json_object{};
		bool expecting_key = true;

		if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};

		while (!match(token_type::eof)) {
			if (match(token_type::rbrace)) return object;

			if (!expecting_key) return error("expected comma");
			if (!match(token_type::string)) return error("expected string key");

			const auto key = std::string{curr()};

			if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};

			if (!match(token_type::colon)) return error("expected colon");

			if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};

			const auto value = parse_value();
			if (!value) return std::unexpected{value.error()};

			object[key] = *value;
			expecting_key = false;

			if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};
			if (match(token_type::comma)) {
				if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};
				expecting_key = true;
			}
		}

		return error("expected closing brace");
	}

	[[nodiscard]]
	auto parse_array() -> std::expected<json_array, std::string> {
		auto array = json_array{};
		bool expecting_value = true;

		if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};

		while (!match(token_type::eof)) {
			if (match(token_type::rbracket)) return array;

			if (!expecting_value) return error("expected comma");

			const auto value = parse_value();
			if (!value) return std::unexpected{value.error()};

			array.emplace_back(*value);
			expecting_value = false;

			if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};
			if (match(token_type::comma)) {
				if (const auto adv = advance(); !adv) return std::unexpected{adv.error()};
				expecting_value = true;
			}
		}

		return error("expected closing bracket");
	}

	auto parse_value() -> std::expected<json_value, std::string> {
		switch (token_.type) {
			using enum token_type;
		case string: return parse_string();
		case number: return parse_number();
		case boolean: return parse_boolean();
		case null: return parse_null();
		case lbrace: return parse_object();
		case lbracket: return parse_array();
		case eof: return error("unexpected end of input");
		case unknown: return error(std::format("unknown token '{}'", curr()));
		default: return error(std::format("unexpected token '{}'", curr()));
		}
	}
};

void print_json_value(const json_value& value, std::int32_t indent = 0) {
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

constexpr auto input = R"(
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

auto main() -> int {
	auto parser = json_parser(input);
	auto json = parser.parse();
	if (!json) {
		std::println(std::cerr, "{}", json.error());
		return 1;
	}
	print_json_value(*json);
}
