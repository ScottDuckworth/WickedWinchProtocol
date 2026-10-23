#pragma once

#include "WickedEvalStatus.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WickedPostfixHeader {
	uint16_t prog_size;
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
  WickedPostfixOp_LerpVec   = 37,
  WickedPostfixOp_LerpTable = 38,
} WickedPostfixOp_t;

const char* WickedPostfixOpToString(WickedPostfixOp_t op);
bool WickedPostfixRead(const uint8_t* data, size_t size, const uint8_t** prog_data, uint16_t* prog_size);
WickedEvalStatus_t WickedPostfixValidate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, uint16_t* minimum_stack_capacity, uint16_t* final_stack_size);
void WickedPostfixEvaluate(const uint8_t* prog_data, uint16_t prog_size, uint16_t initial_stack_size, float* stack_data);

static inline void WickedPostfixReadUnsafe(const uint8_t* data, const uint8_t** prog_data, uint16_t* prog_size) {
  const WickedPostfixHeader_t* header = (const WickedPostfixHeader_t*)data;
  if (prog_data) *prog_data = data + sizeof(WickedPostfixHeader_t);
  if (prog_size) *prog_size = header->prog_size;
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

	void add_b(uint8_t b) { prog_.push_back(b); }
	void add_i(uint8_t b) { add_b(b); }
	uint8_t* prog_data() { return prog_.data(); }
	const uint8_t* prog_data() const { return prog_.data(); }
	uint16_t prog_size() const { return static_cast<uint16_t>(prog_.size()); }

	void add_f(float f) {
		uint8_t bytes[sizeof(float)];
		memcpy(bytes, &f, sizeof(float));
		for (uint8_t b : bytes) prog_.push_back(b);
	}

  void Push(std::span<const float> values) {
    add_b(WickedPostfixOp_Push);
    add_b(static_cast<uint8_t>(values.size()));
    for (float value : values) add_f(value);
  }

  void Push(std::initializer_list<float> values) {
    Push(std::span<const float>(values));
  }

  void Pop(uint8_t n) {
    add_b(WickedPostfixOp_Pop);
    add_b(n);
  }

private:
	constexpr uint16_t prog_offset() const { return sizeof(WickedPostfixHeader); }

	std::vector<uint8_t> prog_;
};

#endif
