import std;
import lexer;

enum class token_type : std::uint8_t {
	assignment,
	plus,
	minus,
	star,
	slash,
	identifier,
	number,
	keyword,
	comment,
	eof,
};

auto token_name(token_type token) -> std::string {
	switch (token) {
		using enum token_type;
	case assignment: return "assignment";
	case plus: return "plus";
	case minus: return "minus";
	case star: return "star";
	case slash: return "slash";
	case identifier: return "identifier";
	case number: return "number";
	case keyword: return "keyword";
	case comment: return "comment";
	case eof: return "eof";
	default: std::unreachable();
	}
}

constexpr auto input = R"(
# this is a comment
let x = 1
let y = 2
x + y
)";

const auto comment_token = lexer::token_definition<token_type>{
	[](auto& ctx) { return ctx.curr() == '#'; },
	[](auto& ctx) -> lexer::token_result<token_type> {
		auto lexeme = std::string{};
		while (ctx.curr() != '\n' && ctx.curr() != lexer::end_of_file) {
			lexeme += ctx.curr();
			ctx.next();
		}
		return lexer::token{token_type::comment, lexeme};
	},
};

auto main() -> int {
	auto lexer = lexer::lexer<token_type>(input);

	lexer.define_token(lexer::definitions::skip_whitespace<token_type>);
	lexer.define_token(lexer::definitions::end_of_file<token_type::eof>);
	lexer.define_token(lexer::definitions::single_char<token_type::assignment, '='>);
	lexer.define_token(lexer::definitions::single_char<token_type::plus, '+'>);
	lexer.define_token(lexer::definitions::single_char<token_type::minus, '-'>);
	lexer.define_token(lexer::definitions::single_char<token_type::star, '*'>);
	lexer.define_token(lexer::definitions::single_char<token_type::slash, '/'>);
	lexer.define_token(lexer::definitions::multi_char<token_type::keyword, 'l', 'e', 't'>);
	lexer.define_token(lexer::definitions::identifier<token_type::identifier>);
	lexer.define_token(lexer::definitions::number<token_type::number>);
	lexer.define_token(comment_token);

	while (true) {
		const auto token = lexer.next_token();
		if (!token) {
			const auto& error = token.error();
			std::println(std::cerr, "{}:{}: {}: '{}'", error.line, error.column, error.message, error.ch);
			return 1;
		}

		std::println("{}:{}:{}: '{}'", token_name(token->type), token->start_line, token->start_column, token->lexeme);
		if (token->type == token_type::eof) break;
	}
}
