export module lexer:context;

import std;

export namespace lexer {

constexpr auto end_of_file = char(-1);

class context {
public:
	explicit context(std::string buffer) : buffer_{std::move(buffer)} {}

	auto next(std::size_t n = 1) -> void {
		for (; n > 0 && curr_ < buffer_.size(); --n, ++curr_) {
			if (buffer_[curr_] == '\n') {
				++line_;
				column_ = 1;
			} else {
				++column_;
			}
		}
	}

	auto prev(std::size_t n = 1) -> void {
		for (; n > 0 && curr_ > 0; --n, --curr_) {
			if (buffer_[curr_ - 1] == '\n') {
				--line_;
				column_ = (line_ == 1) ? curr_ : (curr_ - buffer_.rfind('\n', curr_ - 2));
			} else {
				--column_;
			}
		}
	}

	[[nodiscard]]
	auto curr() const -> char {
		return curr_ < buffer_.size() ? buffer_[curr_] : end_of_file;
	}

	[[nodiscard]]
	auto index() const -> std::size_t {
		return curr_;
	}

	[[nodiscard]]
	auto substr(std::size_t pos, std::size_t n) const -> std::string_view {
		return std::string_view(buffer_).substr(pos, n);
	}

	[[nodiscard]]
	auto substr(std::size_t n) const -> std::string_view {
		return substr(curr_, n);
	}

	[[nodiscard]]
	auto extract(std::size_t n) -> std::string_view {
		auto result = substr(n);
		next(n);
		return result;
	}

	[[nodiscard]]
	auto extract_if(std::string_view sv) -> std::optional<std::string_view> {
		if (match(sv)) return extract(sv.size());
		return std::nullopt;
	}

	[[nodiscard]]
	auto match(char ch) const -> bool {
		return curr() == ch;
	}

	[[nodiscard]]
	auto match(std::string_view sv) const -> bool {
		return substr(sv.size()) == sv;
	}

	template<typename Predicate>
		requires std::predicate<Predicate, char>
	[[nodiscard]]
	auto match(Predicate pred) const -> bool {
		return pred(curr());
	}

	[[nodiscard]]
	auto match(int (*pred)(int)) const -> bool {
		return pred(static_cast<unsigned char>(curr())) != 0;
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
	std::string buffer_;
	std::size_t curr_ = 0;
	std::size_t line_ = 1;
	std::size_t column_ = 1;
};

} // namespace lexer
