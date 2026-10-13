#include <cstdint>
#include <format>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "Lexer.h"

using namespace lex;

namespace
{

struct TestCase
{
	std::string source;
	std::vector<std::string> expected;
};

// Форматирует токен для сравнения в тестах, типо: "Identifier(x)", "IntLiteral(42)", "Plus"
std::string FormatToken(const Token& token)
{
	std::string result{ ToString(token.type) };
	std::visit(
		[&result](const auto& value) {
			using ValueType = std::decay_t<decltype(value)>;
			if constexpr (std::is_same_v<ValueType, char32_t>)
			{
				result += std::format("(U+{:04X})", static_cast<uint32_t>(value));
			}
			else if constexpr (!std::is_same_v<ValueType, std::monostate>)
			{
				result += std::format("({})", value);
			}
		},
		token.value);
	return result;
}

// Разбирает весь исходный код и возвращает токены без конца файла.
std::vector<Token> ParseAllTokens(const std::string& source)
{
	Lexer lexer(source);
	std::vector<Token> tokens;

	// Каждый токен занимает хотя бы один байт, кроме ошибки незакрытой строки,
	// которая следует за открывающей кавычкой.
	// Так что токенов не больше, чем удвоенное число байт.
	for (size_t i = 0; i <= 2 * source.size(); ++i)
	{
		Token token = lexer.ParseToken();
		if (token.type == TokenType::EndOfFile)
		{
			return tokens;
		}
		tokens.push_back(std::move(token));
	}
	FAIL("Lexer has not reached end of file");
	return tokens;
}

// Возвращает отформатированные токены, типо: "Identifier(x)".
std::vector<std::string> Tokenize(const std::string& source)
{
	std::vector<std::string> results;
	for (const Token& token : ParseAllTokens(source))
	{
		results.push_back(FormatToken(token));
	}
	return results;
}

// Возвращает отформатированные токены с позициями, а-ля: "Identifier(x) 1:5"
std::vector<std::string> TokenizeWithLocations(const std::string& source)
{
	std::vector<std::string> results;
	for (const Token& token : ParseAllTokens(source))
	{
		results.push_back(std::format("{} {}:{}", FormatToken(token), token.location.line, token.location.column));
	}
	return results;
}

void CheckTokenize(const std::string& source, const std::vector<std::string>& expected)
{
	CAPTURE(source);
	REQUIRE(Tokenize(source) == expected);
}

void CheckTokenize(const TestCase& testCase)
{
	CheckTokenize(testCase.source, testCase.expected);
}

} // namespace

TEST_CASE("Lexer возвращает конец файла для пустого текста", "[Lexer]")
{
	Lexer lexer("");
	REQUIRE(lexer.ParseToken().type == TokenType::EndOfFile);
	REQUIRE(lexer.ParseToken().type == TokenType::EndOfFile);
}

TEST_CASE("Lexer разбирает идентификаторы", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"alice Bob C00L d_2 _x x",
			{
				"Identifier(alice)",
				"Identifier(Bob)",
				"Identifier(C00L)",
				"Identifier(d_2)",
				"Identifier(_x)",
				"Identifier(x)",
			},
		},
		TestCase{
			"_ __ _1",
			{
				"Error(_)",
				"Error(__)",
				"Identifier(_1)",
			},
		},
		TestCase{
			"имя x",
			{
				"Error(и)",
				"Error(м)",
				"Error(я)",
				"Identifier(x)",
			},
		}));
}

TEST_CASE("Lexer разбирает ключевые слова", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"if else while do for in break continue return",
			{
				"If",
				"Else",
				"While",
				"Do",
				"For",
				"In",
				"Break",
				"Continue",
				"Return",
			},
		},
		TestCase{
			"fun struct val var",
			{
				"Fun",
				"Struct",
				"Val",
				"Var",
			},
		},
		TestCase{
			"true false",
			{
				"True",
				"False",
			},
		},
		TestCase{
			"Int Double Boolean Char String Unit Array",
			{
				"IntType",
				"DoubleType",
				"BooleanType",
				"CharType",
				"StringType",
				"UnitType",
				"ArrayType",
			},
		}));
}

TEST_CASE("Lexer различает регистр в ключевых словах", "[Lexer]")
{
	CheckTokenize("int If WHILE string",
		{
			"Identifier(int)",
			"Identifier(If)",
			"Identifier(WHILE)",
			"Identifier(string)",
		});
}

TEST_CASE("Lexer не допускает зарезервированные слова", "[Lexer]")
{
	CheckTokenize("as catch class finally import interface is null object package super this throw try typealias typeof when",
		{
			"Error(as)",
			"Error(catch)",
			"Error(class)",
			"Error(finally)",
			"Error(import)",
			"Error(interface)",
			"Error(is)",
			"Error(null)",
			"Error(object)",
			"Error(package)",
			"Error(super)",
			"Error(this)",
			"Error(throw)",
			"Error(try)",
			"Error(typealias)",
			"Error(typeof)",
			"Error(when)",
		});
}

TEST_CASE("Lexer разбирает идентификаторы, начинающиеся с ключевого слова", "[Lexer]")
{
	CheckTokenize("iffy fun_ values Integer classes",
		{
			"Identifier(iffy)",
			"Identifier(fun_)",
			"Identifier(values)",
			"Identifier(Integer)",
			"Identifier(classes)",
		});
}

TEST_CASE("Lexer разбирает литералы целых чисел", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"0 42 2147483647",
			{
				"IntLiteral(0)",
				"IntLiteral(42)",
				"IntLiteral(2147483647)",
			},
		},
		TestCase{
			"1_000 1__0 12_34_56",
			{
				"IntLiteral(1000)",
				"IntLiteral(10)",
				"IntLiteral(123456)",
			},
		},
		TestCase{
			"0xFF 0x7fff_ffff 0x0 0b101 0b1_0",
			{
				"IntLiteral(255)",
				"IntLiteral(2147483647)",
				"IntLiteral(0)",
				"IntLiteral(5)",
				"IntLiteral(2)",
			},
		}));
}

TEST_CASE("Lexer не допускает некорректные литералы целых чисел", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"2147483648 0x80000000 0b100000000000000000000000000000000",
			{
				"Error(2147483648)",
				"Error(0x80000000)",
				"Error(0b100000000000000000000000000000000)",
			},
		},
		TestCase{
			"007 0_1 00",
			{
				"Error(007)",
				"Error(0_1)",
				"Error(00)",
			},
		},
		TestCase{
			"1_ 0x_1 0x 0b",
			{
				"Error(1_)",
				"Error(0x_1)",
				"Error(0x)",
				"Error(0b)",
			},
		},
		TestCase{
			"12abc 0b102 0xFG 0XFF 7_x",
			{
				"Error(12abc)",
				"Error(0b102)",
				"Error(0xFG)",
				"Error(0XFF)",
				"Error(7_x)",
			},
		}));
}

TEST_CASE("Lexer разбирает литералы вещественных чисел", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"1.5 0.25 3.0 1_000.000_5",
			{
				"DoubleLiteral(1.5)",
				"DoubleLiteral(0.25)",
				"DoubleLiteral(3)",
				"DoubleLiteral(1000.0005)",
			},
		},
		TestCase{
			"2.5e-3 1E10 1e3 1e+2 0e0",
			{
				"DoubleLiteral(0.0025)",
				"DoubleLiteral(1e+10)",
				"DoubleLiteral(1000)",
				"DoubleLiteral(100)",
				"DoubleLiteral(0)",
			},
		}));
}

TEST_CASE("Lexer не допускает некорректные литералы вещественных чисел", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"1e 2.5E 3.5x 1e5_",
			{
				"Error(1e)",
				"Error(2.5E)",
				"Error(3.5x)",
				"Error(1e5_)",
			},
		},
		TestCase{
			"00.5 01e2",
			{
				"Error(00.5)",
				"Error(01e2)",
			},
		},
		TestCase{
			"1e400 1.0e-400",
			{
				"Error(1e400)",
				"Error(1.0e-400)",
			},
		}));
}

TEST_CASE("Lexer разбирает операторы", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"+ - * / %",
			{
				"Plus",
				"Minus",
				"Multiply",
				"Divide",
				"Modulo",
			},
		},
		TestCase{
			"== != === !== < > <= >=",
			{
				"Equal",
				"NotEqual",
				"Identical",
				"NotIdentical",
				"Less",
				"Greater",
				"LessOrEqual",
				"GreaterOrEqual",
			},
		},
		TestCase{
			"&& || !",
			{
				"And",
				"Or",
				"Not",
			},
		},
		TestCase{
			"= += -= *= /= %=",
			{
				"Assign",
				"PlusAssign",
				"MinusAssign",
				"MultiplyAssign",
				"DivideAssign",
				"ModuloAssign",
			},
		},
		TestCase{
			"++ --",
			{
				"Increment",
				"Decrement",
			},
		},
		TestCase{
			". .. ..< ...",
			{
				"Dot",
				"Range",
				"RangeUntil",
				"Range",
				"Dot",
			},
		},
		TestCase{
			"x in xs || x !in ys",
			{
				"Identifier(x)",
				"In",
				"Identifier(xs)",
				"Or",
				"Identifier(x)",
				"NotIn",
				"Identifier(ys)",
			},
		}));
}

TEST_CASE("Lexer выбирает самую длинную лексему оператора", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a+++b",
			{
				"Identifier(a)",
				"Increment",
				"Plus",
				"Identifier(b)",
			},
		},
		TestCase{
			"a====b!==c",
			{
				"Identifier(a)",
				"Identical",
				"Assign",
				"Identifier(b)",
				"NotIdentical",
				"Identifier(c)",
			},
		},
		TestCase{
			"!inside !in(x) !in",
			{
				"Not",
				"Identifier(inside)",
				"NotIn",
				"LeftParen",
				"Identifier(x)",
				"RightParen",
				"NotIn",
			},
		},
		TestCase{
			"!i !_in",
			{
				"Not",
				"Identifier(i)",
				"Not",
				"Identifier(_in)",
			},
		}));
}

TEST_CASE("Lexer не относит точку без цифр к литералу числа", "[Lexer]")
{
	CheckTokenize(".5 5. 1..2 1.foo",
		{
			"Dot",
			"IntLiteral(5)",
			"IntLiteral(5)",
			"Dot",
			"IntLiteral(1)",
			"Range",
			"IntLiteral(2)",
			"IntLiteral(1)",
			"Dot",
			"Identifier(foo)",
		});
}

TEST_CASE("Lexer не допускает недопустимые символы", "[Lexer]")
{
	CheckTokenize("& | ? @ # \\ `",
		{
			"Error(&)",
			"Error(|)",
			"Error(?)",
			"Error(@)",
			"Error(#)",
			"Error(\\)",
			"Error(`)",
		});
}

TEST_CASE("Lexer разбирает скобки и разделители", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"( ) { } [ ]",
			{
				"LeftParen",
				"RightParen",
				"LeftBrace",
				"RightBrace",
				"LeftBracket",
				"RightBracket",
			},
		},
		TestCase{
			"Array<Array<Int>>",
			{
				"ArrayType",
				"Less",
				"ArrayType",
				"Less",
				"IntType",
				"Greater",
				"Greater",
			},
		},
		TestCase{
			", : ;",
			{
				"Comma",
				"Colon",
				"Semicolon",
			},
		}));
}

TEST_CASE("Lexer пропускает пробельные символы", "[Lexer]")
{
	CheckTokenize(" \t x \t y\t\r\n \t z \n", { "Identifier(x)", "Identifier(y)", "Newline", "Identifier(z)" });
}

TEST_CASE("Lexer считает перевод строки разделителем инструкций", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a\nb",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"a\r\nb\rc",
			{ "Identifier(a)", "Newline", "Identifier(b)", "Newline", "Identifier(c)" },
		},
		TestCase{
			"a\n\n \t\r\n  b",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"\n\na\n\n",
			{ "Identifier(a)" },
		},
		TestCase{
			"x++\ny--\nz",
			{ "Identifier(x)", "Increment", "Newline", "Identifier(y)", "Decrement", "Newline", "Identifier(z)" },
		},
		TestCase{
			"f(a)\nb",
			{ "Identifier(f)", "LeftParen", "Identifier(a)", "RightParen", "Newline", "Identifier(b)" },
		},
		TestCase{
			"{\na\nb\n}",
			{ "LeftBrace", "Identifier(a)", "Newline", "Identifier(b)", "Newline", "RightBrace" },
		}));
}

TEST_CASE("Lexer не считает перевод строки внутри круглых и квадратных скобок разделителем", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"f(a\n,\nb\n)",
			{ "Identifier(f)", "LeftParen", "Identifier(a)", "Comma", "Identifier(b)", "RightParen" },
		},
		TestCase{
			"xs[\ni\n]",
			{ "Identifier(xs)", "LeftBracket", "Identifier(i)", "RightBracket" },
		},
		TestCase{
			"(a\n{\nb\nc\n}\nd)",
			{ "LeftParen", "Identifier(a)", "LeftBrace", "Identifier(b)", "Newline", "Identifier(c)", "Newline", "RightBrace", "Identifier(d)", "RightParen" },
		},
		TestCase{
			"(]\na)\nb",
			{ "LeftParen", "RightBracket", "Identifier(a)", "RightParen", "Newline", "Identifier(b)" },
		}));
}

TEST_CASE("Lexer не считает перевод строки после оператора разделителем", "[Lexer]")
{
	const auto [operatorText, operatorName] = GENERATE(table<std::string, std::string>({
		{ "+", "Plus" },
		{ "-", "Minus" },
		{ "*", "Multiply" },
		{ "/", "Divide" },
		{ "%", "Modulo" },
		{ "==", "Equal" },
		{ "!=", "NotEqual" },
		{ "===", "Identical" },
		{ "!==", "NotIdentical" },
		{ "<", "Less" },
		{ ">", "Greater" },
		{ "<=", "LessOrEqual" },
		{ ">=", "GreaterOrEqual" },
		{ "&&", "And" },
		{ "||", "Or" },
		{ "..", "Range" },
		{ "..<", "RangeUntil" },
		{ "in", "In" },
		{ "!in", "NotIn" },
		{ "=", "Assign" },
		{ "+=", "PlusAssign" },
		{ "-=", "MinusAssign" },
		{ "*=", "MultiplyAssign" },
		{ "/=", "DivideAssign" },
		{ "%=", "ModuloAssign" },
		{ ".", "Dot" },
		{ ",", "Comma" },
		{ ":", "Colon" },
		{ "else", "Else" },
		{ "do", "Do" },
	}));

	CheckTokenize("a " + operatorText + "\nb", { "Identifier(a)", operatorName, "Identifier(b)" });
}

TEST_CASE("Lexer не считает перевод строки после заголовка if, while, for разделителем", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"if (a)\nb\nc",
			{ "If", "LeftParen", "Identifier(a)", "RightParen", "Identifier(b)", "Newline", "Identifier(c)" },
		},
		TestCase{
			"while (f(a))\nb",
			{ "While", "LeftParen", "Identifier(f)", "LeftParen", "Identifier(a)", "RightParen", "RightParen", "Identifier(b)" },
		},
		TestCase{
			"for (i in xs)\nb",
			{ "For", "LeftParen", "Identifier(i)", "In", "Identifier(xs)", "RightParen", "Identifier(b)" },
		}));
}

TEST_CASE("Lexer не считает перевод строки перед продолжением выражения разделителем", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a\n.b",
			{ "Identifier(a)", "Dot", "Identifier(b)" },
		},
		TestCase{
			"a\nelse b",
			{ "Identifier(a)", "Else", "Identifier(b)" },
		},
		TestCase{
			"a\n&& b\n|| c",
			{ "Identifier(a)", "And", "Identifier(b)", "Or", "Identifier(c)" },
		},
		TestCase{
			"a\n..b",
			{ "Identifier(a)", "Newline", "Range", "Identifier(b)" },
		},
		TestCase{
			"a\nelseif",
			{ "Identifier(a)", "Newline", "Identifier(elseif)" },
		}));
}

TEST_CASE("Lexer не добавляет перевод строки рядом с точкой с запятой", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a;\nb",
			{ "Identifier(a)", "Semicolon", "Identifier(b)" },
		},
		TestCase{
			"a\n;b",
			{ "Identifier(a)", "Semicolon", "Identifier(b)" },
		},
		TestCase{
			"a\n;\n;\nb",
			{ "Identifier(a)", "Semicolon", "Semicolon", "Identifier(b)" },
		}));
}

TEST_CASE("Lexer пропускает однострочные комментарии", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a // comment / * */ \"\nb",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"a // comment\r\nb",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"a + // comment\nb",
			{ "Identifier(a)", "Plus", "Identifier(b)" },
		},
		TestCase{
			"a // comment",
			{ "Identifier(a)" },
		},
		TestCase{
			"//",
			{},
		}));
}

TEST_CASE("Lexer пропускает блочные комментарии", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a /* comment */ b/**/c /***/ d",
			{ "Identifier(a)", "Identifier(b)", "Identifier(c)", "Identifier(d)" },
		},
		TestCase{
			"a /* outer /* inner */ still comment */ b",
			{ "Identifier(a)", "Identifier(b)" },
		},
		TestCase{
			"a */ b",
			{ "Identifier(a)", "Multiply", "Divide", "Identifier(b)" },
		}));
}

TEST_CASE("Lexer считает блочный комментарий с переводом строки разделителем", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a /* line\n line */ b",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"a /* comment */\nb",
			{ "Identifier(a)", "Newline", "Identifier(b)" },
		},
		TestCase{
			"f(a /*\n*/ b)",
			{ "Identifier(f)", "LeftParen", "Identifier(a)", "Identifier(b)", "RightParen" },
		}));
}

TEST_CASE("Lexer не допускает незакрытый блочный комментарий", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"a /* comment",
			{ "Identifier(a)", "Error(/* comment)" },
		},
		TestCase{
			"a\n/* outer /* inner */\n",
			{ "Identifier(a)", "Error(/* outer /* inner */\n)" },
		}));
}

TEST_CASE("Lexer разбирает литералы символов", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"('a' ' ' '"' '/')",
			{ "CharLiteral(U+0061)", "CharLiteral(U+0020)", "CharLiteral(U+0022)", "CharLiteral(U+002F)" },
		},
		TestCase{
			"'ж' '€' '😀'",
			{ "CharLiteral(U+0436)", "CharLiteral(U+20AC)", "CharLiteral(U+1F600)" },
		},
		TestCase{
			R"('\n' '\t' '\r' '\b' '\'' '\"' '\\' '\$')",
			{
				"CharLiteral(U+000A)",
				"CharLiteral(U+0009)",
				"CharLiteral(U+000D)",
				"CharLiteral(U+0008)",
				"CharLiteral(U+0027)",
				"CharLiteral(U+0022)",
				"CharLiteral(U+005C)",
				"CharLiteral(U+0024)",
			},
		},
		TestCase{
			R"('\u0041' '\u00e9' '\u{1F600}' '\u{41}' '\u{10FFFF}')",
			{
				"CharLiteral(U+0041)",
				"CharLiteral(U+00E9)",
				"CharLiteral(U+1F600)",
				"CharLiteral(U+0041)",
				"CharLiteral(U+10FFFF)",
			},
		}));
}

TEST_CASE("Lexer не допускает некорректные литералы символов", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			"'' 'ab' 'жж'",
			{ "Error('')", "Error('ab')", "Error('жж')" },
		},
		TestCase{
			"x = 'a",
			{ "Identifier(x)", "Assign", "Error('a)" },
		},
		TestCase{
			"'a\nb '\n'",
			{ "Error('a)", "Newline", "Identifier(b)", "Error(')", "Newline", "Error(')" },
		},
		TestCase{
			R"('\q' '\u12' '\u{}' '\u{1234567}' '\u{110000}' '\uD800' '\u{DFFF}')",
			{
				R"(Error('\q'))",
				R"(Error('\u12'))",
				R"(Error('\u{}'))",
				R"(Error('\u{1234567}'))",
				R"(Error('\u{110000}'))",
				R"(Error('\uD800'))",
				R"(Error('\u{DFFF}'))",
			},
		},
		TestCase{
			R"('\u{41')",
			{ R"(Error('\u{41'))" },
		},
		TestCase{
			"'\xFF' '\xD0'",
			{ "Error('\xFF')", "Error('\xD0')" },
		}));
}

TEST_CASE("Lexer разбирает литералы строк", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"("")",
			{ "StringBegin", "StringEnd" },
		},
		TestCase{
			R"("Hello, world!" "x")",
			{ "StringBegin", "StringText(Hello, world!)", "StringEnd", "StringBegin", "StringText(x)", "StringEnd" },
		},
		TestCase{
			R"("Привет, 😀")",
			{ "StringBegin", "StringText(Привет, 😀)", "StringEnd" },
		},
		TestCase{
			R"("// /* 'a' */")",
			{ "StringBegin", "StringText(// /* 'a' */)", "StringEnd" },
		}));
}

TEST_CASE("Lexer разбирает escape-последовательности в строках", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"("\n\t\r\b\"\'\\\$")",
			{ "StringBegin", "StringText(\n\t\r\b\"'\\$)", "StringEnd" },
		},
		TestCase{
			R"("a\u0416\u{1F600}\u{7F}\u{20AC}b")",
			{ "StringBegin", "StringText(aЖ😀\x7F€b)", "StringEnd" },
		}));
}

TEST_CASE("Lexer продолжает разбор строки после неверной escape-последовательности", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"("a\qb" x)",
			{ "StringBegin", R"(Error(a\qb))", "StringEnd", "Identifier(x)" },
		},
		TestCase{
			R"("\u{110000}\u12")",
			{ "StringBegin", R"(Error(\u{110000}\u12))", "StringEnd" },
		}));
}

TEST_CASE("Lexer не допускает незакрытые строки", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"(x = "abc)",
			{ "Identifier(x)", "Assign", "StringBegin", "StringText(abc)", "Error()" },
		},
		TestCase{
			"\"abc\nx\"\n",
			{ "StringBegin", "StringText(abc)", "Error()", "Newline", "Identifier(x)", "StringBegin", "Error()" },
		},
		TestCase{
			"\"\r\n\"",
			{ "StringBegin", "Error()", "Newline", "StringBegin", "Error()" },
		}));
}

TEST_CASE("Lexer разбирает подстановки переменных в строках", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"("x = $x")",
			{ "StringBegin", "StringText(x = )", "TemplateIdentifier(x)", "StringEnd" },
		},
		TestCase{
			R"("$a$b_1!")",
			{ "StringBegin", "TemplateIdentifier(a)", "TemplateIdentifier(b_1)", "StringText(!)", "StringEnd" },
		},
		TestCase{
			R"("\$x")",
			{ "StringBegin", "StringText($x)", "StringEnd" },
		},
		TestCase{
			R"("$_ $__")",
			{ "StringBegin", "Error($_)", "StringText( )", "Error($__)", "StringEnd" },
		}));
}

TEST_CASE("Lexer считает знак доллара без имени обычным символом", "[Lexer]")
{
	CheckTokenize(R"("$ $1 $" + "$")",
		{
			"StringBegin",
			"StringText($ $1 $)",
			"StringEnd",
			"Plus",
			"StringBegin",
			"StringText($)",
			"StringEnd",
		});
}

TEST_CASE("Lexer разбирает подстановки выражений в строках", "[Lexer]")
{
	CheckTokenize(GENERATE(
		TestCase{
			R"("${x * 2}")",
			{ "StringBegin", "TemplateBegin", "Identifier(x)", "Multiply", "IntLiteral(2)", "TemplateEnd", "StringEnd" },
		},
		TestCase{
			R"("a${f("b$c")}d")",
			{
				"StringBegin",
				"StringText(a)",
				"TemplateBegin",
				"Identifier(f)",
				"LeftParen",
				"StringBegin",
				"StringText(b)",
				"TemplateIdentifier(c)",
				"StringEnd",
				"RightParen",
				"TemplateEnd",
				"StringText(d)",
				"StringEnd",
			},
		},
		TestCase{
			R"("${ {a} }" })",
			{ "StringBegin", "TemplateBegin", "LeftBrace", "Identifier(a)", "RightBrace", "TemplateEnd", "StringEnd", "RightBrace" },
		},
		TestCase{
			R"("${a)",
			{ "StringBegin", "TemplateBegin", "Identifier(a)" },
		}));
}

TEST_CASE("Lexer не считает перевод строки внутри подстановки разделителем", "[Lexer]")
{
	CheckTokenize("\"${\na\nb\n}\"\nc",
		{
			"StringBegin",
			"TemplateBegin",
			"Identifier(a)",
			"Identifier(b)",
			"TemplateEnd",
			"StringEnd",
			"Newline",
			"Identifier(c)",
		});
}

TEST_CASE("Lexer определяет позиции токенов", "[Lexer]")
{
	const std::string source = "fun main() {\n"
							   "\tval s = \"a$x${y}\"\n"
							   "\t// comment\n"
							   "\tprint(s) /* c */\n"
							   "}";

	REQUIRE(TokenizeWithLocations(source) == std::vector<std::string>{
				"Fun 1:1",
				"Identifier(main) 1:5",
				"LeftParen 1:9",
				"RightParen 1:10",
				"LeftBrace 1:12",
				"Val 2:2",
				"Identifier(s) 2:6",
				"Assign 2:8",
				"StringBegin 2:10",
				"StringText(a) 2:11",
				"TemplateIdentifier(x) 2:12",
				"TemplateBegin 2:14",
				"Identifier(y) 2:16",
				"TemplateEnd 2:17",
				"StringEnd 2:18",
				"Newline 2:19",
				"Identifier(print) 4:2",
				"LeftParen 4:7",
				"Identifier(s) 4:8",
				"RightParen 4:9",
				"Newline 4:18",
				"RightBrace 5:1",
			});
}

TEST_CASE("Lexer определяет позиции ошибок", "[Lexer]")
{
	REQUIRE(TokenizeWithLocations("\"ёж\" x\n  'ab' \"abc\n/* ") == std::vector<std::string>{
				"StringBegin 1:1",
				"StringText(ёж) 1:2",
				"StringEnd 1:4",
				"Identifier(x) 1:6",
				"Newline 1:7",
				"Error('ab') 2:3",
				"StringBegin 2:8",
				"StringText(abc) 2:9",
				"Error() 2:12",
				"Error(/* ) 3:1",
			});
}

TEST_CASE("Lexer определяет позицию разделителя по первому переводу строки", "[Lexer]")
{
	REQUIRE(TokenizeWithLocations("a  \r\n\n b") == std::vector<std::string>{
				"Identifier(a) 1:1",
				"Newline 1:4",
				"Identifier(b) 3:2",
			});
}
