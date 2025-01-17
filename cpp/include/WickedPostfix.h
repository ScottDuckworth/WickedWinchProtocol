#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WICKED_RETURN_IF_ERROR(expr) do { WickedEvalStatus_t status = expr; if (status != WickedEvalStatus_Ok) return status; } while (0)

typedef struct WickedPostfixHeader {
	uint16_t prog_size;
} WickedPostfixHeader_t;

typedef enum WickedPostfixOp {
  WickedPostfixOp_Undefined = 0,
  WickedPostfixOp_Ret       = 1,
  WickedPostfixOp_Jmp       = 2,
  WickedPostfixOp_Jz        = 3,
  WickedPostfixOp_Jn        = 4,
  WickedPostfixOp_Push      = 5,
  WickedPostfixOp_Pop       = 6,

  WickedPostfixOp_Dup       = 10,
  WickedPostfixOp_RotL      = 11,
  WickedPostfixOp_RotR      = 12,
  WickedPostfixOp_Rev       = 13,
  WickedPostfixOp_Transpose = 14,

  WickedPostfixOp_AddU      = 20,
  WickedPostfixOp_SubU      = 21,
  WickedPostfixOp_MulU      = 22,
  WickedPostfixOp_MulAddU   = 23,
  WickedPostfixOp_DivU      = 24,
  WickedPostfixOp_ModU      = 25,

  WickedPostfixOp_AddI      = 30,
  WickedPostfixOp_SubI      = 31,
  WickedPostfixOp_MulI      = 32,
  WickedPostfixOp_MulAddI   = 33,
  WickedPostfixOp_DivI      = 34,
  WickedPostfixOp_ModI      = 35,
  WickedPostfixOp_NegI      = 36,
  WickedPostfixOp_AbsI      = 37,
  WickedPostfixOp_ItoF      = 38,
  WickedPostfixOp_FtoI      = 39,

  WickedPostfixOp_AddF      = 40,
  WickedPostfixOp_SubF      = 41,
  WickedPostfixOp_MulF      = 42,
  WickedPostfixOp_MulAddF   = 43,
  WickedPostfixOp_DivF      = 44,
  WickedPostfixOp_ModF      = 45,
  WickedPostfixOp_NegF      = 46,
  WickedPostfixOp_AbsF      = 47,
  WickedPostfixOp_Inv       = 48,
  WickedPostfixOp_Pow       = 49,
  WickedPostfixOp_Sqrt      = 50,
  WickedPostfixOp_Exp       = 51,
  WickedPostfixOp_Ln        = 52,
  WickedPostfixOp_Sin       = 53,
  WickedPostfixOp_Cos       = 54,
  WickedPostfixOp_Tan       = 55,
  WickedPostfixOp_Asin      = 56,
  WickedPostfixOp_Acos      = 57,
  WickedPostfixOp_Atan2     = 58,

  WickedPostfixOp_AddVec    = 60,
  WickedPostfixOp_SubVec    = 61,
  WickedPostfixOp_MulVec    = 62,
  WickedPostfixOp_MulAddVec = 63,
  WickedPostfixOp_ScaleVec  = 64,
  WickedPostfixOp_NegVec    = 65,
  WickedPostfixOp_NormVec   = 66,
  WickedPostfixOp_MulMat    = 67,

  WickedPostfixOp_LerpVec   = 70,
  WickedPostfixOp_PolyVec   = 71,
  WickedPostfixOp_PolyMat   = 72,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);

typedef struct WickedPostfixEval {
  const uint8_t* prog_data;
  uint16_t prog_size;
  uint16_t pc;

  uint8_t* stack_data;
  uint16_t stack_size;
  uint16_t stack_capacity;

  uint8_t* temp_data;
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
#include <initializer_list>
#include <ostream>
#include <span>
#include <vector>

inline std::ostream& operator<<(std::ostream& out, WickedPostfixOp_t op) {
  return out << WickedPostfixOpToString(op);
}

class WickedPostfixWriter {
public:
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

	uint8_t* prog_data() { return prog_.data(); }
	const uint8_t* prog_data() const { return prog_.data(); }
	uint8_t prog_size() const { return prog_.size(); }

	void add_b(uint8_t v) { prog_.push_back(v); }

	void add_u(uint32_t v) {
    uint8_t bytes[sizeof(uint32_t)];
    memcpy(bytes, &v, sizeof(uint32_t));
    for (uint8_t b : bytes) prog_.push_back(b);
  }

	void add_i(int32_t v) { add_u(std::bit_cast<uint32_t>(v)); }
	void add_f(float v) { add_u(std::bit_cast<uint32_t>(v)); }

  void PushU(std::span<const uint32_t> values) {
    add_b(WickedPostfixOp_Push);
    add_b(values.size());
    for (float value : values) add_u(value);
  }

  void PushU(std::initializer_list<uint32_t> values) {
    PushU(std::span<const uint32_t>(values));
  }

  void PushI(std::span<const int32_t> values) {
    add_b(WickedPostfixOp_Push);
    add_b(values.size());
    for (float value : values) add_i(value);
  }

  void PushI(std::initializer_list<int32_t> values) {
    PushI(std::span<const int32_t>(values));
  }

  void PushF(std::span<const float> values) {
    add_b(WickedPostfixOp_Push);
    add_b(values.size());
    for (float value : values) add_f(value);
  }

  void PushF(std::initializer_list<float> values) {
    PushF(std::span<const float>(values));
  }

  void Pop(uint8_t n) {
    add_b(WickedPostfixOp_Pop);
    add_b(n);
  }

private:
	constexpr uint16_t prog_offset() const { return sizeof(WickedPostfixHeader_t); }

	std::vector<uint8_t> prog_;
};

#endif
