#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <variant>
#include <vector>

#include "Instruction.h"

namespace vm
{

enum class Flow : std::uint8_t
{
	Next,
	Stop,
};

class VirtualMachine
{
public:
	VirtualMachine(const std::vector<Instruction>& program, std::istream& in, std::ostream& out);

	void Run();

	Flow operator()(op::Halt /*unused*/) const;
	Flow operator()(const op::Push& push);
	Flow operator()(op::Negate /*unused*/);
	Flow operator()(op::Println /*unused*/);
	Flow operator()(op::Read /*unused*/);

	template <class Strategy>
	Flow operator()(op::Binary<Strategy> /*unused*/)
	{
		const auto right = Pop();
		const auto left = Pop();
		Push(Strategy::Apply(left, right));
		return Flow::Next;
	}

private:
	const std::vector<Instruction>* m_program;
	std::size_t m_ip = 0;
	std::istream* m_in;
	std::ostream* m_out;
	std::vector<std::int32_t> m_values;

	std::int32_t Pop();
	void Push(std::int64_t value);
	std::int32_t ReadByte();
};

} // namespace vm