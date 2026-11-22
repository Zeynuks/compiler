#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>

#include "Parser.h"
#include "SourceFile.h"
#include "VirtualMachine.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: compiler <source-file>\n";
		return EXIT_FAILURE;
	}

	try
	{
		const std::span args(argv, static_cast<std::size_t>(argc));
		parse::Parser parser(lex::ReadSourceFile(args[1]));
		const auto program = parser.ParseProgram();
		vm::VirtualMachine machine(program, std::cin, std::cout);
		machine.Run();
		if (!std::cout.flush())
		{
			throw std::runtime_error("Output error");
		}
	}
	catch (const std::exception& error)
	{
		std::cerr << error.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
