export module lexer:definitions;

import std;

import :token_definition;

export namespace lexer::definitions {

template<auto T, char Chr>
const auto single_char = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match(Chr); },
	[](auto& ctx) -> token_result<decltype(T)> { return token{T, ctx.extract(1)}; },
};

template<auto T, char... Str>
const auto multi_char = token_definition<decltype(T)>{
	[](const auto& ctx) {
		static constexpr auto arr = std::array{Str...};
		static constexpr auto sv = std::string_view{arr.data(), arr.size()};
		return ctx.match(sv);
	},
	[](auto& ctx) -> token_result<decltype(T)> { return token{T, ctx.extract(sizeof...(Str))}; }};

template<typename T>
const auto skip_whitespace = token_definition<T>{
	[](const auto& ctx) { return ctx.match(std::isspace); },
	[](auto& ctx) -> token_result<T> {
		while (ctx.match(std::isspace)) ctx.next();
		return std::nullopt;
	},
};

template<auto T>
const auto boolean = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match("true") || ctx.match("false"); },
	[](auto& ctx) -> token_result<decltype(T)> {
		if (const auto sv = ctx.extract_if("true"); sv) return token{T, *sv};
		if (const auto sv = ctx.extract_if("false"); sv) return token{T, *sv};
		return std::nullopt;
	},
};

template<auto T>
const auto end_of_file = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match(lexer::end_of_file); },
	[](auto&) -> token_result<decltype(T)> { return token{T}; },
};

template<auto T>
const auto anything = token_definition<decltype(T)>{
	[](const auto&) { return true; },
	[](auto& ctx) -> token_result<decltype(T)> { return token{T, ctx.extract(1)}; },
};

template<auto T>
const auto string_literal = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match('"') || ctx.match('\''); },
	[](auto& ctx) -> token_result<decltype(T)> {
		const auto quote_type = ctx.curr();

		ctx.next();
		const auto start = ctx.index();

		while (true) {
			if (ctx.match(lexer::end_of_file)) return std::unexpected{"unterminated string literal"};
			if (ctx.match('\n')) return std::unexpected{"newline in string literal"};

			if (ctx.match(quote_type)) break;

			if (ctx.match('\\')) {
				ctx.next();
				if (!ctx.match(lexer::end_of_file) && ctx.match('\n')) ctx.next();
			} else {
				ctx.next();
			}
		}

		const auto end = ctx.index();
		ctx.next();

		return token{T, ctx.substr(start, end - start)};
	},
};

template<auto T, bool Decimal = true, bool Scientific = true>
const auto number = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match(std::isdigit); },
	[](auto& ctx) -> token_result<decltype(T)> {
		const auto start = ctx.index();

		while (ctx.match(std::isdigit)) ctx.next();

		if constexpr (Decimal) {
			if (ctx.match('.')) {
				ctx.next();
				if (!ctx.match(std::isdigit)) return std::unexpected{"invalid decimal number"};
				while (ctx.match(std::isdigit)) ctx.next();
			}
		}

		if constexpr (Scientific) {
			if (ctx.match('e') || ctx.match('E')) {
				ctx.next();
				if (ctx.match('+') || ctx.match('-')) ctx.next();
				if (!ctx.match(std::isdigit)) return std::unexpected{"invalid scientific notation"};
				while (ctx.match(std::isdigit)) ctx.next();
			}
		}

		return token{T, ctx.substr(start, ctx.index() - start)};
	},
};

template<auto T>
const auto identifier = token_definition<decltype(T)>{
	[](const auto& ctx) { return ctx.match(std::isalpha) || ctx.match('_'); },
	[](auto& ctx) -> token_result<decltype(T)> {
		const auto start = ctx.index();
		while (ctx.match(std::isalnum) || ctx.match('_')) ctx.next();
		return token{T, ctx.substr(start, ctx.index() - start)};
	},
};

} // namespace lexer::definitions
