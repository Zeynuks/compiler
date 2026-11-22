#ifndef PARSE_PARSER_H
#define PARSE_PARSER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Instruction.h"
#include "Lexer.h"
#include "Token.h"

namespace parse
{

// Разбирает исходный текст и формирует инструкции виртуальной машины.
class Parser
{
public:
	explicit Parser(std::string source);

	// При первой лексической или синтаксической ошибке бросает std::runtime_error.
	std::vector<vm::Instruction> ParseProgram();

private:
	enum class ExpressionType : std::uint8_t
	{
		Int,
		Unit,
	};

	void ParseFunctionDeclaration();
	void ParseBlock();
	void ParseStatements();
	void ParseStatement();
	ExpressionType ParseExpression();
	ExpressionType ParseAdditiveExpression();
	ExpressionType ParseMultiplicativeExpression();
	ExpressionType ParsePrefixUnaryExpression();
	ExpressionType ParsePostfixUnaryExpression();
	ExpressionType ParsePrimaryExpression();
	ExpressionType ParseCallSuffix(std::string_view name);
	bool ParseSeparator();

	void RequireInteger(ExpressionType type) const;
	void Advance();
	bool Accept(lex::TokenType type);
	void Expect(lex::TokenType type, std::string_view description) const;
	void Expect(lex::TokenType type) const;
	void Match(lex::TokenType type);
	void MatchIdentifier(std::string_view expectedName);
	[[noreturn]] void Unexpected(std::string_view expected) const;
	[[noreturn]] void Fail(std::string_view message) const;

	lex::Lexer m_lexer;
	lex::Token m_token;
	std::vector<vm::Instruction> m_program;
	std::size_t m_expressionDepth = 0;
};

} // namespace parse

#endif // PARSE_PARSER_H
