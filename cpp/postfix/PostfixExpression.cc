#include "PostfixExpression.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

namespace wickedwinch::postfix {

namespace {

template <typename OutputIt, typename InputIt>
void memcpy_it(OutputIt dst, InputIt src, size_t count) {
  memcpy(&*dst, &*src, count * sizeof(*src));
}

void push(std::vector<float>& stack, std::span<const float>& f, int32_t n) {
  stack.resize(stack.size() + n);
  memcpy_it(stack.end() - n, f.begin(), n);
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

template <typename Pred>
size_t search(size_t base, size_t n, const Pred& pred) {
  while (n) {
    size_t h = n >> 1;
    if (pred(base + h)) {
      n = h;
    } else {
      if (h == 0) break;
      base += h;
      n -= h;
    }
  }
  return base + n;
}

// Return the smallest index i in [0, n) at which pred(i) is true.
template <typename Pred>
size_t search(size_t n, const Pred& pred) {
  return search(0, n, pred);
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
      if (n < 0) return EvalStatus::IllegalOperation;
      if (f.size() < n) return EvalStatus::FloatLiteralsUnderflow;
      push(stack, f, n);
      break;
    }
    case Operation::Pop: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (n < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < n) return EvalStatus::StackUnderflow;
      stack.resize(stack.size() - n);
      break;
    }
    case Operation::Dup: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (n < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < n+1) return EvalStatus::StackUnderflow;
      float v = *(stack.end() - n - 1);
      stack.push_back(v);
      break;
    }
    case Operation::RotL: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t n = popi();
      if (n < 0) return EvalStatus::IllegalOperation;
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
      if (n < 0) return EvalStatus::IllegalOperation;
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
      if (n < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < n) return EvalStatus::StackUnderflow;
      std::span<float> values = peekv(n);
      std::reverse(values.begin(), values.end());
      break;
    }
    case Operation::Transpose: {
      if (i.size() < 2) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      if (rows < 0) return EvalStatus::IllegalOperation;
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (cols < 0) return EvalStatus::IllegalOperation;
      int32_t size = rows * cols;
      if (stack.size() < size) return EvalStatus::StackUnderflow;
      std::span<float> m = peekv(size);
      std::vector<float> t(size);
      for (int32_t i = 0; i < rows; ++i) {
        for (int32_t j = 0; j < cols; ++j) {
          t[rows*j+i] = m[cols*i+j];
        }
      }
      memcpy_it(m.begin(), t.begin(), size);
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
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size + 1);
      std::span<float> coeff(data.data() + 1, size);
      float t = data[0];
      float p = 1;
      float result = 0;
      for (int32_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      stack.resize(stack.size() - size);
      stack.back() = result;
      break;
    }
    case Operation::PolyMat: {
      if (i.size() < 2) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      if (rows < 0) return EvalStatus::IllegalOperation;
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (cols < 0) return EvalStatus::IllegalOperation;
      int32_t size = rows * cols;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size + 1);
      std::span<float> coeff(data.data() + 1, size);
      std::span<float> result(data.begin(), cols);
      float t = data[0];
      for (int32_t j = 0; j < cols; ++j) {
        float r = 0;
        float p = 1;
        for (int32_t i = 0; i < rows; ++i) {
          r += coeff[cols*i+j] * p;
          p *= t;
        }
        result[j] = r;
      }
      stack.resize(stack.size() - (size + 1) + cols);
      break;
    }
    case Operation::AddVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size*2);
      std::span<float> lhs(data.data(), size);
      std::span<float> rhs(data.data() + size, size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      stack.resize(stack.size() - size);
      break;
    }
    case Operation::SubVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size*2);
      std::span<float> lhs(data.data(), size);
      std::span<float> rhs(data.data() + size, size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      stack.resize(stack.size() - size);
      break;
    }
    case Operation::MulVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size*2) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size*2);
      std::span<float> lhs(data.data(), size);
      std::span<float> rhs(data.data() + size, size);
      for (int32_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      stack.resize(stack.size() - size);
      break;
    }
    case Operation::ScaleVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size + 1);
      std::span<float> vec(data.data() + 1, size);
      std::span<float> result(data.data(), size);
      float scalar = data[0];
      for (int32_t i = 0; i < size; ++i) {
        result[i] = scalar * vec[i];
      }
      stack.pop_back();
      break;
    }
    case Operation::NegVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size) return EvalStatus::StackUnderflow;
      std::span<float> vec = peekv(size);
      for (int32_t i = 0; i < size; ++i) {
        vec[i] = -vec[i];
      }
      break;
    }
    case Operation::NormVec: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      auto [size, status] = implicitPushArg(1, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size) return EvalStatus::StackUnderflow;
      std::span<float> vec = peekv(size);
      float result = 0;
      for (int32_t i = 0; i < size; ++i) {
        result += vec[i] * vec[i];
      }
      stack.resize(stack.size() - size + 1);
      stack.back() = std::sqrt(result);
      break;
    }
    case Operation::Lerp: {
      if (i.size() < 1) return EvalStatus::IntLiteralsUnderflow;
      int32_t size = popi();
      if (size < 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size*2+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size * 2 + 1);
      std::span<float> v0(data.data() + 1, size);
      std::span<float> v1(data.data() + 1 + size, size);
      std::span<float> result(data.begin(), size);
      float t = data[0];
      for (int32_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      stack.resize(stack.size() - (size*2+1) + size);
      break;
    }
    case Operation::Lut: {
      if (i.size() < 2) return EvalStatus::IntLiteralsUnderflow;
      int32_t rows = popi();
      auto [cols, status] = implicitPushArg(rows, popi(), stack, f);
      if (status != EvalStatus::Ok) return status;
      int32_t size = rows * cols;
      if (size <= 0) return EvalStatus::IllegalOperation;
      if (stack.size() < size+1) return EvalStatus::StackUnderflow;
      std::span<float> data = peekv(size+1);
      std::span<float> lut(data.data() + 1, size);
      float t = data[0];
      int32_t n = cols - 1;
      size_t ubrow = search(rows, [t, cols, lut](size_t i) -> bool {
        return t < lut[cols*i];
      });
      if (ubrow == 0) {
        auto bound = lut.begin();
        std::copy(bound + 1, bound + cols, stack.end() - (size + 1));
        stack.resize(stack.size() - (size + 1) + n);
      } else if (ubrow == rows) {
        auto bound = lut.end() - cols;
        std::copy(bound + 1, bound + cols, stack.end() - (size + 1));
        stack.resize(stack.size() - (size + 1) + n);
      } else {
        auto ub = lut.begin() + ubrow*cols;
        auto lb = ub - cols;
        float t0 = *lb;
        float t1 = *ub;
        t = (t - t0) / (t1 - t0);
        std::span<const float> v0(lb + 1, n);
        std::span<const float> v1(ub + 1, n);
        std::span<float> result(data.begin(), n);
        for (int32_t i = 0; i < result.size(); ++i) {
          result[i] = (1-t)*v0[i] + t*v1[i];
        }
        stack.resize(stack.size() - (size + 1) + n);
      }
      break;
    }
    default:
      return EvalStatus::UndefinedOperation;
    }
  }
  return EvalStatus::Ok;
}

}