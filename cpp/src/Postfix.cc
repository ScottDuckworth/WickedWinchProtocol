#include <WickedWinchProtocol/Postfix.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>

#define CHECK_STATUS(expr) if (WickedEvalStatus status = expr; status != WickedEvalStatus_Ok) return status

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4);
static_assert(sizeof(WickedPostfixHeader) == 4);

// Return the smallest index i in [0, n) at which pred(i) is true.
template <typename Pred>
static size_t upper_bound(size_t first, size_t last, const Pred& pred) {
  size_t i = first;
  size_t n = last - first;
  while (n > 0) {
    size_t h = n / 2;
    if (pred(i + h)) {
      n = h;
    } else {
      i += h + 1;
      n -= h + 1;
    }
  }
  return i;
}

extern "C" const char* WickedPostfixOpToString(WickedPostfixOp op) {
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
    case WickedPostfixOp_Lut:       return "Lut";
  }
  return "UNKNOWN";
}

extern "C" bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval) {
  if (size < sizeof(WickedPostfixHeader)) return false;

	const WickedPostfixHeader* header = reinterpret_cast<const WickedPostfixHeader*>(data);
	uint8_t op_size = header->op_size;
	uint8_t i_size = header->i_size;
	uint16_t f_size = header->f_size;

	constexpr size_t op_offset = sizeof(WickedPostfixHeader);
	size_t i_offset = op_offset + op_size;
  size_t f_offset = op_offset + op_size + i_size;
  f_offset = (f_offset + size_t(3)) & ~size_t(3);

	size_t data_size = f_offset + f_size * 4;
  if (size != data_size) return false;

  eval->op_head = data + op_offset;
  eval->i_head = data + i_offset;
  eval->f_head = reinterpret_cast<const float*>(data + f_offset);
  eval->op_size = op_size;
  eval->i_size = i_size;
  eval->f_size = f_size;
  return true;
}

extern "C" void WickedPostfixEval_reset(WickedPostfixEval_t* eval) {
  // memset(eval->stack_data, 0, eval->stack_capacity * sizeof(float));
  // memset(eval->temp_data, 0, eval->temp_capacity * sizeof(float));
  eval->stack_size = 0;
}

extern "C" WickedEvalStatus WickedPostfixEval_push(WickedPostfixEval_t* eval, float v) {
  if (eval->stack_capacity - eval->stack_size < 1) return WickedEvalStatus_StackOverflow;
  eval->stack_data[eval->stack_size++] = v;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus WickedPostfixEval_pushv(WickedPostfixEval_t* eval, const float* v, size_t size) {
  if (eval->stack_capacity - eval->stack_size < size) return WickedEvalStatus_StackOverflow;
  memcpy(&eval->stack_data[eval->stack_size], v, size * sizeof(float));
  eval->stack_size += size;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus WickedPostfixEval_pushv(WickedPostfixEval_t* eval, std::span<const float> v) {
  return WickedPostfixEval_pushv(eval, v.data(), v.size());
}

static WickedEvalStatus WickedPostfixEval_pushf(WickedPostfixEval_t* eval, uint16_t n) {
  if (n > eval->f_size) return WickedEvalStatus_FloatLiteralsUnderflow;
  CHECK_STATUS(WickedPostfixEval_pushv(eval, std::span<const float>(eval->f_head, n)));
  eval->f_size -= n;
  eval->f_head += n;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus WickedPostfixEval_implicitPushArg(WickedPostfixEval_t* eval, uint8_t* arg, uint8_t multiple, uint8_t instances) {
  uint8_t mask = uint8_t(1 << instances) - 1;
  uint8_t push_count = *arg & mask;
  *arg >>= instances;
  if (push_count == 0) return WickedEvalStatus_Ok;
  uint16_t size = uint16_t(push_count) * uint16_t(multiple * *arg);
  return WickedPostfixEval_pushf(eval, size);
}

static WickedEvalStatus WickedPostfixEval_allocv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size + n > eval->stack_capacity) return WickedEvalStatus_StackOverflow;
  *v = std::span<float>(&eval->stack_data[eval->stack_size], n);
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus WickedPostfixEval_pop(WickedPostfixEval_t* eval, float* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[--eval->stack_size];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus WickedPostfixEval_popv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= n;
  *v = std::span<float>(&eval->stack_data[eval->stack_size], n);
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus WickedPostfixEval_popv(WickedPostfixEval_t* eval, float* v, size_t n) {
  std::span<float> src;
  WickedEvalStatus status = WickedPostfixEval_popv(eval, n, &src);
  if (status != WickedEvalStatus_Ok) return status;
  for (float s : src) *v++ = s;
  return status;
}

static WickedEvalStatus WickedPostfixEval_peek(WickedPostfixEval_t* eval, float* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  *v = eval->stack_data[eval->stack_size-1];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus WickedPostfixEval_peekv(WickedPostfixEval_t* eval, uint8_t n, std::span<float>* v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  *v = std::span<float>(&eval->stack_data[eval->stack_size - n], n);
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus WickedPostfixEval_geti(WickedPostfixEval_t* eval, uint8_t* n) {
  if (eval->i_size < 1) return WickedEvalStatus_IntLiteralsUnderflow;
  *n = eval->i_head[0];
  --eval->i_size;
  ++eval->i_head;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  for (uint8_t opi = 0; opi < eval->op_size; ++opi) {
    const uint8_t op = eval->op_head[opi];
    switch (op) {
    case WickedPostfixOp_Push: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
      CHECK_STATUS(WickedPostfixEval_pushf(eval, n));
      break;
    }
    case WickedPostfixOp_Pop: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
      std::span<float> discard;
      CHECK_STATUS(WickedPostfixEval_popv(eval, n, &discard));
      break;
    }
    case WickedPostfixOp_Dup: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
      if (eval->stack_size - 1 < n) return WickedEvalStatus_StackUnderflow;
      float v = eval->stack_data[eval->stack_size - 1 - n];
      CHECK_STATUS(WickedPostfixEval_push(eval, v));
      break;
    }
    case WickedPostfixOp_RotL: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
      if (n <= 1) break;
      std::span<float> values;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, n, &values));
      float r = values.back();
      std::copy(values.begin(), values.end() - 1, values.begin() + 1);
      values[0] = r;
      break;
    }
    case WickedPostfixOp_Rev: {
      uint8_t n;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &n));
      std::span<float> values;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, n, &values));
      std::reverse(values.begin(), values.end());
      break;
    }
    case WickedPostfixOp_Transpose: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_geti(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &cols, rows, 1));

      std::span<float> m;
      CHECK_STATUS(WickedPostfixEval_popv(eval, rows * cols, &m));
      if (m.size() > eval->temp_capacity) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          size_t midx = cols * i + j;
          size_t tidx = rows * j + i;
          eval->temp_data[tidx] = m[midx];
        }
      }
      CHECK_STATUS(WickedPostfixEval_pushv(eval, std::span(eval->temp_data, m.size())));
      break;
    }
    case WickedPostfixOp_Add: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_Sub: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_Mul: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAdd: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 3, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_Div: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_Mod: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::fmod(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_Neg: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, -v));
      break;
    }
    case WickedPostfixOp_Abs: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::abs(v)));
      break;
    }
    case WickedPostfixOp_Inv: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, 1.0f / v));
      break;
    }
    case WickedPostfixOp_Pow: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::pow(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_Sqrt: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::sqrt(v)));
      break;
    }
    case WickedPostfixOp_Exp: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::exp(v)));
      break;
    }
    case WickedPostfixOp_Ln: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::log(v)));
      break;
    }
    case WickedPostfixOp_Sin: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::sin(v)));
      break;
    }
    case WickedPostfixOp_Cos: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::cos(v)));
      break;
    }
    case WickedPostfixOp_Tan: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::tan(v)));
      break;
    }
    case WickedPostfixOp_Asin: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::asin(v)));
      break;
    }
    case WickedPostfixOp_Acos: {
      float v;
      CHECK_STATUS(WickedPostfixEval_pop(eval, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::acos(v)));
      break;
    }
    case WickedPostfixOp_Atan2: {
      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_popv(eval, 2, &v));
      CHECK_STATUS(WickedPostfixEval_push(eval, std::atan2(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_PolyVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_geti(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &cols, rows, 1));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case WickedPostfixOp_SubVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

      std::span<float> lhs, rhs;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &rhs));
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulAddVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 2));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

      std::span<float> v;
      CHECK_STATUS(WickedPostfixEval_peekv(eval, size, &v));
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case WickedPostfixOp_NormVec: {
      uint8_t size;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 1));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &arows));
      CHECK_STATUS(WickedPostfixEval_geti(eval, &brows));
      CHECK_STATUS(WickedPostfixEval_geti(eval, &bcols));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &bcols, brows, 1));

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
      CHECK_STATUS(WickedPostfixEval_geti(eval, &size));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &size, 1, 2));

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
    case WickedPostfixOp_Lut: {
      uint8_t rows, cols;
      CHECK_STATUS(WickedPostfixEval_geti(eval, &rows));
      CHECK_STATUS(WickedPostfixEval_geti(eval, &cols));
      CHECK_STATUS(WickedPostfixEval_implicitPushArg(eval, &cols, rows, 1));
      if (rows < 1) return WickedEvalStatus_IllegalOperation;
      if (cols < 1) return WickedEvalStatus_IllegalOperation;

      float t;
      std::span<float> lut, result;
      uint8_t size = rows * cols;
      uint8_t n = cols - 1;
      CHECK_STATUS(WickedPostfixEval_popv(eval, size, &lut));
      CHECK_STATUS(WickedPostfixEval_pop(eval, &t));
      CHECK_STATUS(WickedPostfixEval_allocv(eval, n, &result));
      size_t ubrow = upper_bound(0, rows, [t, cols, lut](size_t i) -> bool {
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
  header->op_size = op_size();
  header->i_size = i_size();
  header->f_size = f_size();

  uint8_t* op = data + op_offset();
  memcpy(op, op_data(), op_size());

  uint8_t* i = data + i_offset();
  memcpy(i, i_data(), i_size());

  float* f = reinterpret_cast<float*>(data + f_offset());
  memcpy(f, f_data(), f_size() * sizeof(float));

  return true;
}
