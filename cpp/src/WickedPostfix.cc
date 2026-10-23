#include "WickedPostfix.h"
#include "WickedUtil.hh"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4);
static_assert(sizeof(WickedPostfixHeader) == 2);

extern "C" const char* WickedPostfixOpToString(WickedPostfixOp_t op) {
  switch (op) {
    case WickedPostfixOp_Undefined: return "Undefined";
    case WickedPostfixOp_Push:      return "Push";
    case WickedPostfixOp_Pop:       return "Pop";
    case WickedPostfixOp_Dup:       return "Dup";
    case WickedPostfixOp_RotL:      return "RotL";
    case WickedPostfixOp_RotR:      return "RotR";
    case WickedPostfixOp_Rev:       return "Rev";
    case WickedPostfixOp_Transpose: return "Transpose";
    case WickedPostfixOp_Add:       return "Add";
    case WickedPostfixOp_Sub:       return "Sub";
    case WickedPostfixOp_Mul:       return "Mul";
    case WickedPostfixOp_MulAdd:    return "MulAdd";
    case WickedPostfixOp_Div:       return "Div";
    case WickedPostfixOp_Mod:       return "Mod";
    case WickedPostfixOp_Neg:       return "Neg";
    case WickedPostfixOp_Abs:       return "Abs";
    case WickedPostfixOp_Inv:       return "Inv";
    case WickedPostfixOp_Pow:       return "Pow";
    case WickedPostfixOp_Sqrt:      return "Sqrt";
    case WickedPostfixOp_Exp:       return "Exp";
    case WickedPostfixOp_Ln:        return "Ln";
    case WickedPostfixOp_Sin:       return "Sin";
    case WickedPostfixOp_Cos:       return "Cos";
    case WickedPostfixOp_Tan:       return "Tan";
    case WickedPostfixOp_Asin:      return "Asin";
    case WickedPostfixOp_Acos:      return "Acos";
    case WickedPostfixOp_Atan2:     return "Atan2";
    case WickedPostfixOp_AddVec:    return "AddVec";
    case WickedPostfixOp_SubVec:    return "SubVec";
    case WickedPostfixOp_MulVec:    return "MulVec";
    case WickedPostfixOp_MulAddVec: return "MulAddVec";
    case WickedPostfixOp_ScaleVec:  return "ScaleVec";
    case WickedPostfixOp_NegVec:    return "NegVec";
    case WickedPostfixOp_NormVec:   return "NormVec";
    case WickedPostfixOp_MulMat:    return "MulMat";
    case WickedPostfixOp_PolyVec:   return "PolyVec";
    case WickedPostfixOp_PolyMat:   return "PolyMat";
    case WickedPostfixOp_LerpVec:   return "LerpVec";
    case WickedPostfixOp_LerpTable: return "LerpTable";
  }
  return "UNKNOWN";
}

extern "C" bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval) {
  if (size < sizeof(WickedPostfixHeader)) return false;

  const auto* header = reinterpret_cast<const WickedPostfixHeader*>(data);
  uint16_t prog_size = header->prog_size;
  constexpr size_t prog_offset = sizeof(WickedPostfixHeader);
  size_t total_size = prog_offset + prog_size;
  if (size != total_size) return false;

  if (eval) {
    eval->prog_data = data + prog_offset;
    eval->prog_size = prog_size;
    eval->pc = 0;
  }
  return true;
}

extern "C" void WickedPostfixEval_reset(WickedPostfixEval_t* eval) {
  eval->stack_size = 0;
  eval->pc = 0;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_push(WickedPostfixEval_t* eval, float v) {
  if (eval->stack_capacity - eval->stack_size < 1) return WickedEvalStatus_StackOverflow;
  eval->stack_data[eval->stack_size++] = v;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushv(WickedPostfixEval_t* eval, const float* v, size_t size) {
  if (eval->stack_capacity - eval->stack_size < size) return WickedEvalStatus_StackOverflow;
  memcpy(&eval->stack_data[eval->stack_size], v, size * sizeof(float));
  eval->stack_size += static_cast<uint8_t>(size);
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pop(WickedPostfixEval_t* eval, float* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[--eval->stack_size];
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popv(WickedPostfixEval_t* eval, float* v, size_t n) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= static_cast<uint8_t>(n);
  const float* src = &eval->stack_data[eval->stack_size];
  for (size_t i = 0; i < n; ++i) v[i] = src[i];
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixValidateProgram(const uint8_t* prog_data, uint16_t prog_size, uint8_t initial_stack_size, uint8_t stack_capacity, uint8_t expected_final_stack_size) {
  if (initial_stack_size > stack_capacity) {
    return WickedEvalStatus_StackOverflow;
  }

  uint16_t pc = 0;
  int32_t stack_depth = initial_stack_size;

  while (pc < prog_size) {
    uint8_t op = prog_data[pc++];
    switch (op) {
      case WickedPostfixOp_Push: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t n = prog_data[pc++];
        uint16_t byte_count = uint16_t(n) * sizeof(float);
        if (prog_size - pc < byte_count) return WickedEvalStatus_IllegalOperation;
        pc += byte_count;
        if (stack_depth + n > stack_capacity) return WickedEvalStatus_StackOverflow;
        stack_depth += n;
        break;
      }
      case WickedPostfixOp_Pop: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t n = prog_data[pc++];
        if (stack_depth < n) return WickedEvalStatus_StackUnderflow;
        stack_depth -= n;
        break;
      }
      case WickedPostfixOp_Dup: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t n = prog_data[pc++];
        if (stack_depth < int32_t(n) + 1) return WickedEvalStatus_StackUnderflow;
        if (stack_depth + 1 > stack_capacity) return WickedEvalStatus_StackOverflow;
        stack_depth += 1;
        break;
      }
      case WickedPostfixOp_RotL:
      case WickedPostfixOp_RotR:
      case WickedPostfixOp_Rev: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t n = prog_data[pc++];
        if (stack_depth < n) return WickedEvalStatus_StackUnderflow;
        break;
      }
      case WickedPostfixOp_Transpose: {
        if (prog_size - pc < 2) return WickedEvalStatus_IllegalOperation;
        uint8_t rows = prog_data[pc++];
        uint8_t cols = prog_data[pc++];
        uint16_t size = uint16_t(rows) * cols;
        if (stack_depth < size) return WickedEvalStatus_StackUnderflow;
        if (rows > 1 && cols > 1 && rows != cols) {
          if (stack_depth + size > stack_capacity) return WickedEvalStatus_StackOverflow;
        }
        break;
      }
      case WickedPostfixOp_Add:
      case WickedPostfixOp_Sub:
      case WickedPostfixOp_Mul:
      case WickedPostfixOp_Div:
      case WickedPostfixOp_Mod:
      case WickedPostfixOp_Pow:
      case WickedPostfixOp_Atan2: {
        if (stack_depth < 2) return WickedEvalStatus_StackUnderflow;
        stack_depth -= 1;
        break;
      }
      case WickedPostfixOp_MulAdd: {
        if (stack_depth < 3) return WickedEvalStatus_StackUnderflow;
        stack_depth -= 2;
        break;
      }
      case WickedPostfixOp_Neg:
      case WickedPostfixOp_Abs:
      case WickedPostfixOp_Inv:
      case WickedPostfixOp_Sqrt:
      case WickedPostfixOp_Exp:
      case WickedPostfixOp_Ln:
      case WickedPostfixOp_Sin:
      case WickedPostfixOp_Cos:
      case WickedPostfixOp_Tan:
      case WickedPostfixOp_Asin:
      case WickedPostfixOp_Acos: {
        if (stack_depth < 1) return WickedEvalStatus_StackUnderflow;
        break;
      }
      case WickedPostfixOp_AddVec:
      case WickedPostfixOp_SubVec:
      case WickedPostfixOp_MulVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < int32_t(size) * 2) return WickedEvalStatus_StackUnderflow;
        stack_depth -= size;
        break;
      }
      case WickedPostfixOp_MulAddVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < int32_t(size) * 3) return WickedEvalStatus_StackUnderflow;
        stack_depth -= int32_t(size) * 2;
        break;
      }
      case WickedPostfixOp_ScaleVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < int32_t(size) + 1) return WickedEvalStatus_StackUnderflow;
        stack_depth -= 1;
        break;
      }
      case WickedPostfixOp_NegVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < size) return WickedEvalStatus_StackUnderflow;
        break;
      }
      case WickedPostfixOp_NormVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < size) return WickedEvalStatus_StackUnderflow;
        if (size == 0) {
          if (stack_depth + 1 > stack_capacity) return WickedEvalStatus_StackOverflow;
          stack_depth += 1;
        } else {
          stack_depth -= (size - 1);
        }
        break;
      }
      case WickedPostfixOp_MulMat: {
        if (prog_size - pc < 3) return WickedEvalStatus_IllegalOperation;
        uint8_t arows = prog_data[pc++];
        uint8_t brows = prog_data[pc++];
        uint8_t bcols = prog_data[pc++];
        uint16_t asize = uint16_t(arows) * brows;
        uint16_t bsize = uint16_t(brows) * bcols;
        uint16_t csize = uint16_t(arows) * bcols;
        if (stack_depth < asize + bsize) return WickedEvalStatus_StackUnderflow;
        if (stack_depth + csize > stack_capacity) return WickedEvalStatus_StackOverflow;
        stack_depth = stack_depth - asize - bsize + csize;
        break;
      }
      case WickedPostfixOp_PolyVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < int32_t(size) + 1) return WickedEvalStatus_StackUnderflow;
        stack_depth -= size;
        break;
      }
      case WickedPostfixOp_PolyMat: {
        if (prog_size - pc < 2) return WickedEvalStatus_IllegalOperation;
        uint8_t rows = prog_data[pc++];
        uint8_t cols = prog_data[pc++];
        uint16_t coeff_size = uint16_t(rows) * cols;
        if (stack_depth < int32_t(coeff_size) + 1) return WickedEvalStatus_StackUnderflow;
        if (stack_depth - (int32_t(coeff_size) + 1) + cols > stack_capacity) return WickedEvalStatus_StackOverflow;
        stack_depth = stack_depth - (int32_t(coeff_size) + 1) + cols;
        break;
      }
      case WickedPostfixOp_LerpVec: {
        if (pc >= prog_size) return WickedEvalStatus_IllegalOperation;
        uint8_t size = prog_data[pc++];
        if (stack_depth < int32_t(size) * 2 + 1) return WickedEvalStatus_StackUnderflow;
        stack_depth -= (int32_t(size) + 1);
        break;
      }
      case WickedPostfixOp_LerpTable: {
        if (prog_size - pc < 2) return WickedEvalStatus_IllegalOperation;
        uint8_t rows = prog_data[pc++];
        uint8_t cols = prog_data[pc++];
        if (rows < 1 || cols < 1) return WickedEvalStatus_IllegalOperation;
        uint16_t lut_size = uint16_t(rows) * cols;
        if (stack_depth < int32_t(lut_size) + 1) return WickedEvalStatus_StackUnderflow;
        if (stack_depth - (int32_t(lut_size) + 1) + (cols - 1) > stack_capacity) return WickedEvalStatus_StackOverflow;
        stack_depth = stack_depth - (int32_t(lut_size) + 1) + (cols - 1);
        break;
      }
      default:
        return WickedEvalStatus_UndefinedOperation;
    }
  }
  if (stack_depth != expected_final_stack_size) {
    return WickedEvalStatus_UnexpectedStackSize;
  }
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixValidate(const WickedPostfixEval_t* eval, uint8_t expected_final_stack_size) {
  if (!eval) return WickedEvalStatus_IllegalOperation;
  return WickedPostfixValidateProgram(eval->prog_data, eval->prog_size, eval->stack_size, eval->stack_capacity, expected_final_stack_size);
}

extern "C" void WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  const uint8_t* pc = eval->prog_data;
  const uint8_t* const pc_end = eval->prog_data + eval->prog_size;
  float* sp = eval->stack_data + eval->stack_size;

  auto loadp = [&pc]() -> uint8_t {
    return *pc++;
  };

  auto loadpv = [&pc](uint16_t n) -> const uint8_t* {
    const uint8_t* p = pc;
    pc += n;
    return p;
  };

  auto push = [&sp](float v) {
    *sp++ = v;
  };

  auto pushv = [&sp](const float* v, size_t n) {
    std::memcpy(sp, v, n * sizeof(float));
    sp += n;
  };

  auto pop = [&sp]() -> float {
    return *--sp;
  };

  auto popv = [&sp](uint8_t n) -> std::span<float> {
    sp -= n;
    return std::span<float>(sp, n);
  };

  auto peek = [&sp]() -> float& {
    return sp[-1];
  };

  auto peekv = [&sp](uint8_t n) -> std::span<float> {
    return std::span<float>(sp - n, n);
  };

  auto allocv = [&sp](uint8_t n) -> std::span<float> {
    std::span<float> s(sp, n);
    sp += n;
    return s;
  };

  while (pc < pc_end) {
    const uint8_t op = *pc++;
    switch (op) {
    case WickedPostfixOp_Push: {
      uint8_t n = loadp();
      const uint8_t* v = loadpv(uint16_t(n) * sizeof(float));
      pushv(reinterpret_cast<const float*>(v), n);
      break;
    }
    case WickedPostfixOp_Pop: {
      uint8_t n = loadp();
      sp -= n;
      break;
    }
    case WickedPostfixOp_Dup: {
      uint8_t n = loadp();
      float v = sp[-1 - n];
      push(v);
      break;
    }
    case WickedPostfixOp_RotL: {
      uint8_t n = loadp();
      if (n > 1) {
        std::span<float> values = peekv(n);
        float l = values[0];
        std::copy(values.begin() + 1, values.end(), values.begin());
        values.back() = l;
      }
      break;
    }
    case WickedPostfixOp_RotR: {
      uint8_t n = loadp();
      if (n > 1) {
        std::span<float> values = peekv(n);
        float r = values.back();
        std::copy_backward(values.begin(), values.end() - 1, values.end());
        values.front() = r;
      }
      break;
    }
    case WickedPostfixOp_Rev: {
      uint8_t n = loadp();
      if (n > 1) {
        std::span<float> values = peekv(n);
        std::reverse(values.begin(), values.end());
      }
      break;
    }
    case WickedPostfixOp_Transpose: {
      uint8_t rows = loadp();
      uint8_t cols = loadp();
      uint16_t size = uint16_t(rows) * cols;
      std::span<float> m = peekv(size);
      if (rows > 1 && cols > 1) {
        if (rows == cols) {
          for (uint8_t i = 0; i < rows; ++i) {
            for (uint8_t j = i + 1; j < cols; ++j) {
              std::swap(m[i * cols + j], m[j * cols + i]);
            }
          }
        } else {
          float* out = sp;
          for (uint8_t i = 0; i < rows; ++i) {
            for (uint8_t j = 0; j < cols; ++j) {
              out[rows * j + i] = m[cols * i + j];
            }
          }
          std::memcpy(m.data(), out, size * sizeof(float));
        }
      }
      break;
    }
    case WickedPostfixOp_Add: {
      float b = pop();
      peek() += b;
      break;
    }
    case WickedPostfixOp_Sub: {
      float b = pop();
      peek() -= b;
      break;
    }
    case WickedPostfixOp_Mul: {
      float b = pop();
      peek() *= b;
      break;
    }
    case WickedPostfixOp_MulAdd: {
      float c = pop();
      float b = pop();
      float& top = peek();
      top = top * b + c;
      break;
    }
    case WickedPostfixOp_Div: {
      float b = pop();
      peek() /= b;
      break;
    }
    case WickedPostfixOp_Mod: {
      float b = pop();
      peek() = std::fmod(peek(), b);
      break;
    }
    case WickedPostfixOp_Neg: {
      peek() = -peek();
      break;
    }
    case WickedPostfixOp_Abs: {
      peek() = std::abs(peek());
      break;
    }
    case WickedPostfixOp_Inv: {
      peek() = 1.0f / peek();
      break;
    }
    case WickedPostfixOp_Pow: {
      float b = pop();
      peek() = std::pow(peek(), b);
      break;
    }
    case WickedPostfixOp_Sqrt: {
      peek() = std::sqrt(peek());
      break;
    }
    case WickedPostfixOp_Exp: {
      peek() = std::exp(peek());
      break;
    }
    case WickedPostfixOp_Ln: {
      peek() = std::log(peek());
      break;
    }
    case WickedPostfixOp_Sin: {
      peek() = std::sin(peek());
      break;
    }
    case WickedPostfixOp_Cos: {
      peek() = std::cos(peek());
      break;
    }
    case WickedPostfixOp_Tan: {
      peek() = std::tan(peek());
      break;
    }
    case WickedPostfixOp_Asin: {
      peek() = std::asin(peek());
      break;
    }
    case WickedPostfixOp_Acos: {
      peek() = std::acos(peek());
      break;
    }
    case WickedPostfixOp_Atan2: {
      float x = pop();
      peek() = std::atan2(peek(), x);
      break;
    }
    case WickedPostfixOp_PolyVec: {
      uint8_t size = loadp();
      std::span<float> coeff = popv(size);
      float t = pop();
      float result = 0;
      float p = 1;
      for (uint8_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      push(result);
      break;
    }
    case WickedPostfixOp_PolyMat: {
      uint8_t rows = loadp();
      uint8_t cols = loadp();
      std::span<float> coeff = popv(rows * cols);
      float t = pop();
      std::span<float> result = allocv(cols);
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
    case WickedPostfixOp_AddVec: {
      uint8_t size = loadp();
      std::span<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case WickedPostfixOp_SubVec: {
      uint8_t size = loadp();
      std::span<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulVec: {
      uint8_t size = loadp();
      std::span<float> rhs = popv(size);
      std::span<float> lhs = peekv(size);
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulAddVec: {
      uint8_t size = loadp();
      std::span<float> c = popv(size);
      std::span<float> b = popv(size);
      std::span<float> a = peekv(size);
      for (uint8_t i = 0; i < size; ++i) {
        a[i] = a[i] * b[i] + c[i];
      }
      break;
    }
    case WickedPostfixOp_ScaleVec: {
      uint8_t size = loadp();
      std::span<float> v = popv(size);
      float scalar = pop();
      std::span<float> result = allocv(size);
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = scalar * v[i];
      }
      break;
    }
    case WickedPostfixOp_NegVec: {
      uint8_t size = loadp();
      std::span<float> v = peekv(size);
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case WickedPostfixOp_NormVec: {
      uint8_t size = loadp();
      std::span<float> v = popv(size);
      float result = 0;
      for (uint8_t i = 0; i < size; ++i) {
        result += v[i] * v[i];
      }
      push(std::sqrt(result));
      break;
    }
    case WickedPostfixOp_MulMat: {
      uint8_t arows = loadp();
      uint8_t brows = loadp();
      uint8_t bcols = loadp();

      uint16_t asize = uint16_t(arows) * brows;
      uint16_t bsize = uint16_t(brows) * bcols;
      uint16_t csize = uint16_t(arows) * bcols;

      std::span<float> b = popv(bsize);
      std::span<float> a = popv(asize);

      float* c = sp + asize + bsize;

      for (uint8_t i = 0; i < arows; ++i) {
        for (uint8_t j = 0; j < bcols; ++j) {
          float r = 0;
          for (uint8_t k = 0; k < brows; ++k) {
            r += a[brows * i + k] * b[bcols * k + j];
          }
          size_t cidx = bcols * i + j;
          c[cidx] = r;
        }
      }

      float* dest = a.data();
      std::memmove(dest, c, csize * sizeof(float));
      sp += csize;
      break;
    }
    case WickedPostfixOp_LerpVec: {
      uint8_t size = loadp();
      std::span<float> v1 = popv(size);
      std::span<float> v0 = popv(size);
      float t = pop();
      std::span<float> result = allocv(size);
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = (1.0f - t) * v0[i] + t * v1[i];
      }
      break;
    }
    case WickedPostfixOp_LerpTable: {
      uint8_t rows = loadp();
      uint8_t cols = loadp();
      uint8_t size = rows * cols;
      uint8_t n = cols - 1;
      std::span<float> lut = popv(size);
      float t = pop();
      std::span<float> result = allocv(n);
      uint8_t ubrow = wicked_upper_bound(uint8_t(0), rows, [t, cols, lut](uint8_t i) -> bool {
        return t < lut[cols * i];
      });
      if (ubrow == 0) {
        auto bound = lut.begin();
        std::copy(bound + 1, bound + cols, result.begin());
      } else if (ubrow == rows) {
        auto bound = lut.end() - cols;
        std::copy(bound + 1, bound + cols, result.begin());
      } else {
        auto ub = lut.begin() + ubrow * cols;
        auto lb = ub - cols;
        float t0 = *lb;
        float t1 = *ub;
        t = (t - t0) / (t1 - t0);
        std::span<const float> v0(lb + 1, n);
        std::span<const float> v1(ub + 1, n);
        for (size_t i = 0; i < result.size(); ++i) {
          result[i] = (1.0f - t) * v0[i] + t * v1[i];
        }
      }
      break;
    }
    default:
      break;
    }
  }

  eval->pc = static_cast<uint16_t>(pc - eval->prog_data);
  eval->stack_size = static_cast<uint8_t>(sp - eval->stack_data);
}

bool WickedPostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;

  auto* header = reinterpret_cast<WickedPostfixHeader*>(data);
  header->prog_size = prog_size();
  memcpy(data + prog_offset(), prog_data(), prog_size());

  return true;
}
