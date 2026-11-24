#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_contains.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "liblex/SourceFile.h"
#include "libparse/Parser.h"
#include "libvm/VirtualMachine.h"

namespace fs = std::filesystem;
using Program = std::vector<vm::Instruction>;

[[nodiscard]] static std::string ReadFile(const fs::path& path)
{
	std::ifstream file(path);
	REQUIRE(file.is_open());
	return { std::istreambuf_iterator(file), std::istreambuf_iterator<char>() };
}

[[nodiscard]] static fs::path ReplaceExtension(fs::path path, const std::string& extension)
{
	path.replace_extension(extension);
	return path;
}

[[nodiscard]] static std::string Run(const fs::path& slopFile, const fs::path& inputFile)
{
	std::istringstream inputStream;
	std::ostringstream outputStream;
	if (fs::exists(inputFile))
	{
		inputStream.str(ReadFile(inputFile));
	}

	std::string const source = lex::ReadSourceFile(slopFile.string());
	parse::Parser parser(source);
	Program const program = parser.ParseProgram();

	vm::VirtualMachine machine(program, inputStream, outputStream);
	machine.Run();

	return outputStream.str();
}

TEST_CASE("Acceptance tests from files", "[acceptance]")
{
	fs::path const testDir = fs::path(__FILE__).parent_path() / "programs";

	REQUIRE(fs::exists(testDir));
	REQUIRE(fs::is_directory(testDir));

	for (const auto& entry : fs::directory_iterator(testDir))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".slop")
		{
			const fs::path& slopFile = entry.path();
			fs::path const stdoutFile = ReplaceExtension(slopFile, ".stdout");
			fs::path const errorFile = ReplaceExtension(slopFile, ".stderr");
			fs::path const stdinFile = ReplaceExtension(slopFile, ".stdin");

			bool const expectError = fs::exists(errorFile);
			std::string const testName = slopFile.stem().string();
			DYNAMIC_SECTION("Test case: " << testName)
			{
				try
				{
					std::string result = Run(slopFile, stdinFile);
					if (expectError)
					{
						FAIL("Ожидалась ошибка, но программа выполнилась успешно");
					}

					REQUIRE(fs::exists(stdoutFile));
					std::string expectedOutput = ReadFile(stdoutFile);
					REQUIRE(result == expectedOutput);
				}
				catch (const std::exception& e)
				{
					if (!expectError)
					{
						FAIL(std::string("Программа должна была завершиться успешно но возникла следующая ошибка\n") + e.what());
					}

					std::string const expectedError = ReadFile(errorFile);
					REQUIRE_THAT(e.what(), Catch::Matchers::ContainsSubstring(expectedError));
				}
			}
		}
	}
}
