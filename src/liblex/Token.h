#ifndef LEX_TOKEN_H
#define LEX_TOKEN_H

#include <cstdint>
#include <string>
#include <variant>

#include "SourceLocation.h"
#include "TokenType.h"

namespace lex
{

/*
 * Значение токена:
 * - значение литерала целого или вещественного числа;
 * - кодовая точка Unicode литерала символа;
 * - текст фрагмента строки после замены escape-последовательностей (в UTF-8);
 * - имя идентификатора, в том числе в подстановке "$name";
 * - исходный текст недопустимой лексемы.
 */
using TokenValue = std::variant<std::monostate, int32_t, double, char32_t, std::string>;

// Токен - одна лексема
struct Token
{
	TokenType type = TokenType::EndOfFile;
	TokenValue value;

	// Позиция начала лексемы в исходном тексте
	SourceLocation location;
};

} // namespace lex

#endif // LEX_TOKEN_H
