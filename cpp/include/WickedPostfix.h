#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WICKED_RETURN_IF_ERROR(expr) do { WickedEvalStatus_t status = expr; if (status != WickedEvalStatus_Ok) return status; } while (0)
#define WICKED_JUMP_TARGET(pc, dc) (((uint32_t) pc) | (((uint32_t) dc) << 16))

typedef struct WickedPostfixHeader {
	uint16_t p_size;
	uint16_t d_size;
} WickedPostfixHeader_t;

typedef enum WickedPostfixOp {
  WickedPostfixOp_Undefined = 0,
  WickedPostfixOp_Ret       = 201,
  WickedPostfixOp_Jmp       = 202,
  WickedPostfixOp_Jz        = 203,
  WickedPostfixOp_Jn        = 204,

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

  WickedPostfixOp_AddI       = 101,
  WickedPostfixOp_SubI       = 102,
  WickedPostfixOp_MulI       = 103,
  WickedPostfixOp_MulAddI    = 104,
  WickedPostfixOp_DivI       = 105,
  WickedPostfixOp_ModI       = 106,
  WickedPostfixOp_NegI       = 107,
  WickedPostfixOp_AbsI       = 108,

  WickedPostfixOp_AddU       = 111,
  WickedPostfixOp_SubU       = 112,
  WickedPostfixOp_MulU       = 113,
  WickedPostfixOp_MulAddU    = 114,
  WickedPostfixOp_DivU       = 115,
  WickedPostfixOp_ModU       = 116,

  WickedPostfixOp_ItoF       = 120,
  WickedPostfixOp_FtoI       = 121,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);

typedef struct WickedPostfixEval {
  const uint8_t* p_data;
  const uint32_t* d_data;
  uint16_t p_size;
  uint16_t d_size;

  uint16_t pc;
  uint16_t dc;

  uint32_t* stack_data;
  uint16_t stack_size;
  uint16_t stack_capacity;

  uint32_t* temp_data;
  uint16_t temp_capacity;
} WickedPostfixEval_t;

WickedEvalStatus_t WickedPostfixEval_pushu(WickedPostfixEval_t* eval, uint32_t v);
WickedEvalStatus_t WickedPostfixEval_pushi(WickedPostfixEval_t* eval, int32_t v);
WickedEvalStatus_t WickedPostfixEval_pushf(WickedPostfixEval_t* eval, float v);

WickedEvalStatus_t WickedPostfixEval_pushuv(WickedPostfixEval_t* eval, uint16_t n, const uint32_t* v);
WickedEvalStatus_t WickedPostfixEval_pushiv(WickedPostfixEval_t* eval, uint16_t n, const int32_t* v);
WickedEvalStatus_t WickedPostfixEval_pushfv(WickedPostfixEval_t* eval, uint16_t n, const float* v);

WickedEvalStatus_t WickedPostfixEval_popu(WickedPostfixEval_t* eval, uint32_t* v);
WickedEvalStatus_t WickedPostfixEval_popi(WickedPostfixEval_t* eval, int32_t* v);
WickedEvalStatus_t WickedPostfixEval_popf(WickedPostfixEval_t* eval, float* v);

WickedEvalStatus_t WickedPostfixEval_popuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v);
WickedEvalStatus_t WickedPostfixEval_popiv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v);
WickedEvalStatus_t WickedPostfixEval_popfv(WickedPostfixEval_t* eval, uint16_t n, float** v);

bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval);
WickedEvalStatus_t WickedPostfixEvaluate(WickedPostfixEval_t* eval);

#ifdef __cplusplus
}

#include <bit>
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

	uint8_t* p_data() { return i_.data(); }
	const uint8_t* p_data() const { return i_.data(); }
	uint8_t p_size() const { return i_.size(); }

	uint32_t* d_data() { return d_.data(); }
	const uint32_t* d_data() const { return d_.data(); }
	uint16_t d_size() const { return d_.size(); }

	void add_p(uint8_t v) { i_.push_back(v); }
	void add_u(uint32_t v) { d_.push_back(v); }
	void add_i(int32_t v) { add_u(std::bit_cast<uint32_t>(v)); }
	void add_f(float v) { add_u(std::bit_cast<uint32_t>(v)); }

  void Push(std::span<const float> values) {
    add_p(WickedPostfixOp_Push);
    add_p(values.size());
    for (float value : values) add_f(value);
  }

  void Pop(uint8_t n) {
    add_p(WickedPostfixOp_Pop);
    add_p(n);
  }

private:
	constexpr uint16_t i_offset() const { return sizeof(WickedPostfixHeader_t); }

	uint16_t d_offset() const {
		uint16_t offset = i_offset() + p_size();
		return (offset + uint16_t(3)) & ~uint16_t(3);
	}

	std::vector<uint8_t> i_;
	std::vector<uint32_t> d_;
};

#endif
