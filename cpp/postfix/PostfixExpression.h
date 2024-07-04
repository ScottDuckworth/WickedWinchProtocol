#pragma once

#include <vector>

#include "proto/PathExpression.pb.h"

namespace wickedwinch::postfix {

enum class EvalStatus {
  Ok,
	UndefinedOperation,
	StackUnderflow,
	IntLiteralsUnderflow,
	FloatLiteralsUnderflow,
};

EvalStatus Eval(const wickedwinch::proto::PostfixExpression& expr, std::vector<float>& stack);

}