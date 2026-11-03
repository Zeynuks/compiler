#include <catch2/catch_test_macros.hpp>

#include "TextScanner.h"

using namespace lex;

TEST_CASE("TextScanner читает символы с предпросмотром и сдвигом позиции", "[TextScanner]")
{
	TextScanner scanner("the\nend");

	REQUIRE(scanner.Peek() == 't');
	REQUIRE(scanner.Peek(1) == 'h');
	REQUIRE(scanner.Peek(3) == '\n');
	REQUIRE(scanner.Peek(7) == '\0');

	scanner.Advance();
	scanner.Advance();
	REQUIRE(scanner.Peek() == 'e');
	REQUIRE(scanner.Peek(2) == 'e');

	scanner.Advance();
	scanner.Advance();
	scanner.Advance();
	scanner.Advance();
	REQUIRE(scanner.Peek() == 'd');
	REQUIRE_FALSE(scanner.IsEnd());

	scanner.Advance();
	REQUIRE(scanner.IsEnd());
	REQUIRE(scanner.Peek() == '\0');

	// Сдвиг за конец текста ничего не трогает и не ломает
	scanner.Advance();
	REQUIRE(scanner.IsEnd());
}

TEST_CASE("TextScanner считает строки и колонки", "[TextScanner]")
{
	TextScanner scanner("a\nb\r\nc\rd");
	const auto advanceAndGetLocation = [&scanner] {
		scanner.Advance();
		return scanner.GetLocation();
	};

	REQUIRE(scanner.GetLocation() == SourceLocation{ 1, 1 });
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 1, 2 }); // \n
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 2, 1 }); // b
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 2, 2 }); // \r
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 2, 3 }); // \n после \r
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 3, 1 }); // c
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 3, 2 }); // \r
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 4, 1 }); // d
	REQUIRE(advanceAndGetLocation() == SourceLocation{ 4, 2 }); // EOF
}

TEST_CASE("TextScanner считает колонки в символах Unicode", "[TextScanner]")
{
	// буквы "ё" и "ж" занимает 2 байта в UTF-8.
	TextScanner scanner("ёж!");
	scanner.Advance();
	scanner.Advance();
	REQUIRE(scanner.GetLocation() == SourceLocation{ 1, 2 });

	scanner.Advance();
	scanner.Advance();
	REQUIRE(scanner.GetLocation() == SourceLocation{ 1, 3 });
	REQUIRE(scanner.Peek() == '!');
}

TEST_CASE("TextScanner захватывает текст от начала токена", "[TextScanner]")
{
	TextScanner scanner("var x");
	scanner.StartCapture();
	REQUIRE(scanner.GetCaptured().empty());

	scanner.Advance();
	scanner.Advance();
	scanner.Advance();
	REQUIRE(scanner.GetCaptured() == "var");

	scanner.Advance();
	scanner.StartCapture();
	scanner.Advance();
	REQUIRE(scanner.GetCaptured() == "x");
}
