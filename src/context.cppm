export module lexer:context;

import std;

export namespace lexer {

constexpr auto end_of_file = char(-1);

class context {
public:
	explicit context(std::string_view buffer) noexcept : buffer_{buffer} {}

	auto next(std::size_t n = 1) noexcept -> void {
		for (; n > 0 && curr_ < buffer_.size(); --n, ++curr_) {
			if (buffer_[curr_] == '\n') {
				line_widths_[line_] = column_;
				++line_;
				column_ = 1;
			} else {
				++column_;
			}
		}
	}

	auto prev(std::size_t n = 1) noexcept -> void {
		for (; n > 0 && curr_ > 0; --n, --curr_) {
			if (buffer_[curr_ - 1] == '\n') {
				--line_;
				column_ = line_widths_[line_];
			} else {
				--column_;
			}
		}
	}

	[[nodiscard]]
	auto curr() const noexcept -> char {
		return curr_ < buffer_.size() ? buffer_[curr_] : end_of_file;
	}

	[[nodiscard]]
	auto get(std::size_t index) const noexcept -> char {
		return index < buffer_.size() ? buffer_[index] : end_of_file;
	}

	[[nodiscard]]
	auto index() const noexcept -> std::size_t {
		return curr_;
	}

	[[nodiscard]]
	auto substr(std::size_t pos, std::size_t n) const noexcept -> std::string_view {
		return buffer_.substr(pos, n);
	}

	[[nodiscard]]
	auto substr(std::size_t n) const noexcept -> std::string_view {
		return substr(curr_, n);
	}

	[[nodiscard]]
	auto extract(std::size_t n) noexcept -> std::string_view {
		auto result = substr(n);
		next(n);
		return result;
	}

	[[nodiscard]]
	auto extract_if(std::string_view sv) noexcept -> std::optional<std::string_view> {
		if (match(sv)) return extract(sv.size());
		return std::nullopt;
	}

	[[nodiscard]]
	auto match(char ch) const noexcept -> bool {
		return curr() == ch;
	}

	[[nodiscard]]
	auto match(std::string_view sv) const noexcept -> bool {
		return substr(sv.size()) == sv;
	}

	template<typename Predicate>
		requires std::predicate<Predicate, char>
	[[nodiscard]]
	auto match(Predicate pred) const noexcept -> bool {
		return pred(curr());
	}

	[[nodiscard]]
	auto match(int (*pred)(int)) const noexcept -> bool {
		return pred(static_cast<unsigned char>(curr())) != 0;
	}

	[[nodiscard]]
	auto line() const noexcept -> std::size_t {
		return line_;
	}

	[[nodiscard]]
	auto column() const noexcept -> std::size_t {
		return column_;
	}

private:
	std::string_view buffer_;
	std::size_t curr_ = 0;
	std::size_t line_ = 1;
	std::size_t column_ = 1;
	std::unordered_map<std::size_t, std::size_t> line_widths_;
};

} // namespace lexer
