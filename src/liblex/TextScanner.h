#ifndef LEX_TEXTSCANNER_H
#define LEX_TEXTSCANNER_H

#include <cstddef>
#include <string>
#include <string_view>

#include "SourceLocation.h"

namespace lex
{

/*  Посимвольно читает исходный текст в памяти,
 * позволяет заглядывать вперёд на любое число байт, но не возвращаться назад.
 */
class TextScanner
{
public:
	explicit TextScanner(std::string source);

	// Отдает байт на расстоянии offset от ткурсора либо '\0' за концом текста
	[[nodiscard]] char Peek(size_t offset = 0) const;

	[[nodiscard]] bool IsEnd() const;

	// Сдвигает курсор на один байт вперёд
	void Advance();

	[[nodiscard]] SourceLocation GetLocation() const;

	// Запоминает текущую позицию как начало захватываемого текста.
	void StartCapture();

	// Возвращает текст от начала захвата до текущей позиции.
	[[nodiscard]] std::string_view GetCaptured() const;

private:
	std::string m_source;
	size_t m_position = 0;
	size_t m_capturePosition = 0;
	SourceLocation m_location;
};

} // namespace lex

#endif // LEX_TEXTSCANNER_H
