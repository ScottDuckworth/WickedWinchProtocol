#pragma once

#include <vector>

#include "proto/PathExpression.pb.h"

namespace wickedwinch::postfix {

void Eval(const wickedwinch::proto::PostfixExpression& expr, std::vector<float>& stack);

}