module;

#include <cstddef>
#include <format>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

export module lexer;

namespace lexer {

export template <typename T>
struct token {
	T type;
	std::string value;
	std::size_t line{};
	std::size_t column{};
};

class context {
public:
	explicit context(std::string input) : input_(std::move(input)) {};

	[[nodiscard]] auto curr() const -> char { return curr_ < input_.size() ? input_[curr_] : '\0'; }

	auto next() -> void {
		if (curr_ < input_.size()) {
			if (input_[curr_] == '\n') {
				++line_;
				column_ = 1;
			} else {
				++column_;
			}
			++curr_;
		}
	}

	auto prev() -> void {
		if (curr_ > 0) {
			--curr_;
			if (input_[curr_] == '\n') {
				--line_;
				column_ = 1;
			} else {
				--column_;
			}
		}
	}

	[[nodiscard]] auto line() const -> std::size_t { return line_; }
	[[nodiscard]] auto column() const -> std::size_t { return column_; }

private:
	std::string input_;
	std::size_t curr_ = 0;
	std::size_t line_ = 1;
	std::size_t column_ = 1;
};

export template <typename T>
class lexer {
public:
	using token_type = token<T>;
	using matcher_type = std::function<bool(char)>;
	using tokenizer_type = std::function<std::optional<token_type>(context&)>;

	explicit lexer(const std::string& input) : context_(input) {};

	auto define_token(const matcher_type& matcher, const tokenizer_type& tokenizer) -> void {
		tokenizers_.emplace_back(matcher, tokenizer);
	}

	auto define_token(T type, char ch) -> void {
		tokenizers_.emplace_back(
			[ch](auto c) { return c == ch; },
			[type, ch](auto& ctx) {
				ctx.next();
				return token_type{.type = type, .value = {ch}};
			}
		);
	}

	auto next_token() -> token_type {
		while (context_.curr() != '\0') {
			for (const auto& [matcher, tokenizer] : tokenizers_) {
				if (matcher(context_.curr())) {
					const auto start_column = context_.column();
					const auto start_line = context_.line();
					try {
						auto token = tokenizer(context_);
						if (token.has_value()) {
							token->line = start_line;
							token->column = start_column;
							return *token;
						}
					} catch (const std::runtime_error& e) {
						throw std::runtime_error(std::format("{}:{}: {}", context_.line(), context_.column(), e.what()));
					}
				}
			}
			throw std::runtime_error(
				std::format("{}:{}: undefined matcher for character '{}'", context_.line(), context_.column(), context_.curr())
			);
		}
		std::unreachable();
	}

private:
	context context_;
	std::vector<std::pair<matcher_type, tokenizer_type>> tokenizers_;
};

} // namespace lexer
