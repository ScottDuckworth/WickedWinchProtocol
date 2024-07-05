#include "PathEval.h"
#include "proto/PathExpression.pb.h"

#include <algorithm>
#include <cassert>

namespace wickedwinch::patheval {

PathEval PathAt(const wickedwinch::proto::Path& path, uint32_t t) {
  struct AtCompare {
    bool operator()(uint32_t t, const wickedwinch::proto::PathSegment& segment) const {
      return t < segment.start_time();
    }
  };
  const auto& segments = path.segments();
  auto it = std::upper_bound(segments.begin(), segments.end(), t, AtCompare());
  if (it == segments.begin()) return {nullptr, 0};
  --it;
  return {&*it, t - it->start_time()};
}

wickedwinch::postfix::EvalStatus PathEval::Eval(std::vector<float>& stack) const {
  assert(segment != nullptr);
  stack.clear();
  stack.push_back(seconds());
  return postfix::Eval(segment->expr(), stack);
}

}