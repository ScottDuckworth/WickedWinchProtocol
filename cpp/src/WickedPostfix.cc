#include "WickedPostfix.h"
#include "WickedUtil.hh"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>

#define CHECK_STATUS(expr) if (WickedEvalStatus_t status = expr; status != WickedEvalStatus_Ok) return status

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4);
static_assert(sizeof(WickedPostfixHeader) == 4);

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
    case WickedPostfixOp_AddI:      return "AddI";
    case WickedPostfixOp_SubI:      return "SubI";
    case WickedPostfixOp_MulI:      return "MulI";
    case WickedPostfixOp_MulAddI:   return "MulAddI";
    case WickedPostfixOp_DivI:      return "DivI";
    case WickedPostfixOp_ModI:      return "ModI";
    case WickedPostfixOp_NegI:      return "NegI";
    case WickedPostfixOp_AbsI:      return "AbsI";
    case WickedPostfixOp_AddU:      return "AddU";
    case WickedPostfixOp_SubU:      return "SubU";
    case WickedPostfixOp_MulU:      return "MulU";
    case WickedPostfixOp_MulAddU:   return "MulAddU";
    case WickedPostfixOp_DivU:      return "DivU";
    case WickedPostfixOp_ModU:      return "ModU";
    case WickedPostfixOp_ItoF:      return "ItoF";
    case WickedPostfixOp_FtoI:      return "FtoI";
  }
  return "UNKNOWN";
}

extern "C" bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval) {
  if (size < sizeof(WickedPostfixHeader)) return false;

	const WickedPostfixHeader* header = reinterpret_cast<const WickedPostfixHeader*>(data);
	uint16_t p_size = header->p_size;
	uint16_t d_size = header->d_size;

	constexpr size_t i_offset = sizeof(WickedPostfixHeader);
  size_t d_offset = i_offset + p_size;
  d_offset = (d_offset + size_t(3)) & ~size_t(3);

	size_t data_size = d_offset + d_size * 4;
  if (size != data_size) return false;

  if (eval) {
    eval->p_data = data + i_offset;
    eval->p_size = p_size;
    eval->d_data = reinterpret_cast<const uint32_t*>(data + d_offset);
    eval->d_size = d_size;
  }
  return true;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushu(WickedPostfixEval_t* eval, uint32_t v) {
  if (eval->stack_capacity - eval->stack_size < 1) return WickedEvalStatus_StackOverflow;
  eval->stack_data[eval->stack_size++] = v;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushi(WickedPostfixEval_t* eval, int32_t v) {
  return WickedPostfixEval_pushu(eval, *reinterpret_cast<const uint32_t*>(&v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushf(WickedPostfixEval_t* eval, float v) {
  return WickedPostfixEval_pushu(eval, *reinterpret_cast<const uint32_t*>(&v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushuv(WickedPostfixEval_t* eval, uint16_t n, const uint32_t* v) {
  if (eval->stack_capacity - eval->stack_size < n) return WickedEvalStatus_StackOverflow;
  memcpy(&eval->stack_data[eval->stack_size], v, n * 4);
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushiv(WickedPostfixEval_t* eval, uint16_t n, const int32_t* v) {
  return WickedPostfixEval_pushuv(eval, n, reinterpret_cast<const uint32_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushfv(WickedPostfixEval_t* eval, uint16_t n, const float* v) {
  return WickedPostfixEval_pushuv(eval, n, reinterpret_cast<const uint32_t*>(v));
}

static WickedEvalStatus_t WickedPostfixEval_pushd(WickedPostfixEval_t* eval, uint16_t n) {
  if (eval->d_size - eval->d_idx < n) return WickedEvalStatus_DataUnderflow;
  const uint32_t* v = eval->d_data + eval->d_idx;
  CHECK_STATUS(WickedPostfixEval_pushuv(eval, n, v));
  eval->d_idx += n;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_implicitPushdArg(WickedPostfixEval_t* eval, uint8_t* arg, uint8_t multiple, uint8_t instances) {
  uint8_t mask = uint8_t(1 << instances) - 1;
  uint8_t push_count = *arg & mask;
  *arg >>= instances;
  if (push_count == 0) return WickedEvalStatus_Ok;
  uint16_t size = uint16_t(push_count) * uint16_t(multiple * *arg);
  return WickedPostfixEval_pushd(eval, size);
}

static WickedEvalStatus_t WickedPostfixEval_allocuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  if (eval->stack_size + n > eval->stack_capacity) return WickedEvalStatus_StackOverflow;
  *v = &eval->stack_data[eval->stack_size];
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_allociv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_allocuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_allocfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_allocuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popu(WickedPostfixEval_t* eval, uint32_t* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[--eval->stack_size];
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popi(WickedPostfixEval_t* eval, int32_t* v) {
  return WickedPostfixEval_popu(eval, reinterpret_cast<uint32_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popf(WickedPostfixEval_t* eval, float* v) {
  return WickedPostfixEval_popu(eval, reinterpret_cast<uint32_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= n;
  *v = &eval->stack_data[eval->stack_size];
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popiv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_popuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_popuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_peekuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  *v = &eval->stack_data[eval->stack_size - n];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_peekiv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_peekuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_peekfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_peekuv(eval, n, reinterpret_cast<uint32_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_nextp(WickedPostfixEval_t* eval, uint8_t* n) {
  if (eval->p_size - eval->p_idx < 1) return WickedEvalStatus_IllegalOperation;
  *n = eval->p_data[eval->p_idx];
  ++eval->p_idx;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  eval->p_idx = 0;
  eval->d_idx = 0;
  while (eval->p_idx < eval->p_size) {
    const uint8_t op = eval->p_data[eval->p_idx++];
    switch (op) {
    case WickedPostfixOp_Push: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      CHECK_STATUS(WickedPostfixEval_pushd(eval, n));
      break;
    }
    case WickedPostfixOp_Pop: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      uint32_t* discard;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, n, &discard));
      break;
    }
    case WickedPostfixOp_Dup: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      if (eval->stack_size - 1 < n) return WickedEvalStatus_StackUnderflow;
      uint32_t v = eval->stack_data[eval->stack_size - 1 - n];
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v));
      break;
    }
    case WickedPostfixOp_RotL: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      if (n <= 1) break;
      uint32_t* values;
      CHECK_STATUS(WickedPostfixEval_peekuv(eval, n, &values));
      uint32_t l = values[0];
      std::copy(&values[1], &values[n], &values[0]);
      values[n-1] = l;
      break;
    }
    case WickedPostfixOp_RotR: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      if (n <= 1) break;
      uint32_t* values;
      CHECK_STATUS(WickedPostfixEval_peekuv(eval, n, &values));
      uint32_t r = values[n-1];
      std::copy(&values[0], &values[n-1], &values[1]);
      values[0] = r;
      break;
    }
    case WickedPostfixOp_Rev: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &n));
      uint32_t* values;
      CHECK_STATUS(WickedPostfixEval_peekuv(eval, n, &values));
      std::reverse(values, values + n);
      break;
    }
    case WickedPostfixOp_Transpose: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &cols, rows, 1));

      uint32_t* m;
      uint16_t size = rows * cols;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, rows * cols, &m));
      if (size > eval->temp_capacity) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          size_t midx = cols * i + j;
          size_t tidx = rows * j + i;
          eval->temp_data[tidx] = m[midx];
        }
      }
      CHECK_STATUS(WickedPostfixEval_pushuv(eval, size, eval->temp_data));
      break;
    }
    case WickedPostfixOp_Add: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_Sub: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_Mul: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAdd: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 3, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_Div: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_Mod: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::fmod(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_Neg: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, -v));
      break;
    }
    case WickedPostfixOp_Abs: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::abs(v)));
      break;
    }
    case WickedPostfixOp_Inv: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, 1.0f / v));
      break;
    }
    case WickedPostfixOp_Pow: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::pow(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_Sqrt: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::sqrt(v)));
      break;
    }
    case WickedPostfixOp_Exp: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::exp(v)));
      break;
    }
    case WickedPostfixOp_Ln: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::log(v)));
      break;
    }
    case WickedPostfixOp_Sin: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::sin(v)));
      break;
    }
    case WickedPostfixOp_Cos: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::cos(v)));
      break;
    }
    case WickedPostfixOp_Tan: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::tan(v)));
      break;
    }
    case WickedPostfixOp_Asin: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::asin(v)));
      break;
    }
    case WickedPostfixOp_Acos: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::acos(v)));
      break;
    }
    case WickedPostfixOp_Atan2: {
      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::atan2(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_PolyVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float result = 0;
      float p = 1;
      float t;
      float* coeff;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &coeff));
      CHECK_STATUS(WickedPostfixEval_popf(eval, &t));
      for (uint8_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      CHECK_STATUS(WickedPostfixEval_pushf(eval, result));
      break;
    }
    case WickedPostfixOp_PolyMat: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &cols, rows, 1));

      float t, *coeff, *result;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, rows * cols, &coeff));
      CHECK_STATUS(WickedPostfixEval_popf(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocfv(eval, cols, &result));
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
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float *lhs, *rhs;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case WickedPostfixOp_SubVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float *lhs, *rhs;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float *lhs, *rhs;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulAddVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 2));

      float *a, *b, *c;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &c));
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &b));
      CHECK_STATUS(WickedPostfixEval_peekfv(eval, size, &a));
      for (uint8_t i = 0; i < size; ++i) {
        a[i] = a[i] * b[i] + c[i];
      }
      break;
    }
    case WickedPostfixOp_ScaleVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float scalar, *v, *result;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &v));
      CHECK_STATUS(WickedPostfixEval_popf(eval, &scalar));
      CHECK_STATUS(WickedPostfixEval_allocfv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = scalar * v[i];
      }
      break;
    }
    case WickedPostfixOp_NegVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float* v;
      CHECK_STATUS(WickedPostfixEval_peekfv(eval, size, &v));
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case WickedPostfixOp_NormVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 1));

      float* v;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &v));
      float result = 0;
      for (uint8_t i = 0; i < size; ++i) {
        result += v[i] * v[i];
      }
      CHECK_STATUS(WickedPostfixEval_pushf(eval, std::sqrt(result)));
      break;
    }
    case WickedPostfixOp_MulMat: {
      uint8_t arows, brows, bcols;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &arows));
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &brows));
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &bcols));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &bcols, brows, 1));

      float *a, *b;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, brows * bcols, &b));
      CHECK_STATUS(WickedPostfixEval_popfv(eval, arows * brows, &a));
      size_t temp_size = arows * bcols;
      float* temp_data = reinterpret_cast<float*>(eval->temp_data);
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
          temp_data[cidx] = r;
        }
      }
      CHECK_STATUS(WickedPostfixEval_pushfv(eval, temp_size, temp_data));
      break;
    }
    case WickedPostfixOp_Lerp: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &size, 1, 2));

      float t, *v0, *v1, *result;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &v1));
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &v0));
      CHECK_STATUS(WickedPostfixEval_popf(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocfv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      break;
    }
    case WickedPostfixOp_LerpTable: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_nextp(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushdArg(eval, &cols, rows, 1));
      if (rows < 1) return WickedEvalStatus_IllegalOperation;
      if (cols < 1) return WickedEvalStatus_IllegalOperation;

      float t, *lut, *result;
      uint8_t size = rows * cols;
      uint8_t n = cols - 1;
      CHECK_STATUS(WickedPostfixEval_popfv(eval, size, &lut));
      CHECK_STATUS(WickedPostfixEval_popf(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocfv(eval, n, &result));
      uint8_t ubrow = wicked_upper_bound(uint8_t(0), rows, [t, cols, lut](uint8_t i) -> bool {
        return t < lut[cols*i];
      });
      if (ubrow == 0) {
        auto bound = lut;
        std::copy(bound + 1, bound + cols, result);
      } else if (ubrow == rows) {
        auto bound = lut + (size - cols);
        std::copy(bound + 1, bound + cols, result);
      } else {
        auto ub = lut + ubrow*cols;
        auto lb = ub - cols;
        float t0 = *lb;
        float t1 = *ub;
        t = (t - t0) / (t1 - t0);
        const float* v0 = lb + 1;
        const float* v1 = ub + 1;
        for (size_t i = 0; i < n; ++i) {
          result[i] = (1-t)*v0[i] + t*v1[i];
        }
      }
      break;
    }
    case WickedPostfixOp_AddI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_SubI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_MulI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAddI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 3, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_DivI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_ModI: {
      int32_t* v;
      CHECK_STATUS(WickedPostfixEval_popiv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v[0] % v[1]));
      break;
    }
    case WickedPostfixOp_NegI: {
      int32_t v;
      CHECK_STATUS(WickedPostfixEval_popi(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, -v));
      break;
    }
    case WickedPostfixOp_AbsI: {
      int32_t v;
      CHECK_STATUS(WickedPostfixEval_popi(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v < 0 ? -v : v));
      break;
    }
    case WickedPostfixOp_AddU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_SubU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_MulU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAddU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 3, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_DivU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_ModU: {
      uint32_t* v;
      CHECK_STATUS(WickedPostfixEval_popuv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_pushu(eval, v[0] % v[1]));
      break;
    }
    case WickedPostfixOp_ItoF: {
      int32_t v;
      CHECK_STATUS(WickedPostfixEval_popi(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, v));
      break;
    }
    case WickedPostfixOp_FtoI: {
      float v;
      CHECK_STATUS(WickedPostfixEval_popf(eval, &v));
      CHECK_STATUS(WickedPostfixEval_pushi(eval, v));
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
  header->p_size = p_size();
  header->d_size = d_size();

  uint8_t* i = data + i_offset();
  memcpy(i, p_data(), p_size());

  float* f = reinterpret_cast<float*>(data + d_offset());
  memcpy(f, d_data(), d_size() * 4);

  return true;
}
