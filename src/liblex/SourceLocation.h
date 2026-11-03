#ifndef LEX_SOURCELOCATION_H
#define LEX_SOURCELOCATION_H

#include <cstddef>

namespace lex
{

// Позиция символа в исходном файле
struct SourceLocation
{
	// Номер строки, начинается с 1
	size_t line = 1;

	// Номер колонки в символах unicode начинается с 1
	size_t column = 1;

	bool operator==(const SourceLocation&) const = default;
};

} // namespace lex

#endif // LEX_SOURCELOCATION_H
