#include "Parser.h"

#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace parse
{
namespace
{

using lex::TokenType;
constexpr std::size_t MAX_EXPRESSION_DEPTH = 512;

} // namespace

// Инициализирует лексер и читает первый токен.
Parser::Parser(std::string source)
	: m_lexer(std::move(source))
{
	Advance();
}

// Разбирает правило `program = [ separator ] [ top_level_declaration { separator top_level_declaration } [ separator ] ] end_of_file ;`.
// В этой итерации программа состоит из одного объявления функции main.
std::vector<vm::Instruction> Parser::ParseProgram()
{
	ParseSeparator();
	ParseFunctionDeclaration();
	ParseSeparator();
	Expect(TokenType::EndOfFile);
	m_program.emplace_back(vm::op::Halt{});
	return std::move(m_program);
}

// Разбирает правило `function_declaration = "fun" identifier "(" [ parameter { "," parameter } ] ")" [ ":" type ] ( block | "=" expression ) ;`.
// Для main поддерживается пустой список параметров и необязательный тип Unit.
void Parser::ParseFunctionDeclaration()
{
	Match(TokenType::Fun);
	MatchIdentifier("main");
	Match(TokenType::LeftParen);
	Match(TokenType::RightParen);
	if (Accept(TokenType::Colon))
	{
		Match(TokenType::UnitType);
	}
	if (Accept(TokenType::Assign))
	{
		ParseExpression();
		return;
	}
	ParseBlock();
}

// Разбирает правило `block = "{" statements "}" ;`.
void Parser::ParseBlock()
{
	Match(TokenType::LeftBrace);
	ParseStatements();
	Match(TokenType::RightBrace);
}

// Разбирает правило `statements = [ separator ] [ statement { separator statement } [ separator ] ] ;`.
void Parser::ParseStatements()
{
	ParseSeparator();
	while (m_token.type != TokenType::RightBrace)
	{
		ParseStatement();
		if (!ParseSeparator())
		{
			Expect(TokenType::RightBrace, "a separator or RightBrace");
		}
	}
}

// Разбирает правило `statement = property_declaration | control_structure_body ;`.
// Из вариантов control_structure_body поддерживается выражение.
void Parser::ParseStatement()
{
	ParseExpression();
}

// Разбирает правило `expression = disjunction ;`.
// Для арифметических выражений рекурсивный спуск сразу переходит к additive_expression.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParseExpression()
{
	return ParseAdditiveExpression();
}

// Разбирает правило `additive_expression = multiplicative_expression { ( "+" | "-" ) multiplicative_expression } ;`.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParseAdditiveExpression()
{
	const auto type = ParseMultiplicativeExpression();
	while (m_token.type == TokenType::Plus || m_token.type == TokenType::Minus)
	{
		RequireInteger(type);
		const auto operation = m_token.type;
		Advance();
		RequireInteger(ParseMultiplicativeExpression());
		if (operation == TokenType::Plus)
		{
			m_program.emplace_back(vm::op::Add{});
		}
		else
		{
			m_program.emplace_back(vm::op::Subtract{});
		}
	}
	return type;
}

// Разбирает правило `multiplicative_expression = prefix_unary_expression { ( "*" | "/" | "%" ) prefix_unary_expression } ;`.
// Для оператора % сообщает об отсутствии соответствующей инструкции виртуальной машины.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParseMultiplicativeExpression()
{
	const auto type = ParsePrefixUnaryExpression();
	while (true)
	{
		vm::Instruction instruction;
		switch (m_token.type)
		{
		case TokenType::Multiply:
			instruction = vm::op::Multiply{};
			break;
		case TokenType::Divide:
			instruction = vm::op::Divide{};
			break;
		case TokenType::Modulo:
			Fail("Operator '%' is unavailable in the virtual machine");
		default:
			return type;
		}
		RequireInteger(type);
		Advance();
		RequireInteger(ParsePrefixUnaryExpression());
		m_program.push_back(instruction);
	}
}

// Разбирает правило `prefix_unary_expression = { "-" | "+" | "!" } postfix_unary_expression ;`.
// В этой итерации поддерживаются унарные плюс и минус.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParsePrefixUnaryExpression()
{
	if (m_expressionDepth == MAX_EXPRESSION_DEPTH)
	{
		Fail("Expression nesting limit exceeded");
	}
	++m_expressionDepth;

	auto type = ExpressionType::Int;
	if (Accept(TokenType::Plus))
	{
		RequireInteger(ParsePrefixUnaryExpression());
	}
	else if (Accept(TokenType::Minus))
	{
		RequireInteger(ParsePrefixUnaryExpression());
		m_program.emplace_back(vm::op::Negate{});
	}
	else
	{
		type = ParsePostfixUnaryExpression();
	}

	--m_expressionDepth;
	return type;
}

// Разбирает правило `postfix_unary_expression = primary_expression { member_access | indexing_suffix | call_suffix } ;`.
// Из постфиксных операций поддерживается вызов функции read или println по имени.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParsePostfixUnaryExpression()
{
	if (m_token.type != TokenType::Identifier)
	{
		return ParsePrimaryExpression();
	}

	const auto name = std::get<std::string>(m_token.value);
	if (name != "read" && name != "println")
	{
		Fail(std::format("Unknown function '{}'", name));
	}
	Advance();
	return ParseCallSuffix(name);
}

// Разбирает целый литерал и выражение в скобках из правила primary_expression.
// Вариант identifier обрабатывается в ParsePostfixUnaryExpression.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParsePrimaryExpression()
{
	switch (m_token.type)
	{
	case TokenType::IntLiteral:
		m_program.emplace_back(vm::op::Push{ .value = std::get<std::int32_t>(m_token.value) });
		Advance();
		return ExpressionType::Int;
	case TokenType::LeftParen: {
		Advance();
		const auto type = ParseExpression();
		Match(TokenType::RightParen);
		return type;
	}
	default:
		Unexpected("an integer expression");
	}
}

// Разбирает правило `call_suffix = "(" [ expression { "," expression } ] ")" ;`.
// Проверяет число и тип аргументов встроенной функции.
// NOLINTNEXTLINE(misc-no-recursion)
Parser::ExpressionType Parser::ParseCallSuffix(std::string_view name)
{
	Match(TokenType::LeftParen);
	std::size_t argumentCount = 0;
	for (bool hasArgument = m_token.type != TokenType::RightParen; hasArgument; hasArgument = Accept(TokenType::Comma))
	{
		RequireInteger(ParseExpression());
		++argumentCount;
	}

	const bool isPrintln = name == "println";
	const std::size_t expectedCount = isPrintln ? 1 : 0;
	Expect(TokenType::RightParen);
	if (argumentCount != expectedCount)
	{
		Fail(std::format("Expected {} argument for '{}', got {}", expectedCount, name, argumentCount));
	}
	Advance();
	if (isPrintln)
	{
		m_program.emplace_back(vm::op::Println{});
		return ExpressionType::Unit;
	}

	m_program.emplace_back(vm::op::Read{});
	return ExpressionType::Int;
}

// Проверяет, что выражение имеет тип Int.
void Parser::RequireInteger(ExpressionType type) const
{
	if (type != ExpressionType::Int)
	{
		Fail("Expected an Int operand");
	}
}

// Разбирает правило `separator = ( ";" | newline ) { ";" | newline } ;`.
// Возвращает true, если прочитан хотя бы один разделитель.
bool Parser::ParseSeparator()
{
	bool found = false;
	while (m_token.type == TokenType::Semicolon || m_token.type == TokenType::Newline)
	{
		Advance();
		found = true;
	}
	return found;
}

// Читает следующий токен и сообщает о лексической ошибке.
void Parser::Advance()
{
	m_token = m_lexer.ParseToken();
	if (m_token.type == TokenType::Error)
	{
		Fail(std::format("Invalid token {:?}", std::get<std::string>(m_token.value)));
	}
}

// Поглощает текущий токен, если он соответствует указанному типу.
bool Parser::Accept(TokenType type)
{
	if (m_token.type != type)
	{
		return false;
	}
	Advance();
	return true;
}

// Проверяет тип текущего токена и использует заданное описание в диагностике.
void Parser::Expect(TokenType type, std::string_view description) const
{
	if (m_token.type != type)
	{
		Unexpected(description);
	}
}

// Проверяет тип текущего токена и использует имя типа в диагностике.
void Parser::Expect(TokenType type) const
{
	Expect(type, lex::ToString(type));
}

// Проверяет и поглощает токен указанного типа.
void Parser::Match(TokenType type)
{
	Expect(type);
	Advance();
}

// Проверяет и поглощает идентификатор с указанным именем.
void Parser::MatchIdentifier(std::string_view expectedName)
{
	Expect(TokenType::Identifier);
	if (const auto& name = std::get<std::string>(m_token.value); name != expectedName)
	{
		Fail(std::format("Expected '{}', got '{}'", expectedName, name));
	}
	Advance();
}

// Сообщает о неожиданном типе токена.
[[noreturn]] void Parser::Unexpected(std::string_view expected) const
{
	Fail(std::format("Expected {}, got {}", expected, lex::ToString(m_token.type)));
}

// Завершает разбор исключением с координатами текущего токена.
[[noreturn]] void Parser::Fail(std::string_view message) const
{
	throw std::runtime_error(std::format("{}:{}: {}", m_token.location.line, m_token.location.column, message));
}

} // namespace parse
