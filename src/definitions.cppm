export module lexer:definitions;

import std;

import :token_definition;

export namespace lexer::definitions {

template <auto T, char Chr>
const auto single_char_token = token_definition<decltype(T)>{
	[](auto ch) { return ch == Chr; },
	[](auto& ctx) -> token_result<decltype(T)> {
		ctx.next();
		return token{T, std::string{1, Chr}};
	}
};

template <auto T, char First, char... Rest>
const auto multi_char_token = token_definition<decltype(T)>{
	[](auto ch) { return ch == First; },
	[](auto& ctx) -> token_result<decltype(T)> {
		constexpr auto str = std::array<char, sizeof...(Rest) + 1>{First, Rest...};
		if (ctx.substr(str.size()) == std::string_view{str.data(), str.size()}) {
			ctx.next(str.size());
			return token{T, std::string{str.data(), str.size()}};
		}
		return std::nullopt;
	}
};

template <typename T>
const auto skip_whitespace = token_definition<T>{
	[](auto ch) { return std::isspace(ch) != 0; },
	[](auto& ctx) -> token_result<T> {
		while (std::isspace(ctx.curr())) ctx.next();
		return std::nullopt;
	}
};

template <auto T>
const auto boolean = token_definition<decltype(T)>{
	[](auto ch) { return ch == 't' || ch == 'f'; },
	[](auto& ctx) -> token_result<decltype(T)> {
		for (const auto& str : std::array{std::string{"true"}, std::string{"false"}}) {
			if (str == ctx.substr(str.size())) {
				ctx.next(str.size());
				return token{T, str};
			}
		}
		return std::nullopt;
	}
};

template <auto T>
const auto end_of_file = token_definition<decltype(T)>{
	[](auto ch) { return ch == lexer::end_of_file; },
	[](auto&) -> token_result<decltype(T)> { return token{T, ""}; },
};

template <auto T>
const auto anything = token_definition<decltype(T)>{
	[](auto) { return true; },
	[](auto& ctx) -> token_result<decltype(T)> {
		const auto lexeme = std::string{ctx.curr()};
		ctx.next();
		return token{T, lexeme};
	}
};

template <auto T>
const auto string_literal = token_definition<decltype(T)>{
	[](auto ch) { return ch == '"' || ch == '\''; },
	[](auto& ctx) -> token_result<decltype(T)> {
		auto lexeme = std::string{};
		auto quote_type = ctx.curr();
		ctx.next();
		while (ctx.curr() != lexer::end_of_file && ctx.curr() != quote_type) {
			if (ctx.curr() == '\\') {
				lexeme += ctx.curr();
				ctx.next();
				if (ctx.curr() != lexer::end_of_file) {
					lexeme += ctx.curr();
					ctx.next();
				}
			} else {
				lexeme += ctx.curr();
				ctx.next();
			}
		}
		if (ctx.curr() != quote_type) return std::unexpected("unterminated string literal");
		ctx.next();
		return token{T, lexeme};
	}
};

template <auto T, bool Decimal = true, bool Scientific = true>
const auto number = token_definition<decltype(T)>{
	[](auto ch) { return std::isdigit(ch) != 0; },
	[](auto& ctx) -> token_result<decltype(T)> {
		auto lexeme = std::string{};
		auto has_decimal = false;
		auto has_exponent = false;
		while (std::isdigit(ctx.curr()) || (Decimal && ctx.curr() == '.' && !has_decimal)
					 || (Scientific && (ctx.curr() == 'e' || ctx.curr() == 'E') && !has_exponent)) {
			if (ctx.curr() == '.') {
				if (has_decimal) break;
				has_decimal = true;
			}
			if (ctx.curr() == 'e' || ctx.curr() == 'E') {
				if (has_exponent) break;
				has_exponent = true;
				lexeme += ctx.curr();
				ctx.next();
				if (ctx.curr() == '+' || ctx.curr() == '-') {
					lexeme += ctx.curr();
					ctx.next();
				}
				continue;
			}
			lexeme += ctx.curr();
			ctx.next();
		}
		return token{T, lexeme};

template<auto T>
const auto identifier = token_definition<decltype(T)>{
	[](auto ch) { return std::isalpha(ch) || ch == '_'; },
	[](auto& ctx) -> token_result<decltype(T)> {
		std::string lexeme;
		while (std::isalnum(ctx.curr()) || ctx.curr() == '_') {
			lexeme += ctx.curr();
			ctx.next();
		}
		return token{T, lexeme};
	},
};

} // namespace lexer::definitions
