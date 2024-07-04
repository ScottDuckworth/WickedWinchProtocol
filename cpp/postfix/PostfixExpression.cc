#include "PostfixExpression.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <utility>
#include <vector>

namespace wickedwinch::postfix {

namespace {

void push(std::vector<float>& stack, std::span<const float>& f, int32_t n) {
  std::span<const float> v(f.data(), n);
  std::copy(v.begin(), v.end(), std::back_inserter(stack));
  f = std::span<const float>(f.data() + n, f.size() - n);
};

std::pair<int32_t, EvalStatus> implicitPushArg(int32_t multiple, int32_t n, std::vector<float>& stack, std::span<const float>& f) {
  bool do_push = n & 1;
  n >>= 1;
  if (do_push) {
    int32_t size = multiple * n;
    if (f.size() < size) return std::make_pair(n, EvalStatus::FloatLiteralsUnderflow);
    push(stack, f, size);
  }
  return std::make_pair(n, EvalStatus::Ok);
}

}

EvalStatus Eval(const wickedwinch::proto::PostfixExpression& expr, std::vector<float>& stack) {
  std::span<const int32_t> i(expr.i().data(), expr.i_size());
  std::span<const float> f(expr.f().data(), expr.f_size());

  auto pop = [&stack] {
    float v = stack.back();
    stack.pop_back();
    return v;
  };
  auto popv = [&stack](int n) {
    std::vector<float> v;
    v.reserve(n);
    std::copy(stack.end() - n, stack.end(), std::back_inserter(v));
    stack.resize(stack.size() - n);
    return v;
  };
  auto peekv = [&stack](int n) {
    return std::span<float>(stack.end() - n, stack.end());
  };
  auto popi = [&i] {
    int32_t n = i[0];
    i = std::span<const int32_t>(i.data() + 1, i.size() - 1);
    return n;
  };

  for (const auto op : expr.op()) {
    using wickedwinch::proto::Operation;
    switch (op) {
    case Operation::Push: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (f.size() < n) return EvalStatus::FloatLiteralsUnderflow;
      push(stack, f, n);
      break;
    }
    case Operation::Pop: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (stack.size() < n) return EvalStatus::StackUnderflow;
      stack.resize(stack.size() - n);
      break;
    }
    case Operation::Dup: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (stack.size() < n+1) return EvalStatus::StackUnderflow;
      float v = *(stack.end() - n - 1);
      stack.push_back(v);
      break;
    }
    case Operation::RotL: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (n > 1) {
        if (stack.size() < n) return EvalStatus::StackUnderflow;
        std::span<float> values = peekv(n);
        float l = values[0];
        std::copy(values.begin() + 1, values.end(), values.begin());
        values.back() = l;
      }
      break;
    }
    case Operation::RotR: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (n > 1) {
        if (stack.size() < n) return EvalStatus::StackUnderflow;
        std::span<float> values = peekv(n);
        float r = values.back();
        std::copy(values.begin(), values.end() - 1, values.begin() + 1);
        values[0] = r;
      }
      break;
    }
    case Operation::Rev: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (stack.size() < n) return EvalStatus::StackUnderflow;
      std::span<float> values = peekv(n);
      std::reverse(values.begin(), values.end());
      break;
    }
    case Operation::Transpose: {
      if (i.size() < 2) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      int32_t size = rows * cols;
      if (stack.size() < size) return EvalStatus::StackUnderflow;
      std::span<float> m = peekv(size);
      std::vector<float> t(size);
      for (int32_t i = 0; i < rows; ++i) {
        for (int32_t j = 0; j < cols; ++j) {
          t[rows*j+i] = m[cols*i+j];
        }
      }
      std::copy(t.begin(), t.end(), m.begin());
      break;
    }
    case Operation::Add: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float rhs = pop();
      stack.back() += rhs;
      break;
    }
    case Operation::Sub: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float rhs = pop();
      stack.back() -= rhs;
      break;
    }
    case Operation::Mul: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float rhs = pop();
      stack.back() *= rhs;
      break;
    }
    case Operation::MulAdd: {
      if (stack.size() < 3) return EvalStatus::StackUnderflow;
      float c = pop();
      float b = pop();
      stack.back() += b * c;
      break;
    }
    case Operation::Div: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float rhs = pop();
      stack.back() /= rhs;
      break;
    }
    case Operation::Mod: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float rhs = pop();
      float lhs = pop();
      stack.push_back(std::fmod(lhs, rhs));
      break;
    }
    case Operation::Neg: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(-operand);
      break;
    }
    case Operation::Abs: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::abs(operand));
      break;
    }
    case Operation::Inv: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(1.0f / operand);
      break;
    }
    case Operation::Pow: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float exp = pop();
      float base = pop();
      stack.push_back(std::pow(base, exp));
      break;
    }
    case Operation::Sqrt: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::sqrt(operand));
      break;
    }
    case Operation::Exp: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::exp(operand));
      break;
    }
    case Operation::Ln: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::log(operand));
      break;
    }
    case Operation::Sin: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::sin(operand));
      break;
    }
    case Operation::Cos: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::cos(operand));
      break;
    }
    case Operation::Tan: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::tan(operand));
      break;
    }
    case Operation::Asin: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::asin(operand));
      break;
    }
    case Operation::Acos: {
      if (stack.size() < 1) return EvalStatus::StackUnderflow;
      float operand = pop();
      stack.push_back(std::acos(operand));
      break;
    }
    case Operation::Atan2: {
      if (stack.size() < 2) return EvalStatus::StackUnderflow;
      float x = pop();
      float y = pop();
      stack.push_back(std::atan2(y, x));
      break;
    }
    case Operation::PolyVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::vector<float> coeff = popv(size);
      float param = pop();
      float p = 1;
      float result = 0;
      for (int32_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= param;
      }
      stack.push_back(result);
      break;
    }
    case Operation::PolyMat: {
      if (i.size() < 2) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      int32_t size = rows * cols;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::vector<float> coeff = popv(size);
      float param = pop();
      float p = 1;
      stack.resize(stack.size() + cols);
      std::span<float> result = peekv(cols);
      for (int32_t i = 0; i < rows; ++i) {
        for (int32_t j = 0; j < cols; ++j) {
          result[j] += coeff[cols*i+j] * p;
          p *= param;
        }
      }
      break;
    }
    case Operation::AddVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::vector<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case Operation::SubVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::vector<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case Operation::MulVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::vector<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case Operation::ScaleVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::vector<float> vec = popv(size);
      float scalar = pop();
      float result = 0;
      for (int32_t i = 0; i < size; ++i) {
        result *= scalar * vec[i];
      }
      stack.push_back(result);
      break;
    }
    case Operation::NormVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size) return EvalStatus::StackUnderflow;
      std::vector<float> vec = popv(size);
      float result = 0;
      for (int32_t i = 0; i < size; ++i) {
        result *= vec[i] * vec[i];
      }
      stack.push_back(std::sqrt(result));
      break;
    }
    case Operation::Lerp: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (stack.size() < size*2+1) return EvalStatus::StackUnderflow;
      std::vector<float> v1 = popv(size);
      std::vector<float> v0 = popv(size);
      float t = pop();
      stack.resize(stack.size() + size);
      std::span<float> result = peekv(size);
      for (int32_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      break;
    }
    case Operation::Lut: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      int32_t size = rows * cols;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size+1);
      float t = data[0];
      std::vector<std::span<float>> lut(rows);
      for (int32_t i = 0; i < rows; ++i) {
        lut[i] = std::span<float>(data.data() + (i*cols+1), cols - 1);
      }
      std::vector<float> result(cols-1);
      struct FrontLess {
        bool operator()(const std::span<float>& lhs, const std::span<float>& rhs) const {
          return lhs.front() < rhs.front();
        }
      };
      auto ub = std::upper_bound(lut.begin(), lut.end(), std::span<float>(&t, 1), FrontLess());
      if (ub == lut.begin()) {
        auto bound = lut.front();
        std::copy(bound.begin() + 1, bound.end(), result.begin());
      } else if (ub == lut.end()) {
        auto bound = lut.back();
        std::copy(bound.begin() + 1, bound.end(), result.begin());
      } else {
        auto lb = ub - 1;
        float t0 = (*lb)[0];
        float t1 = (*ub)[0];
        t = (t - t0) / (t1 - t0);
        std::span<const float> v0(lb->data(), cols-1);
        std::span<const float> v1(ub->data(), cols-1);
        for (int32_t i = 0; i < size; ++i) {
          result[i] = (1-t)*v0[i] + t*v1[i];
        }
      }
      stack.resize(stack.size() - (size + 1));
      std::copy(result.begin(), result.end(), std::back_inserter(stack));
      break;
    }
    default:
      return EvalStatus::UndefinedOperation;
    }
  }
  return EvalStatus::Ok;
}

}