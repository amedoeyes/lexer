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
	[](const auto& ctx) { return ctx.match('#'); },
	[](auto& ctx) -> lexer::token_result<token_type> {
		const auto start = ctx.index();
		while (!ctx.match('\n') && !ctx.match(lexer::end_of_file)) ctx.next();
		return lexer::token{token_type::comment, ctx.substr(start, ctx.index() - start)};
	},
};

auto main() -> int {
	auto lexer = lexer::lexer<token_type>(input);

	lexer.define(lexer::definitions::skip_whitespace<token_type>);
	lexer.define(lexer::definitions::end_of_file<token_type::eof>);
	lexer.define(lexer::definitions::single_char<token_type::assignment, '='>);
	lexer.define(lexer::definitions::single_char<token_type::plus, '+'>);
	lexer.define(lexer::definitions::single_char<token_type::minus, '-'>);
	lexer.define(lexer::definitions::single_char<token_type::star, '*'>);
	lexer.define(lexer::definitions::single_char<token_type::slash, '/'>);
	lexer.define(lexer::definitions::multi_char<token_type::keyword, 'l', 'e', 't'>);
	lexer.define(lexer::definitions::identifier<token_type::identifier>);
	lexer.define(lexer::definitions::number<token_type::number>);
	lexer.define(comment_token);

	while (true) {
		const auto token = lexer.next();
		if (!token) {
			const auto& error = token.error();
			std::println(std::cerr, "{}:{}: {}: '{}'", error.line, error.column, error.message, error.ch);
			return 1;
		}

		std::println("{}:{}:{}: '{}'", token_name(token->type), token->start_line, token->start_column, token->lexeme);
		if (token->type == token_type::eof) break;
	}
}
