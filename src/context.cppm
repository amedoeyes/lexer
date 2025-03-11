export module lexer:context;

import std;

export namespace lexer {

constexpr auto end_of_file = char(-1);

class context {
public:
	explicit context(std::string input) : input_(std::move(input)) {}

	[[nodiscard]]
	auto curr() const -> char {
		return curr_ < input_.size() ? input_[curr_] : end_of_file;
	}

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

	[[nodiscard]]
	auto substr(std::size_t end) const -> std::string_view {
		return std::string_view(input_).substr(curr_, std::min(end, input_.size() - curr_));
	}

	[[nodiscard]]
	auto line() const -> std::size_t {
		return line_;
	}

	[[nodiscard]]
	auto column() const -> std::size_t {
		return column_;
	}

private:
	std::string input_;
	std::size_t curr_ = 0;
	std::size_t line_ = 1;
	std::size_t column_ = 1;
};

} // namespace lexer
