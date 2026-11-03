#include "TextScanner.h"

#include <utility>

#include "CharClass.h"

namespace lex
{

TextScanner::TextScanner(std::string source)
	: m_source(std::move(source))
{
}

char TextScanner::Peek(size_t offset) const
{
	const size_t position = m_position + offset;
	return position < m_source.size() ? m_source[position] : '\0';
}

bool TextScanner::IsEnd() const
{
	return m_position >= m_source.size();
}

void TextScanner::Advance()
{
	if (IsEnd())
	{
		return;
	}

	const char ch = m_source[m_position];
	++m_position;

	/*
	 * Переносы строк "\n", "\r\n" и "\r" увеличивают номер строки,
	 * байты продолжения символа UTF-8 не меняют номер колонки.
	 */
	if (ch == '\n' || (ch == '\r' && Peek() != '\n'))
	{
		++m_location.line;
		m_location.column = 1;
	}
	else if (!IsUtf8ContinuationByte(Peek()))
	{
		++m_location.column;
	}
}

SourceLocation TextScanner::GetLocation() const
{
	return m_location;
}

void TextScanner::StartCapture()
{
	m_capturePosition = m_position;
}

std::string_view TextScanner::GetCaptured() const
{
	return std::string_view(m_source).substr(m_capturePosition, m_position - m_capturePosition);
}

} // namespace lex
