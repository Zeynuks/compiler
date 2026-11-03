#include "Lexer.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "CharClass.h"

namespace lex
{

namespace
{

/*
 * Ключевые слова чувствительны к регистру
 */
struct Keyword
{
	std::string_view word;
	TokenType type;
};

constexpr std::array KEYWORDS{
	Keyword{ "break", TokenType::Break },
	Keyword{ "continue", TokenType::Continue },
	Keyword{ "do", TokenType::Do },
	Keyword{ "else", TokenType::Else },
	Keyword{ "false", TokenType::False },
	Keyword{ "for", TokenType::For },
	Keyword{ "fun", TokenType::Fun },
	Keyword{ "if", TokenType::If },
	Keyword{ "in", TokenType::In },
	Keyword{ "return", TokenType::Return },
	Keyword{ "struct", TokenType::Struct },
	Keyword{ "true", TokenType::True },
	Keyword{ "val", TokenType::Val },
	Keyword{ "var", TokenType::Var },
	Keyword{ "while", TokenType::While },
	Keyword{ "Int", TokenType::IntType },
	Keyword{ "Double", TokenType::DoubleType },
	Keyword{ "Boolean", TokenType::BooleanType },
	Keyword{ "Char", TokenType::CharType },
	Keyword{ "String", TokenType::StringType },
	Keyword{ "Unit", TokenType::UnitType },
	Keyword{ "Array", TokenType::ArrayType },
};

// Зарезервированные слова, нельзя использовать как идентификаторы.
constexpr std::array<std::string_view, 17> RESERVED_WORDS{
	"as",
	"catch",
	"class",
	"finally",
	"import",
	"interface",
	"is",
	"null",
	"object",
	"package",
	"super",
	"this",
	"throw",
	"try",
	"typealias",
	"typeof",
	"when",
};

struct Operator
{
	std::string_view text;
	TokenType type;
};

/**
 * Операторы, скобки и разделители.Более длинные лексемы идут раньше своих префиксов,
 * чтобы при поиске первого совпадения выбиралась самая длинная лексема.
 */
constexpr std::array OPERATORS{
	Operator{ "===", TokenType::Identical },
	Operator{ "!==", TokenType::NotIdentical },
	Operator{ "..<", TokenType::RangeUntil },
	Operator{ "==", TokenType::Equal },
	Operator{ "!=", TokenType::NotEqual },
	Operator{ "<=", TokenType::LessOrEqual },
	Operator{ ">=", TokenType::GreaterOrEqual },
	Operator{ "&&", TokenType::And },
	Operator{ "||", TokenType::Or },
	Operator{ "..", TokenType::Range },
	Operator{ "+=", TokenType::PlusAssign },
	Operator{ "-=", TokenType::MinusAssign },
	Operator{ "*=", TokenType::MultiplyAssign },
	Operator{ "/=", TokenType::DivideAssign },
	Operator{ "%=", TokenType::ModuloAssign },
	Operator{ "++", TokenType::Increment },
	Operator{ "--", TokenType::Decrement },
	Operator{ "+", TokenType::Plus },
	Operator{ "-", TokenType::Minus },
	Operator{ "*", TokenType::Multiply },
	Operator{ "/", TokenType::Divide },
	Operator{ "%", TokenType::Modulo },
	Operator{ "<", TokenType::Less },
	Operator{ ">", TokenType::Greater },
	Operator{ "!", TokenType::Not },
	Operator{ ".", TokenType::Dot },
	Operator{ "=", TokenType::Assign },
	Operator{ "(", TokenType::LeftParen },
	Operator{ ")", TokenType::RightParen },
	Operator{ "{", TokenType::LeftBrace },
	Operator{ "}", TokenType::RightBrace },
	Operator{ "[", TokenType::LeftBracket },
	Operator{ "]", TokenType::RightBracket },
	Operator{ ",", TokenType::Comma },
	Operator{ ":", TokenType::Colon },
	Operator{ ";", TokenType::Semicolon },
};

struct SimpleEscape
{
	char name;
	char32_t value;
};

constexpr std::array SIMPLE_ESCAPES{
	SimpleEscape{ 'n', '\n' },
	SimpleEscape{ 't', '\t' },
	SimpleEscape{ 'r', '\r' },
	SimpleEscape{ 'b', '\b' },
	SimpleEscape{ '"', '"' },
	SimpleEscape{ '\'', '\'' },
	SimpleEscape{ '\\', '\\' },
	SimpleEscape{ '$', '$' },
};

constexpr char32_t MAX_CODE_POINT = 0x10FFFF;
constexpr std::array MIN_CODE_POINT{ char32_t{ 0 }, char32_t{ 0x80 }, char32_t{ 0x800 }, char32_t{ 0x10000 } };

constexpr char32_t MIN_SURROGATE = 0xD800;
constexpr char32_t MAX_SURROGATE = 0xDFFF;

// Добавляет к строке кодовую точку Unicode в кодировке UTF-8.
void AppendUtf8(std::string& text, char32_t codePoint)
{
	const auto appendByte = [&text](char32_t byte) {
		text.push_back(static_cast<char>(byte));
	};

	if (codePoint < 0x80U)
	{
		appendByte(codePoint);
	}
	else if (codePoint < 0x800U)
	{
		appendByte(0xC0U | (codePoint >> 6U));
		appendByte(0x80U | (codePoint & 0x3FU));
	}
	else if (codePoint < 0x10000U)
	{
		appendByte(0xE0U | (codePoint >> 12U));
		appendByte(0x80U | ((codePoint >> 6U) & 0x3FU));
		appendByte(0x80U | (codePoint & 0x3FU));
	}
	else
	{
		appendByte(0xF0U | (codePoint >> 18U));
		appendByte(0x80U | ((codePoint >> 12U) & 0x3FU));
		appendByte(0x80U | ((codePoint >> 6U) & 0x3FU));
		appendByte(0x80U | (codePoint & 0x3FU));
	}
}

// Идентификатор из одних подчёркиваний недопустим.
bool IsUnderscoresOnly(std::string_view word)
{
	return std::ranges::all_of(word, [](char ch) { return ch == '_'; });
}

// Ключевые слова, после которых в круглых скобках идёт заголовок инструкции.
bool IsHeaderKeyword(TokenType type)
{
	return type == TokenType::If || type == TokenType::While || type == TokenType::For;
}

// Проверяет, продолжается ли инструкция после токена на следующей строке.
bool ContinuesOnNextLine(TokenType type)
{
	switch (type)
	{
	case TokenType::Plus:
	case TokenType::Minus:
	case TokenType::Multiply:
	case TokenType::Divide:
	case TokenType::Modulo:
	case TokenType::Equal:
	case TokenType::NotEqual:
	case TokenType::Identical:
	case TokenType::NotIdentical:
	case TokenType::Less:
	case TokenType::Greater:
	case TokenType::LessOrEqual:
	case TokenType::GreaterOrEqual:
	case TokenType::And:
	case TokenType::Or:
	case TokenType::Range:
	case TokenType::RangeUntil:
	case TokenType::In:
	case TokenType::NotIn:
	case TokenType::Assign:
	case TokenType::PlusAssign:
	case TokenType::MinusAssign:
	case TokenType::MultiplyAssign:
	case TokenType::DivideAssign:
	case TokenType::ModuloAssign:
	case TokenType::Dot:
	case TokenType::Comma:
	case TokenType::Colon:
	case TokenType::LeftParen:
	case TokenType::LeftBracket:
	case TokenType::LeftBrace:
	case TokenType::Else:
	case TokenType::Do:
	case TokenType::Semicolon:
	case TokenType::Newline:
		return true;
	default:
		return false;
	}
}

std::optional<TokenType> FindKeyword(std::string_view word)
{
	for (const auto& keyword : KEYWORDS)
	{
		if (keyword.word == word)
		{
			return keyword.type;
		}
	}
	return std::nullopt;
}

} // namespace

Lexer::Lexer(std::string source)
	: m_scanner(std::move(source))
{
}

Token Lexer::ParseToken()
{
	if (!m_contexts.empty() && m_contexts.back() == Context::String)
	{
		Token token = ParseStringPart();
		m_previousType = token.type;
		return token;
	}

	const auto [newlineLocation, hasUnterminatedComment] = SkipWhitespacesAndComments();
	if (hasUnterminatedComment)
	{
		return MakeErrorToken();
	}

	if (newlineLocation && std::ranges::contains(m_contexts, Context::Template))
	{
		AbandonStrings();
		m_previousType = TokenType::Error;
		m_previousSuppressesNewline = false;
		return Token{ .type = TokenType::Error, .value = std::string{}, .location = *newlineLocation };
	}

	if (newlineLocation && IsNewlineSignificant())
	{
		m_previousType = TokenType::Newline;
		m_previousSuppressesNewline = false;
		return Token{ .type = TokenType::Newline, .value = {}, .location = *newlineLocation };
	}

	Token token = ParseNextToken();
	UpdateContexts(token);
	m_previousType = token.type;
	return token;
}

Token Lexer::ParseNextToken()
{
	m_scanner.StartCapture();
	m_tokenLocation = m_scanner.GetLocation();

	if (m_scanner.IsEnd())
	{
		if (std::ranges::contains(m_contexts, Context::String))
		{
			return MakeUnterminatedStringError();
		}

		return MakeToken(TokenType::EndOfFile);
	}

	const char ch = m_scanner.Peek();
	if (IsIdentifierStart(ch))
	{
		return ParseIdentifierOrKeyword();
	}
	if (IsDecimalDigit(ch))
	{
		return ParseNumberLiteral();
	}
	if (ch == '\'')
	{
		return ParseCharLiteral();
	}
	if (ch == '"')
	{
		return ParseStringBegin();
	}
	if (ch == '}' && !m_contexts.empty() && m_contexts.back() == Context::Template)
	{
		m_scanner.Advance();
		m_contexts.pop_back();
		return MakeToken(TokenType::TemplateEnd);
	}

	return ParseOperator();
}

void Lexer::UpdateContexts(const Token& token)
{
	const auto closeContext = [this](Context expected) {
		const bool isMatched = !m_contexts.empty() && m_contexts.back() == expected;
		if (isMatched)
		{
			m_contexts.pop_back();
		}
		return isMatched;
	};

	m_previousSuppressesNewline = false;
	switch (token.type)
	{
	case TokenType::LeftParen:
		m_contexts.push_back(IsHeaderKeyword(m_previousType) ? Context::HeaderParentheses : Context::Parentheses);
		break;
	case TokenType::LeftBracket:
		m_contexts.push_back(Context::Brackets);
		break;
	case TokenType::LeftBrace:
		m_contexts.push_back(m_previousType == TokenType::Do ? Context::DoBraces : Context::Braces);
		break;
	case TokenType::RightParen:
		m_previousSuppressesNewline = closeContext(Context::HeaderParentheses);
		if (!m_previousSuppressesNewline)
		{
			closeContext(Context::Parentheses);
		}
		break;
	case TokenType::RightBracket:
		closeContext(Context::Brackets);
		break;
	case TokenType::RightBrace:
		m_previousSuppressesNewline = closeContext(Context::DoBraces);
		if (!m_previousSuppressesNewline)
		{
			closeContext(Context::Braces);
		}
		break;
	default:
		break;
	}
}

Lexer::SkipResult Lexer::SkipWhitespacesAndComments()
{
	SkipResult result;
	while (true)
	{
		if (IsWhitespace(m_scanner.Peek()))
		{
			RememberNewline(result.newlineLocation);
			m_scanner.Advance();
		}
		else if (IsAhead("//"))
		{
			SkipLineComment();
		}
		else if (IsAhead("/*"))
		{
			if (!SkipBlockComment(result.newlineLocation))
			{
				result.hasUnterminatedComment = true;
				return result;
			}
		}
		else
		{
			return result;
		}
	}
}

void Lexer::SkipLineComment()
{
	while (!m_scanner.IsEnd() && !IsNewline(m_scanner.Peek()))
	{
		m_scanner.Advance();
	}
}

bool Lexer::SkipBlockComment(std::optional<SourceLocation>& newlineLocation)
{
	m_scanner.StartCapture();
	m_tokenLocation = m_scanner.GetLocation();

	AdvanceBy(2);
	size_t depth = 1;
	while (depth > 0)
	{
		if (m_scanner.IsEnd())
		{
			return false;
		}
		if (IsAhead("/*"))
		{
			++depth;
			AdvanceBy(2);
		}
		else if (IsAhead("*/"))
		{
			--depth;
			AdvanceBy(2);
		}
		else
		{
			RememberNewline(newlineLocation);
			m_scanner.Advance();
		}
	}

	return true;
}

void Lexer::RememberNewline(std::optional<SourceLocation>& newlineLocation) const
{
	if (!newlineLocation && IsNewline(m_scanner.Peek()))
	{
		newlineLocation = m_scanner.GetLocation();
	}
}

bool Lexer::IsNewlineSignificant() const
{
	if (!m_contexts.empty() && m_contexts.back() != Context::Braces && m_contexts.back() != Context::DoBraces)
	{
		return false;
	}

	if (m_previousSuppressesNewline || ContinuesOnNextLine(m_previousType))
	{
		return false;
	}

	return !m_scanner.IsEnd() && !IsAhead(";") && !IsContinuationAhead();
}

bool Lexer::IsContinuationAhead() const
{
	if (IsAhead(".."))
	{
		return false;
	}
	return IsAhead(".") || IsAhead("&&") || IsAhead("||") || (IsAhead("else") && !IsIdentifierPart(m_scanner.Peek(4)));
}

Token Lexer::ParseIdentifierOrKeyword()
{
	while (IsIdentifierPart(m_scanner.Peek()))
	{
		m_scanner.Advance();
	}

	const std::string_view word = m_scanner.GetCaptured();

	if (const auto keywordType = FindKeyword(word))
	{
		return MakeToken(*keywordType);
	}

	if (std::ranges::contains(RESERVED_WORDS, word) || IsUnderscoresOnly(word))
	{
		return MakeErrorToken();
	}

	return MakeToken(TokenType::Identifier, std::string(word));
}

Token Lexer::ParseNumberLiteral()
{
	std::string digits;
	if (const char prefix = m_scanner.Peek(1); m_scanner.Peek() == '0' && (prefix == 'x' || prefix == 'X' || prefix == 'b' || prefix == 'B'))
	{
		const bool isHex = prefix == 'x' || prefix == 'X';
		AdvanceBy(2);
		ReadDigits(digits, isHex ? IsHexDigit : IsBinaryDigit);
		return ParseIntLiteral(digits, isHex ? 16 : 2);
	}

	ReadDigits(digits, IsDecimalDigit);

	const bool hasLeadingZero = digits.size() > 1 && digits.front() == '0';
	const bool hasUnderscore = m_scanner.GetCaptured().contains('_');

	const bool hasFraction = ReadFraction(digits);
	const bool hasExponent = ReadExponent(digits);
	if (hasLeadingZero || (hasUnderscore && (hasFraction || hasExponent)))
	{
		return SkipInvalidLiteralTail();
	}
	if (hasFraction || hasExponent)
	{
		return ParseDoubleLiteral(digits);
	}
	return ParseIntLiteral(digits, 10);
}

Token Lexer::ParseIntLiteral(std::string_view digits, int base)
{
	if (digits.empty() || IsIdentifierPart(m_scanner.Peek()))
	{
		return SkipInvalidLiteralTail();
	}

	int32_t value = 0;
	const auto result = std::from_chars(std::to_address(digits.begin()), std::to_address(digits.end()), value, base);
	if (result.ec != std::errc{})
	{
		return MakeErrorToken();
	}
	return MakeToken(TokenType::IntLiteral, value);
}

Token Lexer::ParseDoubleLiteral(std::string_view text)
{
	if (IsIdentifierPart(m_scanner.Peek()))
	{
		return SkipInvalidLiteralTail();
	}

	double value = 0;
	const auto [ptr, ec] = std::from_chars(std::to_address(text.begin()), std::to_address(text.end()), value);
	if (ec == std::errc::result_out_of_range)
	{
		value = std::strtod(std::string(text).c_str(), nullptr);
		if (std::isinf(value))
		{
			return MakeErrorToken();
		}
	}
	else if (ec != std::errc{})
	{
		return MakeErrorToken();
	}
	return MakeToken(TokenType::DoubleLiteral, value);
}

bool Lexer::ReadFraction(std::string& text)
{
	if (m_scanner.Peek() != '.' || !IsDecimalDigit(m_scanner.Peek(1)))
	{
		return false;
	}

	text.push_back('.');
	m_scanner.Advance();
	ReadDigits(text, IsDecimalDigit, false);
	return true;
}

bool Lexer::ReadExponent(std::string& text)
{
	const char exponentChar = m_scanner.Peek();
	if (exponentChar != 'e' && exponentChar != 'E')
	{
		return false;
	}

	const char signChar = m_scanner.Peek(1);
	const bool hasSign = signChar == '+' || signChar == '-';
	if (!IsDecimalDigit(m_scanner.Peek(hasSign ? 2 : 1)))
	{
		return false;
	}

	text.push_back('e');
	m_scanner.Advance();
	if (hasSign)
	{
		text.push_back(signChar);
		m_scanner.Advance();
	}
	ReadDigits(text, IsDecimalDigit, false);
	return true;
}

void Lexer::ReadDigits(std::string& digits, bool (*isDigit)(char), bool allowUnderscores)
{
	while (true)
	{
		const char ch = m_scanner.Peek();
		if (isDigit(ch))
		{
			digits.push_back(ch);
			m_scanner.Advance();
			continue;
		}

		if (allowUnderscores && ch == '_' && !digits.empty())
		{
			size_t offset = 1;
			while (m_scanner.Peek(offset) == '_')
			{
				++offset;
			}
			if (isDigit(m_scanner.Peek(offset)))
			{
				AdvanceBy(offset);
				continue;
			}
		}
		return;
	}
}

Token Lexer::ParseStringBegin()
{
	m_scanner.Advance();
	m_contexts.push_back(Context::String);
	return MakeToken(TokenType::StringBegin);
}

void Lexer::AbandonStrings()
{
	const auto firstString = std::ranges::find(m_contexts, Context::String);
	m_contexts.erase(firstString, m_contexts.end());
}

Token Lexer::MakeUnterminatedStringError()
{
	AbandonStrings();
	return MakeErrorToken();
}

Token Lexer::ParseStringPart()
{
	m_scanner.StartCapture();
	m_tokenLocation = m_scanner.GetLocation();

	if (m_scanner.IsEnd() || IsNewline(m_scanner.Peek()))
	{
		return MakeUnterminatedStringError();
	}

	if (m_scanner.Peek() == '"')
	{
		m_scanner.Advance();
		m_contexts.pop_back();
		return MakeToken(TokenType::StringEnd);
	}

	if (IsAhead("${"))
	{
		AdvanceBy(2);
		m_contexts.push_back(Context::Template);
		return MakeToken(TokenType::TemplateBegin);
	}
	if (IsTemplateAhead())
	{
		return ParseTemplateIdentifier();
	}

	return ParseStringText();
}

Token Lexer::ParseTemplateIdentifier()
{
	m_scanner.Advance();
	std::string name;
	while (IsIdentifierPart(m_scanner.Peek()))
	{
		name.push_back(m_scanner.Peek());
		m_scanner.Advance();
	}

	if (IsUnderscoresOnly(name))
	{
		return MakeErrorToken();
	}
	return MakeToken(TokenType::TemplateIdentifier, std::move(name));
}

bool Lexer::IsTemplateAhead() const
{
	return m_scanner.Peek() == '$' && (m_scanner.Peek(1) == '{' || IsIdentifierStart(m_scanner.Peek(1)));
}

Token Lexer::ParseStringText()
{
	std::string text;
	bool hasInvalidEscape = false;
	while (!m_scanner.IsEnd() && !IsNewline(m_scanner.Peek()) && m_scanner.Peek() != '"' && !IsTemplateAhead())
	{
		if (m_scanner.Peek() == '\\')
		{
			if (const auto codePoint = ParseEscapeSequence())
			{
				AppendUtf8(text, *codePoint);
			}
			else
			{
				hasInvalidEscape = true;
			}
		}
		else
		{
			text.push_back(m_scanner.Peek());
			m_scanner.Advance();
		}
	}

	if (hasInvalidEscape)
	{
		return MakeErrorToken();
	}
	return MakeToken(TokenType::StringText, std::move(text));
}

Token Lexer::ParseCharLiteral()
{
	m_scanner.Advance();

	std::optional<char32_t> value;
	const char ch = m_scanner.Peek();
	if (ch == '\\')
	{
		value = ParseEscapeSequence();
	}
	else if (ch != '\'' && !IsNewline(ch) && !m_scanner.IsEnd())
	{
		value = ReadUtf8Char();
	}

	if (value && m_scanner.Peek() == '\'')
	{
		m_scanner.Advance();
		return MakeToken(TokenType::CharLiteral, *value);
	}

	while (!m_scanner.IsEnd() && !IsNewline(m_scanner.Peek()) && m_scanner.Peek() != '\'')
	{
		m_scanner.Advance();
	}
	if (m_scanner.Peek() == '\'')
	{
		m_scanner.Advance();
	}
	return MakeErrorToken();
}

std::optional<char32_t> Lexer::ParseEscapeSequence()
{
	m_scanner.Advance();

	const char name = m_scanner.Peek();
	for (const auto& escape : SIMPLE_ESCAPES)
	{
		if (escape.name == name)
		{
			m_scanner.Advance();
			return escape.value;
		}
	}

	if (name == 'u')
	{
		m_scanner.Advance();
		return ParseUnicodeEscape();
	}
	return std::nullopt;
}

std::optional<char32_t> Lexer::ParseUnicodeEscape()
{
	const bool hasBraces = m_scanner.Peek() == '{';
	const size_t maxDigitCount = hasBraces ? 6 : 4;
	if (hasBraces)
	{
		m_scanner.Advance();
	}

	char32_t codePoint = 0;
	size_t digitCount = 0;
	while (digitCount < maxDigitCount && IsHexDigit(m_scanner.Peek()))
	{
		codePoint = (codePoint * 16) + HexDigitValue(m_scanner.Peek());
		++digitCount;
		m_scanner.Advance();
	}

	const bool hasValidDigits = hasBraces ? digitCount > 0 : digitCount == maxDigitCount;
	if (!hasValidDigits || (hasBraces && m_scanner.Peek() != '}'))
	{
		return std::nullopt;
	}
	if (hasBraces)
	{
		m_scanner.Advance();
	}

	if (codePoint > MAX_CODE_POINT || (codePoint >= MIN_SURROGATE && codePoint <= MAX_SURROGATE))
	{
		return std::nullopt;
	}
	return codePoint;
}

std::optional<char32_t> Lexer::ReadUtf8Char()
{
	// Первый байт символа UTF-8 задаёт число байт продолжения и старшие биты кодовой точки.
	const auto leadByte = static_cast<unsigned char>(m_scanner.Peek());
	m_scanner.Advance();

	size_t continuationCount = 0;
	char32_t codePoint = 0;
	if (leadByte < 0x80U)
	{
		codePoint = leadByte;
	}
	else if ((leadByte & 0xE0U) == 0xC0U)
	{
		continuationCount = 1;
		codePoint = leadByte & 0x1FU;
	}
	else if ((leadByte & 0xF0U) == 0xE0U)
	{
		continuationCount = 2;
		codePoint = leadByte & 0x0FU;
	}
	else if ((leadByte & 0xF8U) == 0xF0U)
	{
		continuationCount = 3;
		codePoint = leadByte & 0x07U;
	}
	else
	{
		return std::nullopt;
	}

	for (size_t i = 0; i < continuationCount; ++i)
	{
		const char ch = m_scanner.Peek();
		if (!IsUtf8ContinuationByte(ch))
		{
			return std::nullopt;
		}
		codePoint = (codePoint << 6U) | (static_cast<unsigned char>(ch) & 0x3FU);
		m_scanner.Advance();
	}

	if (codePoint < MIN_CODE_POINT.at(continuationCount) || codePoint > MAX_CODE_POINT
		|| (codePoint >= MIN_SURROGATE && codePoint <= MAX_SURROGATE))
	{
		return std::nullopt;
	}
	return codePoint;
}

Token Lexer::ParseOperator()
{
	// "!in" — один оператор, только если "in" не продолжается идентификатором.
	if (IsAhead("!in") && !IsIdentifierPart(m_scanner.Peek(3)))
	{
		AdvanceBy(3);
		return MakeToken(TokenType::NotIn);
	}

	for (const auto& op : OPERATORS)
	{
		if (IsAhead(op.text))
		{
			AdvanceBy(op.text.size());
			return MakeToken(op.type);
		}
	}

	return ParseInvalidCharacter();
}

Token Lexer::ParseInvalidCharacter()
{
	m_scanner.Advance();
	while (IsUtf8ContinuationByte(m_scanner.Peek()))
	{
		m_scanner.Advance();
	}
	return MakeErrorToken();
}

bool Lexer::IsAhead(std::string_view text) const
{
	for (size_t i = 0; i < text.size(); ++i)
	{
		if (m_scanner.Peek(i) != text[i])
		{
			return false;
		}
	}
	return true;
}

void Lexer::AdvanceBy(size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		m_scanner.Advance();
	}
}

Token Lexer::MakeToken(TokenType type, TokenValue value) const
{
	return Token{ .type = type, .value = std::move(value), .location = m_tokenLocation };
}

Token Lexer::SkipInvalidLiteralTail()
{
	while (IsIdentifierPart(m_scanner.Peek()))
	{
		m_scanner.Advance();
	}
	return MakeErrorToken();
}

Token Lexer::MakeErrorToken() const
{
	return MakeToken(TokenType::Error, std::string(m_scanner.GetCaptured()));
}

} // namespace lex
