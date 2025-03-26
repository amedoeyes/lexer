export module lexer:lexer;

import std;

import :context;
import :token;
import :token_definition;

export namespace lexer {

struct lexer_error {
	std::string message;
	char ch;
	std::size_t line;
	std::size_t column;
};

template<typename T>
class lexer {
public:
	explicit lexer(const std::string& buffer) : context_{buffer} {}

	lexer() : context_{""} {}

	auto define(const token_definition<T>& definition) -> void {
		definitions_.emplace_back(definition);
	}

	auto define(const matcher& matcher, const tokenizer<T>& tokenizer) -> void {
		definitions_.emplace_back(matcher, tokenizer);
	}

	[[nodiscard]]
	auto next() -> std::expected<token<T>, lexer_error> {
		for (const auto& def : definitions_) {
			if (def.matcher(context_)) {
				const auto start_column = context_.column();
				const auto start_line = context_.line();

				auto result = def.tokenizer(context_);
				if (!result) return error(result.error());

				if (auto token = *result; token) {
					token->start_line = start_line;
					token->start_column = start_column;
					token->end_column = context_.column() - 1;
					token->end_line = context_.line();
					return *token;
				}
			}
		}

		return error("undefined matcher for character");
	}

	auto set_buffer(const std::string& buffer) -> void {
		context_ = context{buffer};
	}

private:
	context context_;
	std::vector<token_definition<T>> definitions_;

	auto error(std::string_view msg) -> std::unexpected<lexer_error> {
		return std::unexpected{lexer_error{
			.message = std::string{msg},
			.ch = context_.curr(),
			.line = context_.line(),
			.column = context_.column(),
		}};
	}
};

} // namespace lexer
