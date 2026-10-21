#include "SourceFile.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace lex
{

std::string ReadSourceFile(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input.is_open())
	{
		throw std::runtime_error("Cannot open file " + path.string());
	}

	std::string content{ std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
	if (input.bad())
	{
		throw std::runtime_error("Cannot read file " + path.string());
	}

	return content;
}

} // namespace lex
