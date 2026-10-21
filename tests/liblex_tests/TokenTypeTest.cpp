#include <catch2/catch_test_macros.hpp>

#include "TokenType.h"

using namespace lex;

TEST_CASE("ToString возвращает название типа токена", "[TokenType]")
{
	REQUIRE(ToString(TokenType::EndOfFile) == "EndOfFile");
	REQUIRE(ToString(TokenType::Identifier) == "Identifier");
	REQUIRE(ToString(TokenType::NotIn) == "NotIn");
}
