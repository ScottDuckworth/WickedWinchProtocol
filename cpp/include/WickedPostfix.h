#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WickedPostfixHeader {
	uint16_t i_size;
	uint16_t d_size;
} WickedPostfixHeader_t;

typedef enum WickedPostfixOp {
  WickedPostfixOp_Undefined = 0,
  WickedPostfixOp_Push      = 1,
  WickedPostfixOp_Pop       = 2,
  WickedPostfixOp_Dup       = 3,
  WickedPostfixOp_RotL      = 4,
  WickedPostfixOp_RotR      = 5,
  WickedPostfixOp_Rev       = 6,
  WickedPostfixOp_Transpose = 7,
  WickedPostfixOp_Add       = 8,
  WickedPostfixOp_Sub       = 9,
  WickedPostfixOp_Mul       = 10,
  WickedPostfixOp_MulAdd    = 11,
  WickedPostfixOp_Div       = 12,
  WickedPostfixOp_Mod       = 13,
  WickedPostfixOp_Neg       = 14,
  WickedPostfixOp_Abs       = 15,
  WickedPostfixOp_Inv       = 16,
  WickedPostfixOp_Pow       = 17,
  WickedPostfixOp_Sqrt      = 18,
  WickedPostfixOp_Exp       = 19,
  WickedPostfixOp_Ln        = 20,
  WickedPostfixOp_Sin       = 21,
  WickedPostfixOp_Cos       = 22,
  WickedPostfixOp_Tan       = 23,
  WickedPostfixOp_Asin      = 24,
  WickedPostfixOp_Acos      = 25,
  WickedPostfixOp_Atan2     = 26,
  WickedPostfixOp_AddVec    = 27,
  WickedPostfixOp_SubVec    = 28,
  WickedPostfixOp_MulVec    = 29,
  WickedPostfixOp_MulAddVec = 30,
  WickedPostfixOp_ScaleVec  = 31,
  WickedPostfixOp_NegVec    = 32,
  WickedPostfixOp_NormVec   = 33,
  WickedPostfixOp_MulMat    = 34,
  WickedPostfixOp_PolyVec   = 35,
  WickedPostfixOp_PolyMat   = 36,
  WickedPostfixOp_Lerp      = 37,
  WickedPostfixOp_LerpTable = 38,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);

typedef struct WickedPostfixEval {
  const uint8_t* i_data;
  uint16_t i_size;
  uint16_t i_idx;

  const float* d_data;
  uint16_t d_size;
  uint16_t d_idx;

  float* stack_data;
  uint16_t stack_size;
  uint16_t stack_capacity;

  float* temp_data;
  uint16_t temp_capacity;
} WickedPostfixEval_t;

WickedEvalStatus_t WickedPostfixEval_push(WickedPostfixEval_t* eval, float v);
WickedEvalStatus_t WickedPostfixEval_pushv(WickedPostfixEval_t* eval, const float* v, uint16_t size);
WickedEvalStatus_t WickedPostfixEval_pop(WickedPostfixEval_t* eval, float* v);
WickedEvalStatus_t WickedPostfixEval_popv(WickedPostfixEval_t* eval, float* v, uint16_t n);

bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval);
WickedEvalStatus_t WickedPostfixEvaluate(WickedPostfixEval_t* eval);

#ifdef __cplusplus
}

#include <ostream>
#include <span>
#include <vector>

inline std::ostream& operator<<(std::ostream& out, WickedPostfixOp_t op) {
  return out << WickedPostfixOpToString(op);
}

class WickedPostfixWriter {
public:
	uint16_t data_size() const { return d_offset() + d_size() * 4; }
	bool Write(uint8_t* data, size_t size) const;
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

	void clear() {
		i_.clear();
		d_.clear();
	}

	void add_i(uint8_t i) { i_.push_back(i); }
	uint8_t* i_data() { return i_.data(); }
	const uint8_t* i_data() const { return i_.data(); }
	uint8_t i_size() const { return i_.size(); }
	uint8_t i(uint8_t index) const { return i_data()[index]; }
	uint8_t& i(uint8_t index) { return i_data()[index]; }

	void add_f(float f) { d_.push_back(f); }
	float* d_data() { return d_.data(); }
	const float* d_data() const { return d_.data(); }
	uint16_t d_size() const { return d_.size(); }
	float f(uint16_t index) const { return d_data()[index]; }
	float& f(uint16_t index) { return d_data()[index]; }

  void Push(std::span<const float> values) {
    add_i(WickedPostfixOp_Push);
    add_i(values.size());
    for (float value : values) add_f(value);
  }

  void Pop(uint8_t n) {
    add_i(WickedPostfixOp_Pop);
    add_i(n);
  }

private:
	constexpr uint16_t i_offset() const { return sizeof(WickedPostfixHeader); }

	uint16_t d_offset() const {
		uint16_t offset = i_offset() + i_size();
		return (offset + uint16_t(3)) & ~uint16_t(3);
	}

	std::vector<uint8_t> i_;
	std::vector<float> d_;
};

#endif
