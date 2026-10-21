#ifndef LEX_CHARCLASS_H
#define LEX_CHARCLASS_H

namespace lex
{

inline bool IsLetter(char ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

inline bool IsDecimalDigit(char ch)
{
	return ch >= '0' && ch <= '9';
}

inline bool IsHexDigit(char ch)
{
	return IsDecimalDigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

// Возвращает значение шестнадцатеричной цифры, проверенной функцией IsHexDigit.
inline char32_t HexDigitValue(char ch)
{
	if (IsDecimalDigit(ch))
	{
		return static_cast<char32_t>(ch - '0');
	}
	if (ch >= 'a' && ch <= 'f')
	{
		return static_cast<char32_t>(ch - 'a' + 10);
	}
	return static_cast<char32_t>(ch - 'A' + 10);
}

inline bool IsBinaryDigit(char ch)
{
	return ch == '0' || ch == '1';
}

inline bool IsIdentifierStart(char ch)
{
	return IsLetter(ch) || ch == '_';
}

inline bool IsIdentifierPart(char ch)
{
	return IsIdentifierStart(ch) || IsDecimalDigit(ch);
}

inline bool IsWhitespace(char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

inline bool IsNewline(char ch)
{
	return ch == '\r' || ch == '\n';
}

inline bool IsUtf8ContinuationByte(char ch)
{
	return (static_cast<unsigned char>(ch) & 0xC0U) == 0x80U;
}

} // namespace lex

#endif // LEX_CHARCLASS_H
