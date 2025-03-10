export module lexer;

import std;

export namespace lexer {

template <typename T>
struct token {
	T type;
	std::string lexeme;
	std::size_t start_column{};
	std::size_t end_column{};
	std::size_t start_line{};
	std::size_t end_line{};

	token() = default;
	token(T type, std::string_view lexeme) : type{type}, lexeme{lexeme} {}
	token(T type, char lexeme) : type{type}, lexeme{lexeme} {}
};

constexpr auto end_of_file = char(-1);

class context {
public:
	explicit context(std::string input) : input_(std::move(input)) {};

	[[nodiscard]] auto curr() const -> char { return curr_ < input_.size() ? input_[curr_] : end_of_file; }

	auto next(std::size_t n = 1) -> void {
		for (; n > 0 && curr_ < input_.size(); --n, ++curr_) {
			if (input_[curr_] == '\n') {
				++line_;
				column_ = 1;
			} else {
				++column_;
			}
		}
	}

	auto prev(std::size_t n = 1) -> void {
		for (; n > 0 && curr_ > 0; --n, --curr_) {
			if (input_[curr_ - 1] == '\n') {
				--line_;
				column_ = (line_ == 1) ? curr_ : (curr_ - input_.rfind('\n', curr_ - 2));
			} else {
				--column_;
			}
		}
	}

	[[nodiscard]] auto substr(std::size_t end) const -> std::string_view {
		return std::string_view(input_).substr(curr_, std::min(end, input_.size() - curr_));
	}

	[[nodiscard]] auto line() const -> std::size_t { return line_; }
	[[nodiscard]] auto column() const -> std::size_t { return column_; }

private:
	std::string input_;
	std::size_t curr_ = 0;
	std::size_t line_ = 1;
	std::size_t column_ = 1;
};

template <typename T>
class lexer {
public:
	using token_type = token<T>;
	using matcher_type = std::function<bool(char)>;
	using tokenizer_type = std::function<std::optional<token_type>(context&)>;

	explicit lexer(const std::string& input) : context_(input) {};

	auto define_token(const matcher_type& matcher, const tokenizer_type& tokenizer) -> void {
		tokenizers_.emplace_back(matcher, tokenizer);
	}

	auto define_token(T type, char chr) -> void {
		definitions_.emplace_back(
			[chr](auto ch) { return ch == chr; },
			[type, chr](auto& ctx) {
				ctx.next();
				return token{type, chr};
			}
		);
	}

	auto define_token(T type, std::string_view str) -> void {
		definitions_.emplace_back(
			[str](auto c) { return str.starts_with(c); },
			[type, str](auto& ctx) -> token_result<T> {
				if (str == ctx.substr(str.size())) {
					ctx.next(str.size());
					return token{type, str};
				}
				return std::nullopt;
			}
		);
	}

	auto next_token() -> std::expected<token<T>, std::string> {
		for (const auto& [matcher, tokenizer] : tokenizers_) {
			if (matcher(context_.curr())) {
				const auto start_column = context_.column();
				const auto start_line = context_.line();
				auto result = tokenizer(context_);
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
	std::vector<std::pair<matcher_type, tokenizer_type>> tokenizers_;
};

} // namespace lexer
