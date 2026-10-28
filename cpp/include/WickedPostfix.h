#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WICKED_PACKED
#if defined(__GNUC__) || defined(__clang__)
#define WICKED_PACKED __attribute__((__packed__))
#else
#define WICKED_PACKED
#endif
#endif

#pragma pack(push, 1)
// Header preceding the postfix bytecode program, specifying program byte size.
typedef struct WICKED_PACKED WickedPostfixHeader {
	uint16_t prog_size;
} WickedPostfixHeader_t;
#pragma pack(pop)

#if defined(__cplusplus)
static_assert(sizeof(WickedPostfixHeader_t) == 2, "WickedPostfixHeader_t must be 2 bytes");
static_assert(alignof(WickedPostfixHeader_t) == 1, "WickedPostfixHeader_t must be packed");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(WickedPostfixHeader_t) == 2, "WickedPostfixHeader_t must be 2 bytes");
_Static_assert(_Alignof(WickedPostfixHeader_t) == 1, "WickedPostfixHeader_t must be packed");
#endif

// Bytecode operations for the postfix evaluation machine.
typedef enum WickedPostfixOp {
  WickedPostfixOp_Undefined = 0,
  WickedPostfixOp_Push      = 1,
  WickedPostfixOp_Drop      = 2,
  WickedPostfixOp_Dup       = 3,
  WickedPostfixOp_Swap      = 4,
  WickedPostfixOp_Over      = 5,
  WickedPostfixOp_Tuck      = 6,
  WickedPostfixOp_Nip       = 7,
  WickedPostfixOp_Rot       = 8,
  WickedPostfixOp_Pick      = 9,
  WickedPostfixOp_Add       = 10,
  WickedPostfixOp_Sub       = 11,
  WickedPostfixOp_Mul       = 12,
  WickedPostfixOp_Div       = 13,
  WickedPostfixOp_Pow       = 14,
  WickedPostfixOp_Ipow      = 15,
  WickedPostfixOp_MulAdd    = 16,
  WickedPostfixOp_Neg       = 17,
  WickedPostfixOp_Abs       = 18,
  WickedPostfixOp_Inv       = 19,
  WickedPostfixOp_Sqrt      = 20,
  WickedPostfixOp_Exp       = 21,
  WickedPostfixOp_Ln        = 22,
  WickedPostfixOp_Sin       = 23,
  WickedPostfixOp_Cos       = 24,
  WickedPostfixOp_Tan       = 25,
  WickedPostfixOp_Asin      = 26,
  WickedPostfixOp_Acos      = 27,
  WickedPostfixOp_Atan2     = 28,
  WickedPostfixOp_AddVec    = 29,
  WickedPostfixOp_SubVec    = 30,
  WickedPostfixOp_MulVec    = 31,
  WickedPostfixOp_MulAddVec = 32,
  WickedPostfixOp_ScaleVec  = 33,
  WickedPostfixOp_DivVec    = 34,
  WickedPostfixOp_NegVec    = 35,
  WickedPostfixOp_InvVec    = 36,
  WickedPostfixOp_NormSqVec = 37,
  WickedPostfixOp_SumVec    = 38,
  WickedPostfixOp_LerpVec   = 39,
  WickedPostfixOp_MulMat    = 40,
  WickedPostfixOp_Polynomial = 41,
} WickedPostfixOp_t;

// Returns the string representation of an opcode.
const char* WickedPostfixOpToString(WickedPostfixOp_t op);

// Parses and validates the header from data, returning pointers to prog_data and prog_size.
bool WickedPostfixRead(const uint8_t* data, size_t size, const uint8_t** prog_data, uint16_t* prog_size);

// Validates bytecode stack requirements and returns status, minimum stack capacity, and final stack size.
WickedEvalStatus_t WickedPostfixValidate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, uint16_t* minimum_stack_capacity, uint16_t* final_stack_size);

// Evaluates bytecode on stack_data in-place without bounds checking.
void WickedPostfixEvaluate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, float* stack_data);

// Unsafe inline reader that extracts prog_data and prog_size without bounds checking.
static inline void WickedPostfixReadUnsafe(const uint8_t* data, const uint8_t** prog_data, uint16_t* prog_size) {
  if (prog_data) *prog_data = data + sizeof(WickedPostfixHeader_t);
  if (prog_size) {
    uint16_t size;
    memcpy(&size, data, sizeof(uint16_t));
    *prog_size = size;
  }
}

#ifdef __cplusplus
}

#include <cassert>
#include <cstring>
#include <initializer_list>
#include <ostream>
#include <span>
#include <vector>

// Formats opcode name to output stream.
inline std::ostream& operator<<(std::ostream& out, WickedPostfixOp_t op) {
  return out << WickedPostfixOpToString(op);
}

// Builds serialized postfix bytecode programs.
class WickedPostfixWriter {
 public:
  // Pointer to bytecode data buffer.
  uint8_t* prog_data() { return prog_.data(); }
  // Const pointer to bytecode data buffer.
  const uint8_t* prog_data() const { return prog_.data(); }
  // Bytecode size in bytes.
  uint16_t prog_size() const { return static_cast<uint16_t>(prog_.size()); }
  // Total serialized size including header.
  uint16_t data_size() const { return prog_offset() + prog_size(); }

  // Serializes header and bytecode into the provided buffer.
  bool Write(uint8_t* data, size_t size) const;
  // Serializes header and bytecode into a newly allocated vector.
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

  // Clears the bytecode buffer.
  void clear() {
    prog_.clear();
  }

  // Emits a raw byte into the bytecode stream.
  void EmitByte(uint8_t b) { prog_.push_back(b); }

  // Emits an operation opcode byte into the bytecode stream.
  void EmitOp(WickedPostfixOp_t op) { EmitByte(static_cast<uint8_t>(op)); }

  // Emits a 32-bit float literal into the bytecode stream.
  void EmitFloat(float f) {
    uint8_t bytes[sizeof(float)];
    memcpy(bytes, &f, sizeof(float));
    for (uint8_t b : bytes) prog_.push_back(b);
  }

  // Pushes a single float literal onto the stack.
  void Push(float value) {
    EmitOp(WickedPostfixOp_Push);
    EmitByte(1);
    EmitFloat(value);
  }

  // Pushes multiple float literals onto the stack.
  void Push(std::span<const float> values) {
    EmitOp(WickedPostfixOp_Push);
    EmitByte(static_cast<uint8_t>(values.size()));
    for (float value : values) EmitFloat(value);
  }

  // Pushes multiple float literals onto the stack.
  void Push(std::initializer_list<float> values) {
    Push(std::span<const float>(values));
  }

  // Drops the top value from the stack.
  void Drop() {
    EmitOp(WickedPostfixOp_Drop);
  }

  // Duplicates the top value on the stack.
  void Dup() {
    EmitOp(WickedPostfixOp_Dup);
  }

  // Swaps the top two values on the stack.
  void Swap() {
    EmitOp(WickedPostfixOp_Swap);
  }

  // Copies the second value to the top (a b -> a b a).
  void Over() {
    EmitOp(WickedPostfixOp_Over);
  }

  // Inserts copy of top value under second value (a b -> b a b).
  void Tuck() {
    EmitOp(WickedPostfixOp_Tuck);
  }

  // Drops the second value from the top (a b -> b).
  void Nip() {
    EmitOp(WickedPostfixOp_Nip);
  }

  // Rotates third value to the top (a b c -> b c a).
  void Rot() {
    EmitOp(WickedPostfixOp_Rot);
  }

  // Copies or moves an element or subvector from stack depth offset to the top.
  void Pick(uint8_t offset, uint8_t size = 1, bool move = false) {
    EmitOp(WickedPostfixOp_Pick);
    uint8_t arg = (offset & 0x1F) | ((move ? 1 : 0) << 5) | (((size - 1) & 0x03) << 6);
    EmitByte(arg);
  }

  // Pops two values and pushes their sum.
  void Add() { EmitOp(WickedPostfixOp_Add); }

  // Pops rhs and lhs and pushes lhs - rhs.
  void Sub() { EmitOp(WickedPostfixOp_Sub); }

  // Pops two values and pushes their product.
  void Mul() { EmitOp(WickedPostfixOp_Mul); }

  // Pops c, b, a and pushes a*b + c.
  void MulAdd() { EmitOp(WickedPostfixOp_MulAdd); }

  // Pops rhs and lhs and pushes lhs / rhs.
  void Div() { EmitOp(WickedPostfixOp_Div); }

  // Pops rhs and lhs and pushes lhs^rhs.
  void Pow() { EmitOp(WickedPostfixOp_Pow); }

  // Raises the top value to the integer exponent exp (base^exp).
  void Ipow(int8_t exp) {
    EmitOp(WickedPostfixOp_Ipow);
    EmitByte(static_cast<uint8_t>(exp));
  }

  // Negates the top value.
  void Neg() { EmitOp(WickedPostfixOp_Neg); }

  // Absolute value of top value.
  void Abs() { EmitOp(WickedPostfixOp_Abs); }

  // Inverts top value (1 / v).
  void Inv() { EmitOp(WickedPostfixOp_Inv); }

  // Square root of top value.
  void Sqrt() { EmitOp(WickedPostfixOp_Sqrt); }

  // Exponential e^v of top value.
  void Exp() { EmitOp(WickedPostfixOp_Exp); }

  // Natural logarithm ln(v) of top value.
  void Ln() { EmitOp(WickedPostfixOp_Ln); }

  // Sine of top value.
  void Sin() { EmitOp(WickedPostfixOp_Sin); }

  // Cosine of top value.
  void Cos() { EmitOp(WickedPostfixOp_Cos); }

  // Tangent of top value.
  void Tan() { EmitOp(WickedPostfixOp_Tan); }

  // Arc sine of top value.
  void Asin() { EmitOp(WickedPostfixOp_Asin); }

  // Arc cosine of top value.
  void Acos() { EmitOp(WickedPostfixOp_Acos); }

  // Pops x and y and pushes atan2(y, x).
  void Atan2() { EmitOp(WickedPostfixOp_Atan2); }

  // Evaluates a polynomial or matrix of polynomials.
  // Operand constraints:
  //   - rows: 1 to 16 (number of polynomial coefficients per column, i.e. degree + 1)
  //   - cols: 1 to 16 (number of dimensions / columns)
  //   - coeffs.size() == rows * cols
  void Polynomial(uint8_t rows, uint8_t cols, std::span<const float> coeffs) {
    assert(rows >= 1 && rows <= 16);
    assert(cols >= 1 && cols <= 16);
    assert(coeffs.size() == static_cast<size_t>(rows) * cols);
    EmitOp(WickedPostfixOp_Polynomial);
    uint8_t arg = ((rows - 1) & 0x0F) | (((cols - 1) & 0x0F) << 4);
    EmitByte(arg);
    for (size_t i = rows; i > 0; --i) {
      size_t row_offset = (i - 1) * cols;
      for (size_t j = 0; j < cols; ++j) {
        EmitFloat(coeffs[row_offset + j]);
      }
    }
  }

  // Evaluates a polynomial or matrix of polynomials.
  void Polynomial(uint8_t rows, uint8_t cols, std::initializer_list<float> coeffs) {
    Polynomial(rows, cols, std::span<const float>(coeffs));
  }

  // Evaluates a 1D polynomial.
  void Polynomial(std::span<const float> coeffs) {
    Polynomial(static_cast<uint8_t>(coeffs.size()), 1, coeffs);
  }

  // Evaluates a 1D polynomial.
  void Polynomial(std::initializer_list<float> coeffs) {
    Polynomial(std::span<const float>(coeffs));
  }

  // Deprecated compatibility wrappers
  void PolyVec(std::span<const float> coeffs) { Polynomial(coeffs); }
  void PolyVec(std::initializer_list<float> coeffs) { Polynomial(coeffs); }
  void PolyMat(uint8_t rows, uint8_t cols, std::span<const float> coeffs) { Polynomial(rows, cols, coeffs); }
  void PolyMat(uint8_t rows, uint8_t cols, std::initializer_list<float> coeffs) { Polynomial(rows, cols, coeffs); }

  // Adds two vectors componentwise.
  void AddVec(uint8_t size) {
    EmitOp(WickedPostfixOp_AddVec);
    EmitByte(size);
  }

  // Subtracts two vectors componentwise (lhs - rhs).
  void SubVec(uint8_t size) {
    EmitOp(WickedPostfixOp_SubVec);
    EmitByte(size);
  }

  // Multiplies two vectors componentwise.
  void MulVec(uint8_t size) {
    EmitOp(WickedPostfixOp_MulVec);
    EmitByte(size);
  }

  // Componentwise fused multiply-add (a*b + c).
  void MulAddVec(uint8_t size) {
    EmitOp(WickedPostfixOp_MulAddVec);
    EmitByte(size);
  }

  // Scales vector by scalar (factor * vec).
  void ScaleVec(uint8_t size) {
    EmitOp(WickedPostfixOp_ScaleVec);
    EmitByte(size);
  }

  // Divides two vectors componentwise (lhs / rhs).
  void DivVec(uint8_t size) {
    EmitOp(WickedPostfixOp_DivVec);
    EmitByte(size);
  }

  // Negates each component of vector.
  void NegVec(uint8_t size) {
    EmitOp(WickedPostfixOp_NegVec);
    EmitByte(size);
  }

  // Inverts each component of vector (1 / v[i]).
  void InvVec(uint8_t size) {
    EmitOp(WickedPostfixOp_InvVec);
    EmitByte(size);
  }

  // Pushes scalar squared Euclidean norm ||v||^2.
  void NormSqVec(uint8_t size) {
    EmitOp(WickedPostfixOp_NormSqVec);
    EmitByte(size);
  }

  // Pushes scalar sum of vector components.
  void SumVec(uint8_t size) {
    EmitOp(WickedPostfixOp_SumVec);
    EmitByte(size);
  }

  // Linear interpolation between two vectors: (1-t)*v0 + t*v1.
  void LerpVec(uint8_t size) {
    EmitOp(WickedPostfixOp_LerpVec);
    EmitByte(size);
  }

  // Encodes a matrix multiplication C = A * B, where A is (arows x brows) and B is (brows x bcols).
  // Operand constraints:
  //   - arows: 1 to 4
  //   - brows: 1 to 8
  //   - bcols: must be brows (square), brows - 1 (projection down), brows + 1 (projection up), or 1 (reduction/dot product)
  //   - Matrix B is popped from the evaluation stack.
  void MulMat(uint8_t arows, uint8_t brows, uint8_t bcols) {
    assert(arows >= 1 && arows <= 4);
    assert(brows >= 1 && brows <= 8);
    uint8_t mode = 0;
    if (bcols == brows) {
      mode = 0;
    } else if (bcols == brows - 1) {
      mode = 1;
    } else if (bcols == brows + 1) {
      mode = 2;
    } else if (bcols == 1) {
      mode = 3;
    } else {
      assert(false && "Unsupported bcols dimension for MulMat: must be brows, brows-1, brows+1, or 1");
    }
    EmitOp(WickedPostfixOp_MulMat);
    uint8_t arg = ((arows - 1) & 0x03) |
                  (((brows - 1) & 0x07) << 2) |
                  ((mode & 0x03) << 5);  // Imm = 0
    EmitByte(arg);
  }

  // Encodes a matrix multiplication C = A * B with immediate matrix B, where A is on the stack (arows x brows)
  // and B is supplied as immediate float literals in the program stream (brows x bcols).
  // Operand constraints:
  //   - arows: 1 to 4
  //   - brows: 1 to 8
  //   - bcols: must be brows (square), brows - 1 (projection down), brows + 1 (projection up), or 1 (reduction/dot product)
  //   - b_matrix.size() == brows * bcols
  void MulMatImmediate(uint8_t arows, uint8_t brows, uint8_t bcols, std::span<const float> b_matrix) {
    assert(arows >= 1 && arows <= 4);
    assert(brows >= 1 && brows <= 8);
    assert(b_matrix.size() == static_cast<size_t>(brows) * bcols);
    uint8_t mode = 0;
    if (bcols == brows) {
      mode = 0;
    } else if (bcols == brows - 1) {
      mode = 1;
    } else if (bcols == brows + 1) {
      mode = 2;
    } else if (bcols == 1) {
      mode = 3;
    } else {
      assert(false && "Unsupported bcols dimension for MulMat: must be brows, brows-1, brows+1, or 1");
    }
    EmitOp(WickedPostfixOp_MulMat);
    uint8_t arg = ((arows - 1) & 0x03) |
                  (((brows - 1) & 0x07) << 2) |
                  ((mode & 0x03) << 5) |
                  0x80;  // Imm = 1
    EmitByte(arg);
    for (float f : b_matrix) {
      EmitFloat(f);
    }
  }

  // Encodes a matrix multiplication C = A * B with immediate matrix B supplied as an initializer list.
  void MulMatImmediate(uint8_t arows, uint8_t brows, uint8_t bcols, std::initializer_list<float> b_matrix) {
    MulMatImmediate(arows, brows, bcols, std::span<const float>(b_matrix));
  }

 private:
  constexpr uint16_t prog_offset() const { return sizeof(WickedPostfixHeader); }

  std::vector<uint8_t> prog_;
};

#endif
