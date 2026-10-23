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

#if defined(_MSC_VER)
#define WICKED_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define WICKED_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define WICKED_ALWAYS_INLINE inline
#endif

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

namespace {

struct ValidationContext {
  const uint8_t* pc;
  const uint8_t* const pc_end;
  int32_t stack_depth;
  const uint8_t stack_capacity;

  WICKED_ALWAYS_INLINE bool has_bytes(uint16_t n) const {
    return static_cast<size_t>(pc_end - pc) >= n;
  }

  WICKED_ALWAYS_INLINE uint8_t loadp() {
    return *pc++;
  }

  WICKED_ALWAYS_INLINE void skip(uint16_t n) {
    pc += n;
  }
};

namespace validate {

WICKED_ALWAYS_INLINE WickedEvalStatus_t Push(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.loadp();
  uint16_t byte_count = uint16_t(n) * sizeof(float);
  if (!ctx.has_bytes(byte_count)) return WickedEvalStatus_IllegalOperation;
  ctx.skip(byte_count);
  if (ctx.stack_depth + n > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  ctx.stack_depth += n;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Pop(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.loadp();
  if (ctx.stack_depth < n) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= n;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Dup(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.loadp();
  if (ctx.stack_depth < int32_t(n) + 1) return WickedEvalStatus_StackUnderflow;
  if (ctx.stack_depth + 1 > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  ctx.stack_depth += 1;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t RotOrRev(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.loadp();
  if (ctx.stack_depth < n) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Transpose(ValidationContext& ctx) {
  if (!ctx.has_bytes(2)) return WickedEvalStatus_IllegalOperation;
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  uint16_t size = uint16_t(rows) * cols;
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  if (rows > 1 && cols > 1 && rows != cols) {
    if (ctx.stack_depth + size > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  }
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t BinaryOp(ValidationContext& ctx) {
  if (ctx.stack_depth < 2) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= 1;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t MulAdd(ValidationContext& ctx) {
  if (ctx.stack_depth < 3) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= 2;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t UnaryOp(ValidationContext& ctx) {
  if (ctx.stack_depth < 1) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t BinaryVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < int32_t(size) * 2) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= size;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t MulAddVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < int32_t(size) * 3) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= int32_t(size) * 2;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t ScaleVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < int32_t(size) + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= 1;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t NegVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t NormVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  if (size == 0) {
    if (ctx.stack_depth + 1 > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
    ctx.stack_depth += 1;
  } else {
    ctx.stack_depth -= (size - 1);
  }
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t MulMat(ValidationContext& ctx) {
  if (!ctx.has_bytes(3)) return WickedEvalStatus_IllegalOperation;
  uint8_t arows = ctx.loadp();
  uint8_t brows = ctx.loadp();
  uint8_t bcols = ctx.loadp();
  uint16_t asize = uint16_t(arows) * brows;
  uint16_t bsize = uint16_t(brows) * bcols;
  uint16_t csize = uint16_t(arows) * bcols;
  if (ctx.stack_depth < asize + bsize) return WickedEvalStatus_StackUnderflow;
  if (ctx.stack_depth + csize > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  ctx.stack_depth = ctx.stack_depth - asize - bsize + csize;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t PolyVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < int32_t(size) + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= size;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t PolyMat(ValidationContext& ctx) {
  if (!ctx.has_bytes(2)) return WickedEvalStatus_IllegalOperation;
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  uint16_t coeff_size = uint16_t(rows) * cols;
  if (ctx.stack_depth < int32_t(coeff_size) + 1) return WickedEvalStatus_StackUnderflow;
  if (ctx.stack_depth - (int32_t(coeff_size) + 1) + cols > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  ctx.stack_depth = ctx.stack_depth - (int32_t(coeff_size) + 1) + cols;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t LerpVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.loadp();
  if (ctx.stack_depth < int32_t(size) * 2 + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= (int32_t(size) + 1);
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t LerpTable(ValidationContext& ctx) {
  if (!ctx.has_bytes(2)) return WickedEvalStatus_IllegalOperation;
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  if (rows < 1 || cols < 1) return WickedEvalStatus_IllegalOperation;
  uint16_t lut_size = uint16_t(rows) * cols;
  if (ctx.stack_depth < int32_t(lut_size) + 1) return WickedEvalStatus_StackUnderflow;
  if (ctx.stack_depth - (int32_t(lut_size) + 1) + (cols - 1) > ctx.stack_capacity) return WickedEvalStatus_StackOverflow;
  ctx.stack_depth = ctx.stack_depth - (int32_t(lut_size) + 1) + (cols - 1);
  return WickedEvalStatus_Ok;
}

} // namespace validate
} // namespace

extern "C" WickedEvalStatus_t WickedPostfixValidateProgram(const uint8_t* prog_data, uint16_t prog_size, uint8_t initial_stack_size, uint8_t stack_capacity, uint8_t expected_final_stack_size) {
  if (initial_stack_size > stack_capacity) {
    return WickedEvalStatus_StackOverflow;
  }

  ValidationContext ctx{
    .pc = prog_data,
    .pc_end = prog_data + prog_size,
    .stack_depth = initial_stack_size,
    .stack_capacity = stack_capacity,
  };

  while (ctx.pc < ctx.pc_end) {
    uint8_t op = ctx.loadp();
    WickedEvalStatus_t status = WickedEvalStatus_Ok;

    switch (op) {
      case WickedPostfixOp_Push:      status = validate::Push(ctx); break;
      case WickedPostfixOp_Pop:       status = validate::Pop(ctx); break;
      case WickedPostfixOp_Dup:       status = validate::Dup(ctx); break;
      case WickedPostfixOp_RotL:
      case WickedPostfixOp_RotR:
      case WickedPostfixOp_Rev:       status = validate::RotOrRev(ctx); break;
      case WickedPostfixOp_Transpose: status = validate::Transpose(ctx); break;
      case WickedPostfixOp_Add:
      case WickedPostfixOp_Sub:
      case WickedPostfixOp_Mul:
      case WickedPostfixOp_Div:
      case WickedPostfixOp_Mod:
      case WickedPostfixOp_Pow:
      case WickedPostfixOp_Atan2:     status = validate::BinaryOp(ctx); break;
      case WickedPostfixOp_MulAdd:    status = validate::MulAdd(ctx); break;
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
      case WickedPostfixOp_Acos:      status = validate::UnaryOp(ctx); break;
      case WickedPostfixOp_AddVec:
      case WickedPostfixOp_SubVec:
      case WickedPostfixOp_MulVec:    status = validate::BinaryVec(ctx); break;
      case WickedPostfixOp_MulAddVec: status = validate::MulAddVec(ctx); break;
      case WickedPostfixOp_ScaleVec:  status = validate::ScaleVec(ctx); break;
      case WickedPostfixOp_NegVec:    status = validate::NegVec(ctx); break;
      case WickedPostfixOp_NormVec:   status = validate::NormVec(ctx); break;
      case WickedPostfixOp_MulMat:    status = validate::MulMat(ctx); break;
      case WickedPostfixOp_PolyVec:   status = validate::PolyVec(ctx); break;
      case WickedPostfixOp_PolyMat:   status = validate::PolyMat(ctx); break;
      case WickedPostfixOp_LerpVec:   status = validate::LerpVec(ctx); break;
      case WickedPostfixOp_LerpTable: status = validate::LerpTable(ctx); break;
      default:
        return WickedEvalStatus_UndefinedOperation;
    }

    if (status != WickedEvalStatus_Ok) {
      return status;
    }
  }

  if (ctx.stack_depth != expected_final_stack_size) {
    return WickedEvalStatus_UnexpectedStackSize;
  }
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixValidate(const WickedPostfixEval_t* eval, uint8_t expected_final_stack_size) {
  if (!eval) return WickedEvalStatus_IllegalOperation;
  return WickedPostfixValidateProgram(eval->prog_data, eval->prog_size, eval->stack_size, eval->stack_capacity, expected_final_stack_size);
}

namespace {

struct EvalContext {
  const uint8_t* pc;
  float* sp;

  WICKED_ALWAYS_INLINE uint8_t loadp() {
    return *pc++;
  }

  WICKED_ALWAYS_INLINE const uint8_t* loadpv(uint16_t n) {
    const uint8_t* p = pc;
    pc += n;
    return p;
  }

  WICKED_ALWAYS_INLINE void push(float v) {
    *sp++ = v;
  }

  WICKED_ALWAYS_INLINE void pushv(const float* v, size_t n) {
    std::memcpy(sp, v, n * sizeof(float));
    sp += n;
  }

  WICKED_ALWAYS_INLINE float pop() {
    return *--sp;
  }

  WICKED_ALWAYS_INLINE std::span<float> popv(uint8_t n) {
    sp -= n;
    return std::span<float>(sp, n);
  }

  WICKED_ALWAYS_INLINE float& peek() {
    return sp[-1];
  }

  WICKED_ALWAYS_INLINE std::span<float> peekv(uint8_t n) {
    return std::span<float>(sp - n, n);
  }

  WICKED_ALWAYS_INLINE std::span<float> allocv(uint8_t n) {
    std::span<float> s(sp, n);
    sp += n;
    return s;
  }
};

namespace ops {

WICKED_ALWAYS_INLINE void Push(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  const uint8_t* v = ctx.loadpv(uint16_t(n) * sizeof(float));
  ctx.pushv(reinterpret_cast<const float*>(v), n);
}

WICKED_ALWAYS_INLINE void Pop(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  ctx.sp -= n;
}

WICKED_ALWAYS_INLINE void Dup(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  float v = ctx.sp[-1 - n];
  ctx.push(v);
}

WICKED_ALWAYS_INLINE void RotL(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    float l = values[0];
    std::copy(values.begin() + 1, values.end(), values.begin());
    values.back() = l;
  }
}

WICKED_ALWAYS_INLINE void RotR(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    float r = values.back();
    std::copy_backward(values.begin(), values.end() - 1, values.end());
    values.front() = r;
  }
}

WICKED_ALWAYS_INLINE void Rev(EvalContext& ctx) {
  uint8_t n = ctx.loadp();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    std::reverse(values.begin(), values.end());
  }
}

WICKED_ALWAYS_INLINE void Transpose(EvalContext& ctx) {
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  uint16_t size = uint16_t(rows) * cols;
  std::span<float> m = ctx.peekv(size);
  if (rows > 1 && cols > 1) {
    if (rows == cols) {
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = i + 1; j < cols; ++j) {
          std::swap(m[i * cols + j], m[j * cols + i]);
        }
      }
    } else {
      float* out = ctx.sp;
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          out[rows * j + i] = m[cols * i + j];
        }
      }
      std::memcpy(m.data(), out, size * sizeof(float));
    }
  }
}

WICKED_ALWAYS_INLINE void Add(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() += b;
}

WICKED_ALWAYS_INLINE void Sub(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() -= b;
}

WICKED_ALWAYS_INLINE void Mul(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() *= b;
}

WICKED_ALWAYS_INLINE void MulAdd(EvalContext& ctx) {
  float c = ctx.pop();
  float b = ctx.pop();
  float& top = ctx.peek();
  top = top * b + c;
}

WICKED_ALWAYS_INLINE void Div(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() /= b;
}

WICKED_ALWAYS_INLINE void Mod(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() = std::fmod(ctx.peek(), b);
}

WICKED_ALWAYS_INLINE void Neg(EvalContext& ctx) {
  ctx.peek() = -ctx.peek();
}

WICKED_ALWAYS_INLINE void Abs(EvalContext& ctx) {
  ctx.peek() = std::abs(ctx.peek());
}

WICKED_ALWAYS_INLINE void Inv(EvalContext& ctx) {
  ctx.peek() = 1.0f / ctx.peek();
}

WICKED_ALWAYS_INLINE void Pow(EvalContext& ctx) {
  float b = ctx.pop();
  ctx.peek() = std::pow(ctx.peek(), b);
}

WICKED_ALWAYS_INLINE void Sqrt(EvalContext& ctx) {
  ctx.peek() = std::sqrt(ctx.peek());
}

WICKED_ALWAYS_INLINE void Exp(EvalContext& ctx) {
  ctx.peek() = std::exp(ctx.peek());
}

WICKED_ALWAYS_INLINE void Ln(EvalContext& ctx) {
  ctx.peek() = std::log(ctx.peek());
}

WICKED_ALWAYS_INLINE void Sin(EvalContext& ctx) {
  ctx.peek() = std::sin(ctx.peek());
}

WICKED_ALWAYS_INLINE void Cos(EvalContext& ctx) {
  ctx.peek() = std::cos(ctx.peek());
}

WICKED_ALWAYS_INLINE void Tan(EvalContext& ctx) {
  ctx.peek() = std::tan(ctx.peek());
}

WICKED_ALWAYS_INLINE void Asin(EvalContext& ctx) {
  ctx.peek() = std::asin(ctx.peek());
}

WICKED_ALWAYS_INLINE void Acos(EvalContext& ctx) {
  ctx.peek() = std::acos(ctx.peek());
}

WICKED_ALWAYS_INLINE void Atan2(EvalContext& ctx) {
  float x = ctx.pop();
  ctx.peek() = std::atan2(ctx.peek(), x);
}

WICKED_ALWAYS_INLINE void PolyVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> coeff = ctx.popv(size);
  float t = ctx.pop();
  float result = 0;
  float p = 1;
  for (uint8_t n = 0; n < size; ++n) {
    result += coeff[n] * p;
    p *= t;
  }
  ctx.push(result);
}

WICKED_ALWAYS_INLINE void PolyMat(EvalContext& ctx) {
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  std::span<float> coeff = ctx.popv(rows * cols);
  float t = ctx.pop();
  std::span<float> result = ctx.allocv(cols);
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
}

WICKED_ALWAYS_INLINE void AddVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] += rhs[i];
  }
}

WICKED_ALWAYS_INLINE void SubVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] -= rhs[i];
  }
}

WICKED_ALWAYS_INLINE void MulVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] *= rhs[i];
  }
}

WICKED_ALWAYS_INLINE void MulAddVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> c = ctx.popv(size);
  std::span<float> b = ctx.popv(size);
  std::span<float> a = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    a[i] = a[i] * b[i] + c[i];
  }
}

WICKED_ALWAYS_INLINE void ScaleVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> v = ctx.popv(size);
  float scalar = ctx.pop();
  std::span<float> result = ctx.allocv(size);
  for (uint8_t i = 0; i < size; ++i) {
    result[i] = scalar * v[i];
  }
}

WICKED_ALWAYS_INLINE void NegVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> v = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    v[i] = -v[i];
  }
}

WICKED_ALWAYS_INLINE void NormVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> v = ctx.popv(size);
  float result = 0;
  for (uint8_t i = 0; i < size; ++i) {
    result += v[i] * v[i];
  }
  ctx.push(std::sqrt(result));
}

WICKED_ALWAYS_INLINE void MulMat(EvalContext& ctx) {
  uint8_t arows = ctx.loadp();
  uint8_t brows = ctx.loadp();
  uint8_t bcols = ctx.loadp();

  uint16_t asize = uint16_t(arows) * brows;
  uint16_t bsize = uint16_t(brows) * bcols;
  uint16_t csize = uint16_t(arows) * bcols;

  std::span<float> b = ctx.popv(bsize);
  std::span<float> a = ctx.popv(asize);

  float* c = ctx.sp + asize + bsize;

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
  ctx.sp += csize;
}

WICKED_ALWAYS_INLINE void LerpVec(EvalContext& ctx) {
  uint8_t size = ctx.loadp();
  std::span<float> v1 = ctx.popv(size);
  std::span<float> v0 = ctx.popv(size);
  float t = ctx.pop();
  std::span<float> result = ctx.allocv(size);
  for (uint8_t i = 0; i < size; ++i) {
    result[i] = (1.0f - t) * v0[i] + t * v1[i];
  }
}

WICKED_ALWAYS_INLINE void LerpTable(EvalContext& ctx) {
  uint8_t rows = ctx.loadp();
  uint8_t cols = ctx.loadp();
  uint8_t size = rows * cols;
  uint8_t n = cols - 1;
  std::span<float> lut = ctx.popv(size);
  float t = ctx.pop();
  std::span<float> result = ctx.allocv(n);
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
}

} // namespace ops
} // namespace

extern "C" void WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  EvalContext ctx{
    .pc = eval->prog_data,
    .sp = eval->stack_data + eval->stack_size,
  };
  const uint8_t* const pc_end = eval->prog_data + eval->prog_size;

  while (ctx.pc < pc_end) {
    const uint8_t op = ctx.loadp();
    switch (op) {
      case WickedPostfixOp_Push:      ops::Push(ctx); break;
      case WickedPostfixOp_Pop:       ops::Pop(ctx); break;
      case WickedPostfixOp_Dup:       ops::Dup(ctx); break;
      case WickedPostfixOp_RotL:      ops::RotL(ctx); break;
      case WickedPostfixOp_RotR:      ops::RotR(ctx); break;
      case WickedPostfixOp_Rev:       ops::Rev(ctx); break;
      case WickedPostfixOp_Transpose: ops::Transpose(ctx); break;
      case WickedPostfixOp_Add:       ops::Add(ctx); break;
      case WickedPostfixOp_Sub:       ops::Sub(ctx); break;
      case WickedPostfixOp_Mul:       ops::Mul(ctx); break;
      case WickedPostfixOp_MulAdd:    ops::MulAdd(ctx); break;
      case WickedPostfixOp_Div:       ops::Div(ctx); break;
      case WickedPostfixOp_Mod:       ops::Mod(ctx); break;
      case WickedPostfixOp_Neg:       ops::Neg(ctx); break;
      case WickedPostfixOp_Abs:       ops::Abs(ctx); break;
      case WickedPostfixOp_Inv:       ops::Inv(ctx); break;
      case WickedPostfixOp_Pow:       ops::Pow(ctx); break;
      case WickedPostfixOp_Sqrt:      ops::Sqrt(ctx); break;
      case WickedPostfixOp_Exp:       ops::Exp(ctx); break;
      case WickedPostfixOp_Ln:        ops::Ln(ctx); break;
      case WickedPostfixOp_Sin:       ops::Sin(ctx); break;
      case WickedPostfixOp_Cos:       ops::Cos(ctx); break;
      case WickedPostfixOp_Tan:       ops::Tan(ctx); break;
      case WickedPostfixOp_Asin:      ops::Asin(ctx); break;
      case WickedPostfixOp_Acos:      ops::Acos(ctx); break;
      case WickedPostfixOp_Atan2:     ops::Atan2(ctx); break;
      case WickedPostfixOp_PolyVec:   ops::PolyVec(ctx); break;
      case WickedPostfixOp_PolyMat:   ops::PolyMat(ctx); break;
      case WickedPostfixOp_AddVec:    ops::AddVec(ctx); break;
      case WickedPostfixOp_SubVec:    ops::SubVec(ctx); break;
      case WickedPostfixOp_MulVec:    ops::MulVec(ctx); break;
      case WickedPostfixOp_MulAddVec: ops::MulAddVec(ctx); break;
      case WickedPostfixOp_ScaleVec:  ops::ScaleVec(ctx); break;
      case WickedPostfixOp_NegVec:    ops::NegVec(ctx); break;
      case WickedPostfixOp_NormVec:   ops::NormVec(ctx); break;
      case WickedPostfixOp_MulMat:    ops::MulMat(ctx); break;
      case WickedPostfixOp_LerpVec:   ops::LerpVec(ctx); break;
      case WickedPostfixOp_LerpTable: ops::LerpTable(ctx); break;
      default:
        break;
    }
  }

  eval->pc = static_cast<uint16_t>(ctx.pc - eval->prog_data);
  eval->stack_size = static_cast<uint8_t>(ctx.sp - eval->stack_data);
}

bool WickedPostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;

  auto* header = reinterpret_cast<WickedPostfixHeader*>(data);
  header->prog_size = prog_size();
  memcpy(data + prog_offset(), prog_data(), prog_size());

  return true;
}
