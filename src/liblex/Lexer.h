#ifndef LEX_LEXER_H
#define LEX_LEXER_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "TextScanner.h"
#include "Token.h"

namespace lex
{

/**
 * Выполняет лексический анализ исходного кода, разделяя его на лексемы, выделяет токены из исходного текста
 */
class Lexer
{
public:
	explicit Lexer(std::string source);

	Token ParseToken();

private:
	enum class Context : uint8_t
	{
		Parentheses,
		HeaderParentheses,
		Brackets,
		Braces,
		String,
		Template,
	};

	Token ParseNextToken();
	void UpdateContexts(const Token& token);

	struct SkipResult
	{
		std::optional<SourceLocation> newlineLocation;
		bool hasUnterminatedComment = false;
	};

	SkipResult SkipWhitespacesAndComments();
	void SkipLineComment();
	bool SkipBlockComment(std::optional<SourceLocation>& newlineLocation);
	void RememberNewline(std::optional<SourceLocation>& newlineLocation) const;
	bool IsNewlineSignificant() const;
	bool IsContinuationAhead() const;

	Token ParseIdentifierOrKeyword();
	Token ParseNumberLiteral();
	Token ParseIntLiteral(std::string_view digits, int base);
	Token ParseDoubleLiteral(std::string_view text);
	bool ReadFraction(std::string& text);
	bool ReadExponent(std::string& text);
	void ReadDigits(std::string& digits, bool (*isDigit)(char));
	Token ParseStringBegin();
	Token ParseStringPart();
	Token ParseStringText();
	Token ParseTemplateIdentifier();
	bool IsTemplateAhead() const;
	Token ParseCharLiteral();
	std::optional<char32_t> ParseEscapeSequence();
	std::optional<char32_t> ParseUnicodeEscape();
	std::optional<char32_t> ReadUtf8Char();
	Token ParseOperator();
	Token ParseInvalidCharacter();
	bool IsAhead(std::string_view text) const;
	void AdvanceBy(size_t count);

	Token MakeToken(TokenType type, TokenValue value = {}) const;
	Token MakeErrorToken() const;
	Token SkipInvalidLiteralTail();

	TextScanner m_scanner;
	SourceLocation m_tokenLocation;
	std::vector<Context> m_contexts;

	TokenType m_previousType = TokenType::Newline;
	bool m_previousClosesHeader = false;
};

} // namespace lex

#endif // LEX_LEXER_H
