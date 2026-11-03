#ifndef LEX_TOKENTYPE_H
#define LEX_TOKENTYPE_H

#include <cstdint>
#include <string_view>

namespace lex
{

enum class TokenType : uint8_t
{
	// Конец входного текста
	EndOfFile,

	// Недопустимая лексема
	Error,

	Identifier,
	IntLiteral,
	DoubleLiteral,
	CharLiteral,

	// Части литерала строки: открывающая кавычка, фрагмент текста, закрывающая кавычка.
	StringBegin,
	StringText,
	StringEnd,

	// Подстановки в строке: "$name" и "${" ... "}"
	TemplateIdentifier,
	TemplateBegin,
	TemplateEnd,

	// Ключевые слова.
	Break,
	Continue,
	Do,
	Else,
	False,
	For,
	Fun,
	If,
	In,
	Return,
	Struct,
	True,
	Val,
	Var,
	While,

	// Ключевые слова - в названия типов.
	IntType,
	DoubleType,
	BooleanType,
	CharType,
	StringType,
	UnitType,
	ArrayType,

	// Операторы.
	Plus,
	Minus,
	Multiply,
	Divide,
	Modulo,
	Equal,
	NotEqual,
	Identical,
	NotIdentical,
	Less,
	Greater,
	LessOrEqual,
	GreaterOrEqual,
	And,
	Or,
	Not,
	Dot,
	Range,
	RangeUntil,
	NotIn,
	Assign,
	PlusAssign,
	MinusAssign,
	MultiplyAssign,
	DivideAssign,
	ModuloAssign,
	Increment,
	Decrement,

	// Скобки.
	LeftParen,
	RightParen,
	LeftBrace,
	RightBrace,
	LeftBracket,
	RightBracket,

	// Разделители
	Comma,
	Colon,
	Semicolon,

	// Перевод строки, разделяющий инструкции
	Newline,
};

std::string_view ToString(TokenType type);

} // namespace lex

#endif // LEX_TOKENTYPE_H
