#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WickedPostfixHeader {
	uint8_t op_size;
	uint8_t i_size;
	uint16_t f_size;
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
  WickedPostfixOp_Lut       = 38,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);

typedef struct WickedPostfixEval {
  const uint8_t* op_head;
  const uint8_t* i_head;
  const float* f_head;
  uint8_t op_size;
  uint8_t i_size;
  uint16_t f_size;

  float* stack_data;
  float* temp_data;
  uint8_t stack_size;
  uint8_t stack_capacity;
  uint8_t temp_capacity;
} WickedPostfixEval_t;

void WickedPostfixEval_reset(WickedPostfixEval_t* eval);
WickedEvalStatus_t WickedPostfixEval_push(WickedPostfixEval_t* eval, float v);
WickedEvalStatus_t WickedPostfixEval_pushv(WickedPostfixEval_t* eval, const float* v, size_t size);
WickedEvalStatus_t WickedPostfixEval_pop(WickedPostfixEval_t* eval, float* v);
WickedEvalStatus_t WickedPostfixEval_popv(WickedPostfixEval_t* eval, float* v, size_t n);

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
	uint16_t data_size() const { return f_offset() + f_size() * 4; }
	bool Write(uint8_t* data, size_t size) const;
  std::vector<uint8_t> Write() const {
    std::vector<uint8_t> buffer(data_size());
    Write(buffer.data(), buffer.size());
    return buffer;
  }

	void clear() {
		op_.clear();
		i_.clear();
		f_.clear();
	}

	void add_op(uint8_t op) { op_.push_back(op); }
	uint8_t* op_data() { return op_.data(); }
	const uint8_t* op_data() const { return op_.data(); }
	uint8_t op_size() const { return op_.size(); }
	uint8_t op(uint8_t index) const { return op_data()[index]; }
	uint8_t& op(uint8_t index) { return op_data()[index]; }

	void add_i(uint8_t i) { i_.push_back(i); }
	uint8_t* i_data() { return i_.data(); }
	const uint8_t* i_data() const { return i_.data(); }
	uint8_t i_size() const { return i_.size(); }
	uint8_t i(uint8_t index) const { return i_data()[index]; }
	uint8_t& i(uint8_t index) { return i_data()[index]; }

	void add_f(float f) { f_.push_back(f); }
	float* f_data() { return f_.data(); }
	const float* f_data() const { return f_.data(); }
	uint16_t f_size() const { return f_.size(); }
	float f(uint16_t index) const { return f_data()[index]; }
	float& f(uint16_t index) { return f_data()[index]; }

  void Push(std::span<const float> values) {
    add_op(WickedPostfixOp_Push);
    add_i(values.size());
    for (float value : values) add_f(value);
  }

  void Pop(uint8_t n) {
    add_op(WickedPostfixOp_Pop);
    add_i(n);
  }

private:
	constexpr uint16_t op_offset() const { return sizeof(WickedPostfixHeader); }

	uint16_t i_offset() const { return op_offset() + op_size(); }

	uint16_t f_offset() const {
		uint16_t offset = op_offset() + op_size() + i_size();
		return (offset + uint16_t(3)) & ~uint16_t(3);
	}

	std::vector<uint8_t> op_;
	std::vector<uint8_t> i_;
	std::vector<float> f_;
};

#endif
