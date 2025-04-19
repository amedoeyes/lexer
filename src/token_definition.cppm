export module lexer:token_definition;

import std;

import :context;
import :token;

export namespace lexer {

template<typename T>
using token_result = std::expected<std::optional<token<T>>, std::string>;

template<typename T>
using tokenizer = std::function<token_result<T>(context&)>;

using matcher = std::function<bool(const context&)>;

template<typename T>
struct token_definition {
	matcher matcher;
	tokenizer<T> tokenizer;

	token_definition() noexcept = default;

	token_definition(lexer::matcher matcher, lexer::tokenizer<T> tokenizer) noexcept
		: matcher{std::move(matcher)},
		  tokenizer{std::move(tokenizer)} {}

	auto operator+(const token_definition& rhs) const noexcept -> token_definition {
		return token_definition{
			[this, rhs](const auto& ctx) -> bool { return matcher(ctx) || rhs.matcher(ctx); },
			[this, rhs](auto& ctx) -> token_result<T> {
				if (matcher(ctx)) return tokenizer(ctx);
				return rhs.tokenizer(ctx);
			},
		};
	}
};

} // namespace lexer
