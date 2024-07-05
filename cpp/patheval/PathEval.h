#pragma once

#include "proto/PathExpression.pb.h"
#include "postfix/PostfixExpression.h"

#include <vector>

namespace wickedwinch::patheval {

struct PathEval {
  const wickedwinch::proto::PathSegment* segment;
  uint32_t t;

  float seconds() const { return t * 1e-3f; }

  wickedwinch::postfix::EvalStatus Eval(std::vector<float>& stack) const;
};

PathEval PathAt(const wickedwinch::proto::Path& path, uint32_t t);

}