module;

#include <format>
#include <functional>
#include <print>
#include <stdexcept>
#include <string>
#include <utility>

export module lexer;

namespace lexer {

export template <typename T>
struct token {
	T type;
	std::string value;
	size_t line{};
	size_t column{};
};

export template <typename T>
class lexer {
public:
	using token_type = token<T>;
	using matcher_type = std::function<bool(char)>;
	using tokenizer_type = std::function<std::optional<
		token_type>(const std::function<char()>&, const std::function<void()>&, const std::function<void()>&)>;

	explicit lexer(std::string text) : input_(std::move(text)) {};

	auto define_token(const matcher_type& matcher, const tokenizer_type& tokenizer) -> void {
		tokenizers_.emplace_back(matcher, tokenizer);
	}

	auto define_token(T type, char ch) -> void {
		tokenizers_.emplace_back(
			[ch](auto c) { return c == ch; },
			[type, ch](const auto&, const auto& next, const auto&) {
				next();
				return token_type{.type = type, .value = {ch}};
			}
		);
	}

	auto next_token() -> token_type {
		while (curr_ <= input_.size()) {
			for (const auto& [matcher, tokenizer] : tokenizers_) {
				if (matcher(curr())) {
					try {
						auto token = tokenizer(
							std::bind(&lexer<T>::curr, this), std::bind(&lexer<T>::next, this), std::bind(&lexer<T>::prev, this)
						);
						if (!token.has_value()) continue;
						(*token).line = line();
						(*token).column = column() - 1;
						return *token;
					} catch (const std::runtime_error& e) {
						throw std::runtime_error(std::format("{}:{}: {}", line(), column(), e.what()));
					}
				}
			}
			throw std::runtime_error(std::format("{}:{}: undefined matcher for character '{}'", line(), column(), curr()));
		}
		std::unreachable();
	}

private:
	std::string input_;
	size_t curr_ = 0;
	std::vector<std::pair<matcher_type, tokenizer_type>> tokenizers_;

	[[nodiscard]] auto line() const -> size_t {
		size_t line = 1;
		for (size_t i = 0; i < curr_; ++i) {
			if (input_[i] == '\n') ++line;
		}
		return line;
	}

	[[nodiscard]] auto column() const -> size_t {
		size_t col = 1;
		for (size_t i = curr_; i > 0; --i) {
			if (input_[i - 1] == '\n') break;
			++col;
		}
		return col;
	}

	[[nodiscard]] auto curr() const -> char { return curr_ < input_.size() ? input_[curr_] : '\0'; }
	auto next() -> void { ++curr_; }
	auto prev() -> void { --curr_; }
};

} // namespace lexer
