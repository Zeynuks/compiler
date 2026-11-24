#pragma once

#include <cstdint>
#include <stdexcept>
#include <variant>
namespace vm
{
namespace op
{
struct Halt
{
};

struct Push
{
	std::int32_t value;
};

struct Negate
{
};

struct Println
{
};

struct Read
{
};

struct AddStrategy
{
	static std::int64_t Apply(const std::int64_t left, const std::int64_t right) { return left + right; }
};

struct SubtractStrategy
{
	static std::int64_t Apply(const std::int64_t left, const std::int64_t right) { return left - right; }
};

struct MultiplyStrategy
{
	static std::int64_t Apply(const std::int64_t left, const std::int64_t right) { return left * right; }
};

struct DivideStrategy
{
	static std::int64_t Apply(const std::int64_t left, const std::int64_t right)
	{
		if (right == 0)
		{
			throw std::runtime_error("Division by zero");
		}

		return left / right;
	}
};

struct RemainderStrategy
{
	static std::int64_t Apply(const std::int64_t left, const std::int64_t right)
	{
		if (right == 0)
		{
			throw std::runtime_error("Division by zero");
		}

		return left % right;
	}
};

template <class Strategy>
struct Binary
{
};

using Add = Binary<AddStrategy>;
using Subtract = Binary<SubtractStrategy>;
using Multiply = Binary<MultiplyStrategy>;
using Divide = Binary<DivideStrategy>;
using Remainder = Binary<RemainderStrategy>;
} // namespace op

using Instruction = std::variant<
	op::Halt,
	op::Push,
	op::Add,
	op::Subtract,
	op::Multiply,
	op::Divide,
	op::Remainder,
	op::Negate,
	op::Println,
	op::Read>;
} // namespace vm