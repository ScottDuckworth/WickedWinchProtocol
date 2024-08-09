#include <WickedWinchProtocol/Postfix.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>
#include <vector>

namespace wickedwinch::protocol {
namespace {

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

#define CHECK_STATUS(expr) if (EvalStatus status = expr; status != EvalStatus::Ok) return status

EvalStatus PostfixEvalContext::push(float v) {
  if (stack_size + 1 > stack_capacity) return EvalStatus::StackOverflow;
  stack_data[stack_size++] = v;
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::pushv(std::span<const float> v) {
  if (stack_size + v.size() > stack_capacity) return EvalStatus::StackOverflow;
  memcpy(&stack_data[stack_size], v.data(), v.size() * sizeof(float));
  stack_size += v.size();
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::pushf(uint16_t n) {
  if (n > f_size) return EvalStatus::FloatLiteralsUnderflow;
  CHECK_STATUS(pushv(std::span<const float>(f_head, n)));
  f_size -= n;
  f_head += n;
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::implicitPushArg(uint8_t& arg, uint8_t multiple, uint8_t instances) {
  uint8_t mask = uint8_t(1 << instances) - 1;
  uint8_t push_count = arg & mask;
  arg >>= instances;
  if (push_count == 0) return EvalStatus::Ok;
  uint16_t size = uint16_t(push_count * multiple * arg);
  return pushf(size);
}

EvalStatus PostfixEvalContext::allocv(size_t n, std::span<float>& v) {
  if (stack_size + n > stack_capacity) return EvalStatus::StackOverflow;
  v = std::span<float>(&stack_data[stack_size], n);
  stack_size += n;
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::pop(float& v) {
  if (stack_size < 1) return EvalStatus::StackUnderflow;
  v = stack_data[--stack_size];
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::popv(size_t n, std::span<float>& v) {
  if (stack_size < n) return EvalStatus::StackUnderflow;
  stack_size -= n;
  v = std::span<float>(&stack_data[stack_size], n);
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::peek(float& v) {
  if (stack_size < 1) return EvalStatus::StackUnderflow;
  v = stack_data[stack_size-1];
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::peekv(size_t n, std::span<float>& v) {
  if (stack_size < n) return EvalStatus::StackUnderflow;
  v = std::span<float>(&stack_data[stack_size - n], n);
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::geti(uint8_t& n) {
  if (i_size < 1) return EvalStatus::IntLiteralsUnderflow;
  n = i_head[0];
  --i_size;
  ++i_head;
  return EvalStatus::Ok;
}

EvalStatus PostfixEvalContext::Eval() {
  for (uint8_t opi = 0; opi < op_size; ++opi) {
    const PostfixOp op = op_head[opi];
    switch (op) {
    case PostfixOp::Push: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      CHECK_STATUS(pushf(n));
      break;
    }
    case PostfixOp::Pop: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      std::span<float> discard;
      CHECK_STATUS(popv(n, discard));
      break;
    }
    case PostfixOp::Dup: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      if (stack_size - 1 < n) return EvalStatus::StackUnderflow;
      float v = stack_data[stack_size - 1 - n];
      CHECK_STATUS(push(v));
      break;
    }
    case PostfixOp::RotL: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(peekv(n, values));
      float l = values[0];
      std::copy(values.begin() + 1, values.end(), values.begin());
      values.back() = l;
      break;
    }
    case PostfixOp::RotR: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(peekv(n, values));
      float r = values.back();
      std::copy(values.begin(), values.end() - 1, values.begin() + 1);
      values[0] = r;
      break;
    }
    case PostfixOp::Rev: {
      uint8_t n;
      CHECK_STATUS(geti(n));
      std::span<float> values;
      CHECK_STATUS(peekv(n, values));
      std::reverse(values.begin(), values.end());
      break;
    }
    case PostfixOp::Transpose: {
      uint8_t rows, cols;
      CHECK_STATUS(geti(rows));
      CHECK_STATUS(geti(cols));
      CHECK_STATUS(implicitPushArg(cols, rows, 1));

      std::span<float> m;
      CHECK_STATUS(popv(rows * cols, m));
      temp.resize(m.size());
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          size_t midx = cols * i + j;
          size_t tidx = rows * j + i;
          temp[tidx] = m[midx];
        }
      }
      CHECK_STATUS(pushv(temp));
      break;
    }
    case PostfixOp::Add: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(v[0] + v[1]));
      break;
    }
    case PostfixOp::Sub: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(v[0] - v[1]));
      break;
    }
    case PostfixOp::Mul: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(v[0] * v[1]));
      break;
    }
    case PostfixOp::MulAdd: {
      std::span<float> v;
      CHECK_STATUS(popv(3, v));
      CHECK_STATUS(push(v[0] * v[1] + v[2]));
      break;
    }
    case PostfixOp::Div: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(v[0] / v[1]));
      break;
    }
    case PostfixOp::Mod: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(std::fmod(v[0], v[1])));
      break;
    }
    case PostfixOp::Neg: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(-v));
      break;
    }
    case PostfixOp::Abs: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::abs(v)));
      break;
    }
    case PostfixOp::Inv: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(1.0f / v));
      break;
    }
    case PostfixOp::Pow: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(std::pow(v[0], v[1])));
      break;
    }
    case PostfixOp::Sqrt: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::sqrt(v)));
      break;
    }
    case PostfixOp::Exp: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::exp(v)));
      break;
    }
    case PostfixOp::Ln: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::log(v)));
      break;
    }
    case PostfixOp::Sin: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::sin(v)));
      break;
    }
    case PostfixOp::Cos: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::cos(v)));
      break;
    }
    case PostfixOp::Tan: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::tan(v)));
      break;
    }
    case PostfixOp::Asin: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::asin(v)));
      break;
    }
    case PostfixOp::Acos: {
      float v;
      CHECK_STATUS(pop(v));
      CHECK_STATUS(push(std::acos(v)));
      break;
    }
    case PostfixOp::Atan2: {
      std::span<float> v;
      CHECK_STATUS(popv(2, v));
      CHECK_STATUS(push(std::atan2(v[0], v[1])));
      break;
    }
    case PostfixOp::PolyVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      float result = 0;
      float p = 1;
      float t;
      std::span<float> coeff;
      CHECK_STATUS(popv(size, coeff));
      CHECK_STATUS(pop(t));
      for (uint8_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      CHECK_STATUS(push(result));
      break;
    }
    case PostfixOp::PolyMat: {
      uint8_t rows, cols;
      CHECK_STATUS(geti(rows));
      CHECK_STATUS(geti(cols));
      CHECK_STATUS(implicitPushArg(cols, rows, 1));

      float t;
      std::span<float> coeff, result;
      CHECK_STATUS(popv(rows * cols, coeff));
      CHECK_STATUS(pop(t));
      CHECK_STATUS(allocv(cols, result));
      for (uint8_t j = 0; j < cols; ++j) {
        float r = 0;
        float p = 1;
        for (uint8_t i = 0; i < rows; ++i) {
          size_t cidx = cols * i + j;
          r += coeff[cidx] * p;
          p *= t;
        }
        result[j] = r;
      }
      break;
    }
    case PostfixOp::AddVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(popv(size, rhs));
      CHECK_STATUS(peekv(size, lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case PostfixOp::SubVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(popv(size, rhs));
      CHECK_STATUS(peekv(size, lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case PostfixOp::MulVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(popv(size, rhs));
      CHECK_STATUS(peekv(size, lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case PostfixOp::MulAddVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 2));

      std::span<float> a, b, c;
      CHECK_STATUS(popv(size, c));
      CHECK_STATUS(popv(size, b));
      CHECK_STATUS(peekv(size, a));
      for (uint8_t i = 0; i < size; ++i) {
        a[i] = a[i] * b[i] + c[i];
      }
      break;
    }
    case PostfixOp::ScaleVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      float scalar;
      std::span<float> v, result;
      CHECK_STATUS(popv(size, v));
      CHECK_STATUS(pop(scalar));
      CHECK_STATUS(allocv(size, result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = scalar * v[i];
      }
      break;
    }
    case PostfixOp::NegVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      std::span<float> v;
      CHECK_STATUS(peekv(size, v));
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case PostfixOp::NormVec: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 1));

      std::span<float> v;
      CHECK_STATUS(popv(size, v));
      float result = 0;
      for (uint8_t i = 0; i < size; ++i) {
        result += v[i] * v[i];
      }
      CHECK_STATUS(push(std::sqrt(result)));
      break;
    }
    case PostfixOp::MulMat: {
      uint8_t arows, brows, bcols;
      CHECK_STATUS(geti(arows));
      CHECK_STATUS(geti(brows));
      CHECK_STATUS(geti(bcols));
      CHECK_STATUS(implicitPushArg(bcols, brows, 1));

      std::span<float> a, b;
      CHECK_STATUS(popv(brows * bcols, b));
      CHECK_STATUS(popv(arows * brows, a));
      temp.resize(arows * bcols);
      for (uint8_t i = 0; i < arows; ++i) {
        for (uint8_t j = 0; j < bcols; ++j) {
          float r = 0;
          for (uint8_t k = 0; k < brows; ++k) {
            size_t aidx = brows * i + k;
            size_t bidx = bcols * k + j;
            r += a[aidx] * b[bidx];
          }
          size_t cidx = bcols * i + j;
          temp[cidx] = r;
        }
      }
      CHECK_STATUS(pushv(temp));
      break;
    }
    case PostfixOp::Lerp: {
      uint8_t size;
      CHECK_STATUS(geti(size));
      CHECK_STATUS(implicitPushArg(size, 1, 2));

      float t;
      std::span<float> v0, v1, result;
      CHECK_STATUS(popv(size, v1));
      CHECK_STATUS(popv(size, v0));
      CHECK_STATUS(pop(t));
      CHECK_STATUS(allocv(size, result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      break;
    }
    case PostfixOp::Lut: {
      uint8_t rows, cols;
      CHECK_STATUS(geti(rows));
      CHECK_STATUS(geti(cols));
      CHECK_STATUS(implicitPushArg(cols, rows, 1));
      if (rows < 1) return EvalStatus::IllegalOperation;
      if (cols < 1) return EvalStatus::IllegalOperation;

      float t;
      std::span<float> lut, result;
      size_t size = rows * cols;
      uint8_t n = cols - 1;
      CHECK_STATUS(popv(size, lut));
      CHECK_STATUS(pop(t));
      CHECK_STATUS(allocv(n, result));
      size_t ubrow = search(rows, [t, cols, lut](size_t i) -> bool {
        return t < lut[cols*i];
      });
      if (ubrow == 0) {
        auto bound = lut.begin();
        std::copy(bound + 1, bound + cols, result.begin());
      } else if (ubrow == rows) {
        auto bound = lut.end() - cols;
        std::copy(bound + 1, bound + cols, result.begin());
      } else {
        auto ub = lut.begin() + ubrow*cols;
        auto lb = ub - cols;
        float t0 = *lb;
        float t1 = *ub;
        t = (t - t0) / (t1 - t0);
        std::span<const float> v0(lb + 1, n);
        std::span<const float> v1(ub + 1, n);
        for (size_t i = 0; i < result.size(); ++i) {
          result[i] = (1-t)*v0[i] + t*v1[i];
        }
      }
      break;
    }
    default:
      return EvalStatus::UndefinedOperation;
    }
  }
  return EvalStatus::Ok;
}

bool PostfixReader::Read(const uint8_t* data, size_t size) {
  buffer_ = data;
  if (size < sizeof(PostfixHeader)) return false;
  return size >= data_size();
}

bool PostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;

  auto* header = reinterpret_cast<PostfixHeader*>(data);
  header->op_size = op_size();
  header->i_size = i_size();
  header->f_size = f_size();

  auto* op = reinterpret_cast<PostfixOp*>(data + op_offset());
  memcpy(op, op_data(), op_size());

  auto* i = data + i_offset();
  memcpy(i, i_data(), i_size());

  auto* f = reinterpret_cast<float*>(data + f_offset());
  memcpy(f, f_data(), f_size() * sizeof(float));

  return true;
}

}
