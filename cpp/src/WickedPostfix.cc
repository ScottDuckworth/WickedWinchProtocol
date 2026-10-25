#include "WickedPostfix.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>
#include <span>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4);
static_assert(sizeof(WickedPostfixHeader) == 2);
static_assert(alignof(WickedPostfixHeader) == 1);

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
    case WickedPostfixOp_Store:     return "Store";
    case WickedPostfixOp_Load:      return "Load";
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
  }
  return "UNKNOWN";
}

extern "C" bool WickedPostfixRead(const uint8_t* data, size_t size, const uint8_t** prog_data, uint16_t* prog_size) {
  if (size < sizeof(WickedPostfixHeader)) return false;
  uint16_t p_size;
  memcpy(&p_size, data, sizeof(uint16_t));
  constexpr size_t prog_offset = sizeof(WickedPostfixHeader);
  size_t total_size = prog_offset + p_size;
  if (size != total_size) return false;
  if (prog_data) *prog_data = data + prog_offset;
  if (prog_size) *prog_size = p_size;
  return true;
}

namespace {

struct ValidationContext {
  const uint8_t* pc;
  const uint8_t* const pc_end;
  int32_t stack_depth;
  int32_t max_stack_depth;

  WICKED_ALWAYS_INLINE void update_max(int32_t depth) {
    if (depth > max_stack_depth) {
      max_stack_depth = depth;
    }
  }

  WICKED_ALWAYS_INLINE bool has_bytes(uint16_t n) const {
    return static_cast<size_t>(pc_end - pc) >= n;
  }

  WICKED_ALWAYS_INLINE uint8_t fetch_byte() {
    return *pc++;
  }

  WICKED_ALWAYS_INLINE float fetch_float() {
    float f;
    std::memcpy(&f, pc, sizeof(float));
    pc += sizeof(float);
    return f;
  }

  WICKED_ALWAYS_INLINE void skip(uint16_t n) {
    pc += n;
  }
};

namespace validate {

WICKED_ALWAYS_INLINE WickedEvalStatus_t Push(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.fetch_byte();
  uint16_t byte_count = uint16_t(n) * sizeof(float);
  if (!ctx.has_bytes(byte_count)) return WickedEvalStatus_IllegalOperation;
  ctx.skip(byte_count);
  ctx.stack_depth += n;
  ctx.update_max(ctx.stack_depth);
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Pop(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.fetch_byte();
  if (ctx.stack_depth < n) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= n;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Dup(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.fetch_byte();
  if (ctx.stack_depth < int32_t(n) + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth += 1;
  ctx.update_max(ctx.stack_depth);
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t RotOrRev(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t n = ctx.fetch_byte();
  if (ctx.stack_depth < n) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Transpose(ValidationContext& ctx) {
  if (!ctx.has_bytes(2)) return WickedEvalStatus_IllegalOperation;
  uint8_t rows = ctx.fetch_byte();
  uint8_t cols = ctx.fetch_byte();
  uint16_t size = uint16_t(rows) * cols;
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  if (rows > 1 && cols > 1 && rows != cols) {
    ctx.update_max(ctx.stack_depth + size);
  }
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Store(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t arg = ctx.fetch_byte();
  uint8_t slot = arg & 0x03;
  bool pop = (arg & 0x10) != 0;
  uint8_t size = ((arg >> 6) & 0x03) + 1;
  if (slot + size > 4) return WickedEvalStatus_IllegalOperation;
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  if (pop) {
    ctx.stack_depth -= size;
  }
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t Load(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t arg = ctx.fetch_byte();
  uint8_t slot = arg & 0x03;
  uint8_t size = ((arg >> 6) & 0x03) + 1;
  if (slot + size > 4) return WickedEvalStatus_IllegalOperation;
  ctx.stack_depth += size;
  ctx.update_max(ctx.stack_depth);
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
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < int32_t(size) * 2) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= size;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t MulAddVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < int32_t(size) * 3) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= int32_t(size) * 2;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t ScaleVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < int32_t(size) + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= 1;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t NegVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t NormVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < size) return WickedEvalStatus_StackUnderflow;
  if (size == 0) {
    ctx.stack_depth += 1;
    ctx.update_max(ctx.stack_depth);
  } else {
    ctx.stack_depth -= (size - 1);
  }
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t MulMat(ValidationContext& ctx) {
  if (!ctx.has_bytes(3)) return WickedEvalStatus_IllegalOperation;
  uint8_t arows = ctx.fetch_byte();
  uint8_t brows = ctx.fetch_byte();
  uint8_t bcols = ctx.fetch_byte();
  uint16_t asize = uint16_t(arows) * brows;
  uint16_t bsize = uint16_t(brows) * bcols;
  uint16_t csize = uint16_t(arows) * bcols;
  if (ctx.stack_depth < asize + bsize) return WickedEvalStatus_StackUnderflow;
  ctx.update_max(ctx.stack_depth + csize);
  ctx.stack_depth = ctx.stack_depth - asize - bsize + csize;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t PolyVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  uint16_t byte_count = uint16_t(size) * sizeof(float);
  if (!ctx.has_bytes(byte_count)) return WickedEvalStatus_IllegalOperation;
  ctx.skip(byte_count);
  if (ctx.stack_depth < 1) return WickedEvalStatus_StackUnderflow;
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t PolyMat(ValidationContext& ctx) {
  if (!ctx.has_bytes(2)) return WickedEvalStatus_IllegalOperation;
  uint8_t rows = ctx.fetch_byte();
  uint8_t cols = ctx.fetch_byte();
  uint16_t coeff_size = uint16_t(rows) * cols;
  uint16_t byte_count = coeff_size * sizeof(float);
  if (!ctx.has_bytes(byte_count)) return WickedEvalStatus_IllegalOperation;
  ctx.skip(byte_count);
  if (ctx.stack_depth < 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth = ctx.stack_depth - 1 + cols;
  ctx.update_max(ctx.stack_depth);
  return WickedEvalStatus_Ok;
}

WICKED_ALWAYS_INLINE WickedEvalStatus_t LerpVec(ValidationContext& ctx) {
  if (!ctx.has_bytes(1)) return WickedEvalStatus_IllegalOperation;
  uint8_t size = ctx.fetch_byte();
  if (ctx.stack_depth < int32_t(size) * 2 + 1) return WickedEvalStatus_StackUnderflow;
  ctx.stack_depth -= (int32_t(size) + 1);
  return WickedEvalStatus_Ok;
}

}  // namespace validate
}  // namespace

extern "C" WickedEvalStatus_t WickedPostfixValidate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, uint16_t* minimum_stack_capacity, uint16_t* final_stack_size) {
  ValidationContext ctx{
    .pc = prog_data,
    .pc_end = prog_data + prog_size,
    .stack_depth = initial_stack_size,
    .max_stack_depth = initial_stack_size,
  };

  while (ctx.pc < ctx.pc_end) {
    uint8_t op = ctx.fetch_byte();
    WickedEvalStatus_t status = WickedEvalStatus_Ok;

    switch (op) {
      case WickedPostfixOp_Push:      status = validate::Push(ctx); break;
      case WickedPostfixOp_Pop:       status = validate::Pop(ctx); break;
      case WickedPostfixOp_Dup:       status = validate::Dup(ctx); break;
      case WickedPostfixOp_RotL:
      case WickedPostfixOp_RotR:
      case WickedPostfixOp_Rev:       status = validate::RotOrRev(ctx); break;
      case WickedPostfixOp_Transpose: status = validate::Transpose(ctx); break;
      case WickedPostfixOp_Store:     status = validate::Store(ctx); break;
      case WickedPostfixOp_Load:      status = validate::Load(ctx); break;
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
      default:
        return WickedEvalStatus_UndefinedOperation;
    }

    if (status != WickedEvalStatus_Ok) {
      return status;
    }
  }

  if (ctx.max_stack_depth > UINT16_MAX) {
    return WickedEvalStatus_StackOverflow;
  }

  if (minimum_stack_capacity) {
    *minimum_stack_capacity = static_cast<uint16_t>(ctx.max_stack_depth);
  }
  if (final_stack_size) {
    *final_stack_size = static_cast<uint16_t>(ctx.stack_depth);
  }
  return WickedEvalStatus_Ok;
}

namespace {

struct EvalContext {
  const uint8_t* pc;
  float* sp;
  float scratch[4] = {};

  WICKED_ALWAYS_INLINE uint8_t fetch_byte() {
    return *pc++;
  }

  WICKED_ALWAYS_INLINE float fetch_float() {
    float f;
    std::memcpy(&f, pc, sizeof(float));
    pc += sizeof(float);
    return f;
  }

  WICKED_ALWAYS_INLINE void push(float v) {
    *sp++ = v;
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
  uint8_t n = ctx.fetch_byte();
  size_t bytes = n * sizeof(float);
  std::memcpy(ctx.sp, ctx.pc, bytes);
  ctx.sp += n;
  ctx.pc += bytes;
}

WICKED_ALWAYS_INLINE void Pop(EvalContext& ctx) {
  uint8_t n = ctx.fetch_byte();
  ctx.sp -= n;
}

WICKED_ALWAYS_INLINE void Dup(EvalContext& ctx) {
  uint8_t n = ctx.fetch_byte();
  float v = ctx.sp[-1 - n];
  ctx.push(v);
}

WICKED_ALWAYS_INLINE void RotL(EvalContext& ctx) {
  uint8_t n = ctx.fetch_byte();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    float l = values[0];
    std::copy(values.begin() + 1, values.end(), values.begin());
    values.back() = l;
  }
}

WICKED_ALWAYS_INLINE void RotR(EvalContext& ctx) {
  uint8_t n = ctx.fetch_byte();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    float r = values.back();
    std::copy_backward(values.begin(), values.end() - 1, values.end());
    values.front() = r;
  }
}

WICKED_ALWAYS_INLINE void Rev(EvalContext& ctx) {
  uint8_t n = ctx.fetch_byte();
  if (n > 1) {
    std::span<float> values = ctx.peekv(n);
    std::reverse(values.begin(), values.end());
  }
}

WICKED_ALWAYS_INLINE void Transpose(EvalContext& ctx) {
  uint8_t rows = ctx.fetch_byte();
  uint8_t cols = ctx.fetch_byte();
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

WICKED_ALWAYS_INLINE void Store(EvalContext& ctx) {
  uint8_t arg = ctx.fetch_byte();
  uint8_t slot = arg & 0x03;
  bool pop = (arg & 0x10) != 0;
  uint8_t size = ((arg >> 6) & 0x03) + 1;
  std::span<float> src = pop ? ctx.popv(size) : ctx.peekv(size);
  std::copy_n(src.data(), size, &ctx.scratch[slot]);
}

WICKED_ALWAYS_INLINE void Load(EvalContext& ctx) {
  uint8_t arg = ctx.fetch_byte();
  uint8_t slot = arg & 0x03;
  uint8_t size = ((arg >> 6) & 0x03) + 1;
  std::span<float> dest = ctx.allocv(size);
  std::copy_n(&ctx.scratch[slot], size, dest.data());
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
  top = std::fma(top, b, c);
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
  uint8_t size = ctx.fetch_byte();
  float t = ctx.pop();
  if (size == 0) {
    ctx.push(0.0f);
    return;
  }
  float result = ctx.fetch_float();
  for (uint8_t n = 1; n < size; ++n) {
    result = std::fma(result, t, ctx.fetch_float());
  }
  ctx.push(result);
}

WICKED_ALWAYS_INLINE void PolyMat(EvalContext& ctx) {
  uint8_t rows = ctx.fetch_byte();
  uint8_t cols = ctx.fetch_byte();
  float t = ctx.pop();
  std::span<float> result = ctx.allocv(cols);
  if (rows == 0) {
    std::fill(result.begin(), result.end(), 0.0f);
    return;
  }
  for (uint8_t j = 0; j < cols; ++j) {
    result[j] = ctx.fetch_float();
  }
  for (uint8_t i = 1; i < rows; ++i) {
    for (uint8_t j = 0; j < cols; ++j) {
      result[j] = std::fma(result[j], t, ctx.fetch_float());
    }
  }
}

WICKED_ALWAYS_INLINE void AddVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] += rhs[i];
  }
}

WICKED_ALWAYS_INLINE void SubVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] -= rhs[i];
  }
}

WICKED_ALWAYS_INLINE void MulVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> rhs = ctx.popv(size);
  std::span<float> lhs = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    lhs[i] *= rhs[i];
  }
}

WICKED_ALWAYS_INLINE void MulAddVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> c = ctx.popv(size);
  std::span<float> b = ctx.popv(size);
  std::span<float> a = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    a[i] = a[i] * b[i] + c[i];
  }
}

WICKED_ALWAYS_INLINE void ScaleVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> v = ctx.popv(size);
  float scalar = ctx.pop();
  std::span<float> result = ctx.allocv(size);
  for (uint8_t i = 0; i < size; ++i) {
    result[i] = scalar * v[i];
  }
}

WICKED_ALWAYS_INLINE void NegVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> v = ctx.peekv(size);
  for (uint8_t i = 0; i < size; ++i) {
    v[i] = -v[i];
  }
}

WICKED_ALWAYS_INLINE void NormVec(EvalContext& ctx) {
  uint8_t size = ctx.fetch_byte();
  std::span<float> v = ctx.popv(size);
  float result = 0;
  for (uint8_t i = 0; i < size; ++i) {
    result += v[i] * v[i];
  }
  ctx.push(std::sqrt(result));
}

WICKED_ALWAYS_INLINE void MulMat(EvalContext& ctx) {
  uint8_t arows = ctx.fetch_byte();
  uint8_t brows = ctx.fetch_byte();
  uint8_t bcols = ctx.fetch_byte();

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
  uint8_t size = ctx.fetch_byte();
  std::span<float> v1 = ctx.popv(size);
  std::span<float> v0 = ctx.popv(size);
  float t = ctx.pop();
  std::span<float> result = ctx.allocv(size);
  for (uint8_t i = 0; i < size; ++i) {
    result[i] = (1.0f - t) * v0[i] + t * v1[i];
  }
}

}  // namespace ops
}  // namespace

extern "C" void WickedPostfixEvaluate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, float* stack_data) {
  EvalContext ctx{
    .pc = prog_data,
    .sp = stack_data + initial_stack_size,
  };
  const uint8_t* const pc_end = prog_data + prog_size;

  while (ctx.pc < pc_end) {
    const uint8_t op = ctx.fetch_byte();
    switch (op) {
      case WickedPostfixOp_Push:      ops::Push(ctx); break;
      case WickedPostfixOp_Pop:       ops::Pop(ctx); break;
      case WickedPostfixOp_Dup:       ops::Dup(ctx); break;
      case WickedPostfixOp_RotL:      ops::RotL(ctx); break;
      case WickedPostfixOp_RotR:      ops::RotR(ctx); break;
      case WickedPostfixOp_Rev:       ops::Rev(ctx); break;
      case WickedPostfixOp_Transpose: ops::Transpose(ctx); break;
      case WickedPostfixOp_Store:     ops::Store(ctx); break;
      case WickedPostfixOp_Load:      ops::Load(ctx); break;
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
      default:
        break;
    }
  }
}

bool WickedPostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;

  auto* header = reinterpret_cast<WickedPostfixHeader*>(data);
  header->prog_size = prog_size();
  memcpy(data + prog_offset(), prog_data(), prog_size());

  return true;
}
