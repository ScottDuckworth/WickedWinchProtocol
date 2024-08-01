#pragma once

namespace wickedwinch::protocol {

enum class [[nodiscard]] EvalStatus {
  Ok,
	UndefinedOperation,
	IllegalOperation,
	StackOverflow,
	StackUnderflow,
	IntLiteralsUnderflow,
	FloatLiteralsUnderflow,
};

}
