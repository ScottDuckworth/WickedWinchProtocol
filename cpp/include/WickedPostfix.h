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

typedef enum WickedPostfixOp {
  WickedPostfixOp_Undefined = 0,
  WickedPostfixOp_Push      = 1,
  WickedPostfixOp_Pop       = 2,
  WickedPostfixOp_Dup       = 3,
  WickedPostfixOp_RotL      = 4,
  WickedPostfixOp_RotR      = 5,
  WickedPostfixOp_Rev       = 6,
  WickedPostfixOp_Transpose = 7,
  WickedPostfixOp_Store     = 8,
  WickedPostfixOp_Load      = 9,
  WickedPostfixOp_Add       = 10,
  WickedPostfixOp_Sub       = 11,
  WickedPostfixOp_Mul       = 12,
  WickedPostfixOp_MulAdd    = 13,
  WickedPostfixOp_Div       = 14,
  WickedPostfixOp_Mod       = 15,
  WickedPostfixOp_Neg       = 16,
  WickedPostfixOp_Abs       = 17,
  WickedPostfixOp_Inv       = 18,
  WickedPostfixOp_Pow       = 19,
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
  WickedPostfixOp_NegVec    = 34,
  WickedPostfixOp_NormVec   = 35,
  WickedPostfixOp_MulMat    = 36,
  WickedPostfixOp_PolyVec   = 37,
  WickedPostfixOp_PolyMat   = 38,
  WickedPostfixOp_LerpVec   = 39,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);
bool WickedPostfixRead(const uint8_t* data, size_t size, const uint8_t** prog_data, uint16_t* prog_size);
WickedEvalStatus_t WickedPostfixValidate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, uint16_t* minimum_stack_capacity, uint16_t* final_stack_size);
void WickedPostfixEvaluate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, float* stack_data);

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

#include <cstring>
#include <initializer_list>
#include <ostream>
#include <span>
#include <vector>

inline std::ostream& operator<<(std::ostream& out, WickedPostfixOp_t op) {
  return out << WickedPostfixOpToString(op);
}

class WickedPostfixWriter {
 public:
  uint8_t* prog_data() { return prog_.data(); }
  const uint8_t* prog_data() const { return prog_.data(); }
  uint16_t prog_size() const { return static_cast<uint16_t>(prog_.size()); }
  uint16_t data_size() const { return prog_offset() + prog_size(); }

  bool Write(uint8_t* data, size_t size) const;
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

  void clear() {
    prog_.clear();
  }

  void EmitByte(uint8_t b) { prog_.push_back(b); }

  void EmitOp(WickedPostfixOp_t op) { EmitByte(static_cast<uint8_t>(op)); }

  void EmitFloat(float f) {
    uint8_t bytes[sizeof(float)];
    memcpy(bytes, &f, sizeof(float));
    for (uint8_t b : bytes) prog_.push_back(b);
  }

  void Push(float value) {
    EmitOp(WickedPostfixOp_Push);
    EmitByte(1);
    EmitFloat(value);
  }

  void Push(std::span<const float> values) {
    EmitOp(WickedPostfixOp_Push);
    EmitByte(static_cast<uint8_t>(values.size()));
    for (float value : values) EmitFloat(value);
  }

  void Push(std::initializer_list<float> values) {
    Push(std::span<const float>(values));
  }

  void Pop(uint8_t n) {
    EmitOp(WickedPostfixOp_Pop);
    EmitByte(n);
  }

  void Dup(uint8_t n) {
    EmitOp(WickedPostfixOp_Dup);
    EmitByte(n);
  }

  void RotL(uint8_t n) {
    EmitOp(WickedPostfixOp_RotL);
    EmitByte(n);
  }

  void RotR(uint8_t n) {
    EmitOp(WickedPostfixOp_RotR);
    EmitByte(n);
  }

  void Rev(uint8_t n) {
    EmitOp(WickedPostfixOp_Rev);
    EmitByte(n);
  }

  void Transpose(uint8_t rows, uint8_t cols) {
    EmitOp(WickedPostfixOp_Transpose);
    EmitByte(rows);
    EmitByte(cols);
  }

  void Store(uint8_t slot, uint8_t size = 1, bool pop = false) {
    EmitOp(WickedPostfixOp_Store);
    uint8_t arg = (slot & 0x03) | ((pop ? 1 : 0) << 4) | (((size - 1) & 0x03) << 6);
    EmitByte(arg);
  }

  void Load(uint8_t slot, uint8_t size = 1) {
    EmitOp(WickedPostfixOp_Load);
    uint8_t arg = (slot & 0x03) | (((size - 1) & 0x03) << 6);
    EmitByte(arg);
  }

  void Add() { EmitOp(WickedPostfixOp_Add); }
  void Sub() { EmitOp(WickedPostfixOp_Sub); }
  void Mul() { EmitOp(WickedPostfixOp_Mul); }
  void MulAdd() { EmitOp(WickedPostfixOp_MulAdd); }
  void Div() { EmitOp(WickedPostfixOp_Div); }
  void Mod() { EmitOp(WickedPostfixOp_Mod); }
  void Neg() { EmitOp(WickedPostfixOp_Neg); }
  void Abs() { EmitOp(WickedPostfixOp_Abs); }
  void Inv() { EmitOp(WickedPostfixOp_Inv); }
  void Pow() { EmitOp(WickedPostfixOp_Pow); }
  void Sqrt() { EmitOp(WickedPostfixOp_Sqrt); }
  void Exp() { EmitOp(WickedPostfixOp_Exp); }
  void Ln() { EmitOp(WickedPostfixOp_Ln); }
  void Sin() { EmitOp(WickedPostfixOp_Sin); }
  void Cos() { EmitOp(WickedPostfixOp_Cos); }
  void Tan() { EmitOp(WickedPostfixOp_Tan); }
  void Asin() { EmitOp(WickedPostfixOp_Asin); }
  void Acos() { EmitOp(WickedPostfixOp_Acos); }
  void Atan2() { EmitOp(WickedPostfixOp_Atan2); }

  void PolyVec(std::span<const float> coeffs) {
    EmitOp(WickedPostfixOp_PolyVec);
    EmitByte(static_cast<uint8_t>(coeffs.size()));
    for (size_t i = coeffs.size(); i > 0; --i) {
      EmitFloat(coeffs[i - 1]);
    }
  }

  void PolyVec(std::initializer_list<float> coeffs) {
    PolyVec(std::span<const float>(coeffs));
  }

  void PolyMat(uint8_t rows, uint8_t cols, std::span<const float> coeffs) {
    EmitOp(WickedPostfixOp_PolyMat);
    EmitByte(rows);
    EmitByte(cols);
    for (size_t i = rows; i > 0; --i) {
      size_t row_offset = (i - 1) * cols;
      for (size_t j = 0; j < cols; ++j) {
        EmitFloat(coeffs[row_offset + j]);
      }
    }
  }

  void PolyMat(uint8_t rows, uint8_t cols, std::initializer_list<float> coeffs) {
    PolyMat(rows, cols, std::span<const float>(coeffs));
  }

  void AddVec(uint8_t size) {
    EmitOp(WickedPostfixOp_AddVec);
    EmitByte(size);
  }

  void SubVec(uint8_t size) {
    EmitOp(WickedPostfixOp_SubVec);
    EmitByte(size);
  }

  void MulVec(uint8_t size) {
    EmitOp(WickedPostfixOp_MulVec);
    EmitByte(size);
  }

  void MulAddVec(uint8_t size) {
    EmitOp(WickedPostfixOp_MulAddVec);
    EmitByte(size);
  }

  void ScaleVec(uint8_t size) {
    EmitOp(WickedPostfixOp_ScaleVec);
    EmitByte(size);
  }

  void NegVec(uint8_t size) {
    EmitOp(WickedPostfixOp_NegVec);
    EmitByte(size);
  }

  void NormVec(uint8_t size) {
    EmitOp(WickedPostfixOp_NormVec);
    EmitByte(size);
  }

  void MulMat(uint8_t arows, uint8_t brows, uint8_t bcols) {
    EmitOp(WickedPostfixOp_MulMat);
    EmitByte(arows);
    EmitByte(brows);
    EmitByte(bcols);
  }

  void LerpVec(uint8_t size) {
    EmitOp(WickedPostfixOp_LerpVec);
    EmitByte(size);
  }

 private:
  constexpr uint16_t prog_offset() const { return sizeof(WickedPostfixHeader); }

  std::vector<uint8_t> prog_;
};

#endif
