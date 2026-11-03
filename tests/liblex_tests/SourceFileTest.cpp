#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "SourceFile.h"

using namespace lex;

namespace
{

// Создает временный файл с заданным содержимым и удаляет его при разрушении.
class TempFile
{
public:
	TempFile(std::string_view name, std::string_view content)
		: m_path(std::filesystem::temp_directory_path() / name)
	{
		std::ofstream out(m_path, std::ios::binary | std::ios::trunc);
		out << content;
	}

	~TempFile()
	{
		std::error_code ec;
		std::filesystem::remove(m_path, ec);
	}

	TempFile(const TempFile&) = delete;
	TempFile& operator=(const TempFile&) = delete;
	TempFile(TempFile&&) = delete;
	TempFile& operator=(TempFile&&) = delete;

	[[nodiscard]] const std::filesystem::path& GetPath() const
	{
		return m_path;
	}

private:
	std::filesystem::path m_path;
};

} // namespace

TEST_CASE("ReadSourceFile загружает файл в память без изменений", "[SourceFile]")
{
	const std::string content = "val x = 1\r\nval y = \"Привет\"\rprint(x)\n";
	const TempFile file("liblex_tests_source_file.txt", content);

	REQUIRE(ReadSourceFile(file.GetPath()) == content);
}

TEST_CASE("ReadSourceFile бросает исключение для несуществующего файла", "[SourceFile]")
{
	const auto path = std::filesystem::temp_directory_path() / "liblex_tests_missing_file.txt";

	REQUIRE_THROWS_AS(ReadSourceFile(path), std::runtime_error);
}
