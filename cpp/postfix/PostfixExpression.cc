#include "PostfixExpression.h"

namespace wickedwinch::postfix {

void Eval(const wickedwinch::proto::PostfixExpression& expr, std::vector<float>& stack) {
  using wickedwinch::proto::Operation;

  for (const auto op : expr.op()) {
    switch (op) {
    case Operation::Undefined:
      break;
    case Operation::Push:
      break;
    }
  }
}

}