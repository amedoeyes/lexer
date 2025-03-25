export module lexer:token;

import std;

export namespace lexer {

template<typename T>
struct token {
	T type;
	std::string_view lexeme;
	std::size_t start_column{};
	std::size_t end_column{};
	std::size_t start_line{};
	std::size_t end_line{};

	token() = default;

	explicit token(T type) : type{type} {}

	token(T type, std::string_view lexeme) : type{type}, lexeme{lexeme} {}
};

} // namespace lexer
