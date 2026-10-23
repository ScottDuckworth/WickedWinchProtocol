#include "WickedPostfix.h"
#include "WickedUtil.hh"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>

#define CHECK_STATUS(expr) if (WickedEvalStatus_t status = expr; status != WickedEvalStatus_Ok) return status

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
    case WickedPostfixOp_Lerp:      return "Lerp";
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

static WickedEvalStatus_t WickedPostfixEval_pushv(WickedPostfixEval_t* eval, std::span<const float> v) {
  return WickedPostfixEval_pushv(eval, v.data(), v.size());
}

static WickedEvalStatus_t WickedPostfixEval_allocv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size + n > eval->stack_capacity) return WickedEvalStatus_StackOverflow;
  *v = std::span<float>(&eval->stack_data[eval->stack_size], n);
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pop(WickedPostfixEval_t* eval, float* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[--eval->stack_size];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_popv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= n;
  *v = std::span<float>(&eval->stack_data[eval->stack_size], n);
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popv(WickedPostfixEval_t* eval, float* v, size_t n) {
  std::span<float> src;
  WickedEvalStatus_t status = WickedPostfixEval_popv(eval, static_cast<uint8_t>(n), &src);
  if (status != WickedEvalStatus_Ok) return status;
  for (float s : src) *v++ = s;
  return status;
}

static WickedEvalStatus_t WickedPostfixEval_peek(WickedPostfixEval_t* eval, float* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[eval->stack_size-1];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_peekv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  *v = std::span<float>(&eval->stack_data[eval->stack_size - n], n);
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_loadp(WickedPostfixEval_t* eval, uint8_t* v) {
  if (eval->prog_size - eval->pc < 1) return WickedEvalStatus_IllegalOperation;
  *v = eval->prog_data[eval->pc++];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_loadpv(WickedPostfixEval_t* eval, uint16_t n, const uint8_t** v) {
  if (eval->prog_size - eval->pc < n) return WickedEvalStatus_IllegalOperation;
  *v = &eval->prog_data[eval->pc];
  eval->pc += n;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  eval->pc = 0;
  while (eval->pc < eval->prog_size) {
    const uint8_t op = eval->prog_data[eval->pc++];
    switch (op) {
    case WickedPostfixOp_Push: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      const uint8_t* v;
      uint16_t byte_count = uint16_t(n) * sizeof(float);
      CHECK_STATUS(WickedPostfixEval_loadpv(eval, byte_count, &v));
      CHECK_STATUS(WickedPostfixEval_pushv(eval, reinterpret_cast<const float*>(v), n));
      break;
    }
    case WickedPostfixOp_Pop: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      std::span<float> discard;
      CHECK_STATUS(WickedPostfixEval_popv(eval, n, &discard));
      break;
    }
    case WickedPostfixOp_Dup: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      if (eval->stack_size - 1 < n) return WickedEvalStatus_StackUnderflow;
      float v = eval->stack_data[eval->stack_size - 1 - n];
      CHECK_STATUS(WickedPostfixEval_push(eval, v));
      break;
    }
    case WickedPostfixOp_RotL: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, n, &values));
      float l = values[0];
      std::copy(values.begin() + 1, values.end(), values.begin());
      values.back() = l;
      break;
    }
    case WickedPostfixOp_RotR: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, n, &values));
      float r = values.back();
      std::copy_backward(values.begin(), values.end() - 1, values.end());
      values.front() = r;
      break;
    }
    case WickedPostfixOp_Rev: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, n, &values));
      std::reverse(values.begin(), values.end());
      break;
    }
    case WickedPostfixOp_Transpose: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &cols));
      std::span<float> m;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, rows * cols, &m));
      if (rows * cols > eval->temp_capacity) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          eval->temp_data[rows * j + i] = m[cols * i + j];
        }
      }
      for (size_t i = 0; i < m.size(); ++i) {
        m[i] = eval->temp_data[i];
      }
      break;
    }
    case WickedPostfixOp_Add: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = a + b;
      break;
    }
    case WickedPostfixOp_Sub: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = a - b;
      break;
    }
    case WickedPostfixOp_Mul: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = a * b;
      break;
    }
    case WickedPostfixOp_MulAdd: {
      float a, b, c;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &c));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = a * b + c;
      break;
    }
    case WickedPostfixOp_Div: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = a / b;
      break;
    }
    case WickedPostfixOp_Mod: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::fmod(a, b);
      break;
    }
    case WickedPostfixOp_Neg: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = -a;
      break;
    }
    case WickedPostfixOp_Abs: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::abs(a);
      break;
    }
    case WickedPostfixOp_Inv: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = 1.0f / a;
      break;
    }
    case WickedPostfixOp_Pow: {
      float a, b;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &b));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::pow(a, b);
      break;
    }
    case WickedPostfixOp_Sqrt: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::sqrt(a);
      break;
    }
    case WickedPostfixOp_Exp: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::exp(a);
      break;
    }
    case WickedPostfixOp_Ln: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::log(a);
      break;
    }
    case WickedPostfixOp_Sin: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::sin(a);
      break;
    }
    case WickedPostfixOp_Cos: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::cos(a);
      break;
    }
    case WickedPostfixOp_Tan: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::tan(a);
      break;
    }
    case WickedPostfixOp_Asin: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::asin(a);
      break;
    }
    case WickedPostfixOp_Acos: {
      float a;
      CHECK_STATUS(WickedPostfixEval_peek(eval, &a));
      eval->stack_data[eval->stack_size-1] = std::acos(a);
      break;
    }
    case WickedPostfixOp_Atan2: {
      float y, x;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &x));
      CHECK_STATUS(WickedPostfixEval_peek(eval, &y));
      eval->stack_data[eval->stack_size-1] = std::atan2(y, x);
      break;
    }
    case WickedPostfixOp_PolyVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      float result = 0;
      float p = 1;
      float t;
      std::span<float> coeff;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &coeff));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &t));
      for (uint8_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      CHECK_STATUS(WickedPostfixEval_push(eval, result));
      break;
    }
    case WickedPostfixOp_PolyMat: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &cols));

      float t;
      std::span<float> coeff, result;
      CHECK_STATUS(WickedPostfixEval_popv(eval, rows * cols, &coeff));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocv(eval, cols, &result));
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
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> rhs, lhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case WickedPostfixOp_SubVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> rhs, lhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> rhs, lhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulAddVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> a, b, c;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &c));
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &b));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &a));
      for (uint8_t i = 0; i < size; ++i) {
        a[i] = a[i] * b[i] + c[i];
      }
      break;
    }
    case WickedPostfixOp_ScaleVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      float scalar;
      std::span<float> v, result;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &v));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &scalar));
      CHECK_STATUS(WickedPostfixEval_allocv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = scalar * v[i];
      }
      break;
    }
    case WickedPostfixOp_NegVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &v));
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case WickedPostfixOp_NormVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &v));
      float result = 0;
      for (uint8_t i = 0; i < size; ++i) {
        result += v[i] * v[i];
      }
      CHECK_STATUS(WickedPostfixEval_push(eval, std::sqrt(result)));
      break;
    }
    case WickedPostfixOp_MulMat: {
      uint8_t arows, brows, bcols;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &arows));
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &brows));
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &bcols));

      std::span<float> a, b;
      CHECK_STATUS(WickedPostfixEval_popv(eval, brows * bcols, &b));
      CHECK_STATUS(WickedPostfixEval_popv(eval, arows * brows, &a));
      size_t temp_size = arows * bcols;
      if (temp_size > eval->temp_capacity) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < arows; ++i) {
        for (uint8_t j = 0; j < bcols; ++j) {
          float r = 0;
          for (uint8_t k = 0; k < brows; ++k) {
            size_t aidx = brows * i + k;
            size_t bidx = bcols * k + j;
            r += a[aidx] * b[bidx];
          }
          size_t cidx = bcols * i + j;
          eval->temp_data[cidx] = r;
        }
      }
      CHECK_STATUS(WickedPostfixEval_pushv(eval, std::span(eval->temp_data, temp_size)));
      break;
    }
    case WickedPostfixOp_Lerp: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &size));

      float t;
      std::span<float> v0, v1, result;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &v1));
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &v0));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      break;
    }
    case WickedPostfixOp_LerpTable: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_loadp(eval, &cols));
      if (rows < 1) return WickedEvalStatus_IllegalOperation;
      if (cols < 1) return WickedEvalStatus_IllegalOperation;

      float t;
      std::span<float> lut, result;
      uint8_t size = rows * cols;
      uint8_t n = cols - 1;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &lut));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocv(eval, n, &result));
      uint8_t ubrow = wicked_upper_bound(uint8_t(0), rows, [t, cols, lut](uint8_t i) -> bool {
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
      return WickedEvalStatus_UndefinedOperation;
    }
  }
  return WickedEvalStatus_Ok;
}

bool WickedPostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;

  auto* header = reinterpret_cast<WickedPostfixHeader*>(data);
  header->prog_size = prog_size();
  memcpy(data + prog_offset(), prog_data(), prog_size());

  return true;
}
