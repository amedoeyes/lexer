export module lexer:lexer;

import std;

import :token;
import :token_definition;
import :context;

export namespace lexer {

template <typename T>
class lexer {
public:
	explicit lexer(const std::string& input) : context_(input) {};

	auto define_token(const token_definition<T>& definition) -> void { definitions_.emplace_back(definition); }

	auto define_token(const matcher& matcher, const tokenizer<T>& tokenizer) -> void {
		definitions_.emplace_back(matcher, tokenizer);
	}

	auto next_token() -> std::expected<token<T>, std::string> {
		for (const auto& def : definitions_) {
			if (def.matcher(context_.curr())) {
				const auto start_column = context_.column();
				const auto start_line = context_.line();
				auto result = def.tokenizer(context_);
				if (!result) return std::unexpected(result.error());
				auto token = *result;
				if (token.has_value()) {
					token->start_line = start_line;
					token->start_column = start_column;
					token->end_column = context_.column() - 1;
					token->end_line = context_.line();
					return *token;
				}
			}
		}
		return std::unexpected(std::format("undefined matcher for character '{}'", context_.curr()));
	}

private:
	context context_;
	std::vector<token_definition<T>> definitions_;
};

} // namespace lexer
