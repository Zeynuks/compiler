#include "VirtualMachine.h"

#include <istream>
#include <ostream>
#include <print>
#include <stdexcept>
#include <utility>

namespace vm
{

namespace
{
std::int32_t CheckedInt(const std::int64_t value)
{
	if (!std::in_range<std::int32_t>(value))
	{
		throw std::runtime_error("Int overflow");
	}

	return static_cast<std::int32_t>(value);
}
} // namespace

VirtualMachine::VirtualMachine(const std::vector<Instruction>& program, std::istream& in, std::ostream& out)
	: m_program(&program)
	, m_in(&in)
	, m_out(&out)
{
}

void VirtualMachine::Run()
{
	while (m_ip < m_program->size())
	{
		if (const auto& instruction = (*m_program)[m_ip++]; std::visit(*this, instruction) == Flow::Stop)
		{
			return;
		}
	}
}

Flow VirtualMachine::operator()(op::Halt /*unused*/) const
{
	return Flow::Stop;
}

Flow VirtualMachine::operator()(const op::Push& push)
{
	m_values.push_back(push.value);
	return Flow::Next;
}

Flow VirtualMachine::operator()(op::Negate /*unused*/)
{
	Push(-static_cast<std::int64_t>(Pop()));
	return Flow::Next;
}

Flow VirtualMachine::operator()(op::Println /*unused*/)
{
	std::println(*m_out, "{}", Pop());
	if (!*m_out)
	{
		throw std::runtime_error("Output error");
	}

	return Flow::Next;
}

Flow VirtualMachine::operator()(op::Read /*unused*/)
{
	m_values.push_back(ReadByte());
	return Flow::Next;
}

std::int32_t VirtualMachine::Pop()
{
	if (m_values.empty())
	{
		throw std::runtime_error("VM stack empty");
	}

	const auto value = m_values.back();
	m_values.pop_back();

	return value;
}

void VirtualMachine::Push(const std::int64_t value)
{
	m_values.push_back(CheckedInt(value));
}

std::int32_t VirtualMachine::ReadByte()
{
	if (const auto value = m_in->get(); value != std::istream::traits_type::eof())
	{
		return static_cast<unsigned char>(value);
	}

	if (m_in->bad() || !m_in->eof())
	{
		throw std::runtime_error("Input error");
	}

	return -1;
}

} // namespace vm