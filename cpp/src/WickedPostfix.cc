#include "WickedPostfix.h"
#include "WickedUtil.hh"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <iterator>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(float) == 4);

extern "C" const char* WickedPostfixOpToString(WickedPostfixOp_t op) {
  switch (op) {
    case WickedPostfixOp_Undefined: return "Undefined";
    case WickedPostfixOp_Ret:       return "Ret";
    case WickedPostfixOp_Jmp:       return "Jmp";
    case WickedPostfixOp_Jz:        return "Jz";
    case WickedPostfixOp_Jn:        return "Jn";
    case WickedPostfixOp_Push:      return "Push";
    case WickedPostfixOp_Pop:       return "Pop";
    case WickedPostfixOp_Dup:       return "Dup";
    case WickedPostfixOp_RotL:      return "RotL";
    case WickedPostfixOp_RotR:      return "RotR";
    case WickedPostfixOp_Rev:       return "Rev";
    case WickedPostfixOp_Transpose: return "Transpose";
    case WickedPostfixOp_AddU:      return "AddU";
    case WickedPostfixOp_SubU:      return "SubU";
    case WickedPostfixOp_MulU:      return "MulU";
    case WickedPostfixOp_MulAddU:   return "MulAddU";
    case WickedPostfixOp_DivU:      return "DivU";
    case WickedPostfixOp_ModU:      return "ModU";
    case WickedPostfixOp_AddI:      return "AddI";
    case WickedPostfixOp_SubI:      return "SubI";
    case WickedPostfixOp_MulI:      return "MulI";
    case WickedPostfixOp_MulAddI:   return "MulAddI";
    case WickedPostfixOp_DivI:      return "DivI";
    case WickedPostfixOp_ModI:      return "ModI";
    case WickedPostfixOp_NegI:      return "NegI";
    case WickedPostfixOp_AbsI:      return "AbsI";
    case WickedPostfixOp_ItoF:      return "ItoF";
    case WickedPostfixOp_FtoI:      return "FtoI";
    case WickedPostfixOp_AddF:      return "AddF";
    case WickedPostfixOp_SubF:      return "SubF";
    case WickedPostfixOp_MulF:      return "MulF";
    case WickedPostfixOp_MulAddF:   return "MulAddF";
    case WickedPostfixOp_DivF:      return "DivF";
    case WickedPostfixOp_ModF:      return "ModF";
    case WickedPostfixOp_NegF:      return "NegF";
    case WickedPostfixOp_AbsF:      return "AbsF";
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
    case WickedPostfixOp_LerpVec:   return "LerpVec";
    case WickedPostfixOp_PolyVec:   return "PolyVec";
    case WickedPostfixOp_PolyMat:   return "PolyMat";
  }
  return "UNKNOWN";
}

bool WickedPostfixWriter::Write(uint8_t* data, size_t size) const {
  if (size < data_size()) return false;
  auto* header = reinterpret_cast<WickedPostfixHeader*>(data);
  header->prog_size = prog_size();
  memcpy(data + prog_offset(), prog_data(), prog_size());
  return true;
}

extern "C" bool WickedPostfixRead(const uint8_t* data, size_t size, WickedPostfixEval_t* eval) {
  if (size < sizeof(WickedPostfixHeader)) return false;

	const WickedPostfixHeader* header = reinterpret_cast<const WickedPostfixHeader*>(data);
	uint16_t prog_size = header->prog_size;
	constexpr size_t prog_offset = sizeof(WickedPostfixHeader);
	size_t data_size = prog_offset + prog_size;
  if (size != data_size) return false;

  if (eval) {
    eval->prog_data = data + prog_offset;
    eval->prog_size = prog_size;
  }
  return true;
}

static WickedEvalStatus_t WickedPostfixEval_loadp(WickedPostfixEval_t* eval, uint8_t* v) {
  if (eval->prog_size - eval->pc < 1) return WickedEvalStatus_IllegalOperation;
  *v = eval->prog_data[eval->pc];
  ++eval->pc;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_loadpv(WickedPostfixEval_t* eval, uint16_t n, const uint8_t** v) {
  if (eval->prog_size - eval->pc < n) return WickedEvalStatus_IllegalOperation;
  *v = &eval->prog_data[eval->pc];
  eval->pc += n;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_pushbv(WickedPostfixEval_t* eval, uint16_t n, const uint8_t* v) {
  if (eval->stack_capacity - eval->stack_size < n) return WickedEvalStatus_StackOverflow;
  memcpy(&eval->stack_data[eval->stack_size], v, n);
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushu(WickedPostfixEval_t* eval, uint32_t v) {
  return WickedPostfixEval_pushbv(eval, sizeof(uint32_t), reinterpret_cast<uint8_t*>(&v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushi(WickedPostfixEval_t* eval, int32_t v) {
  return WickedPostfixEval_pushbv(eval, sizeof(int32_t), reinterpret_cast<uint8_t*>(&v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushf(WickedPostfixEval_t* eval, float v) {
  return WickedPostfixEval_pushbv(eval, sizeof(float), reinterpret_cast<uint8_t*>(&v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushuv(WickedPostfixEval_t* eval, uint16_t n, const uint32_t* v) {
  return WickedPostfixEval_pushbv(eval, n * sizeof(uint32_t), reinterpret_cast<const uint8_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushiv(WickedPostfixEval_t* eval, uint16_t n, const int32_t* v) {
  return WickedPostfixEval_pushbv(eval, n * sizeof(int32_t), reinterpret_cast<const uint8_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_pushfv(WickedPostfixEval_t* eval, uint16_t n, const float* v) {
  return WickedPostfixEval_pushbv(eval, n * sizeof(float), reinterpret_cast<const uint8_t*>(v));
}

static WickedEvalStatus_t WickedPostfixEval_allocbv(WickedPostfixEval_t* eval, uint16_t n, uint8_t** v) {
  if (eval->stack_capacity - eval->stack_size < n) return WickedEvalStatus_StackOverflow;
  *v = &eval->stack_data[eval->stack_size];
  eval->stack_size += n;
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_allocuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  return WickedPostfixEval_allocbv(eval, n * sizeof(uint32_t), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_allociv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_allocbv(eval, n * sizeof(int32_t), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_allocfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_allocbv(eval, n * sizeof(float), reinterpret_cast<uint8_t**>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popbv(WickedPostfixEval_t* eval, uint16_t n, uint8_t* v) {
  if (eval->stack_size < 1) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= n;
  memcpy(v, eval->stack_data + eval->stack_size, n);
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popu(WickedPostfixEval_t* eval, uint32_t* v) {
  return WickedPostfixEval_popbv(eval, sizeof(uint32_t), reinterpret_cast<uint8_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popi(WickedPostfixEval_t* eval, int32_t* v) {
  return WickedPostfixEval_popbv(eval, sizeof(int32_t), reinterpret_cast<uint8_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popf(WickedPostfixEval_t* eval, float* v) {
  return WickedPostfixEval_popbv(eval, sizeof(float), reinterpret_cast<uint8_t*>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popbp(WickedPostfixEval_t* eval, uint16_t n, uint8_t** v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  eval->stack_size -= n;
  *v = &eval->stack_data[eval->stack_size];
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  return WickedPostfixEval_popbp(eval, n * sizeof(uint32_t), reinterpret_cast<uint8_t**>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popiv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_popbp(eval, n * sizeof(int32_t), reinterpret_cast<uint8_t**>(v));
}

extern "C" WickedEvalStatus_t WickedPostfixEval_popfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_popbp(eval, n * sizeof(float), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_peekbp(WickedPostfixEval_t* eval, uint16_t n, uint8_t** v) {
  if (eval->stack_size < n) return WickedEvalStatus_StackUnderflow;
  *v = &eval->stack_data[eval->stack_size - n];
  return WickedEvalStatus_Ok;
}

static WickedEvalStatus_t WickedPostfixEval_peekuv(WickedPostfixEval_t* eval, uint16_t n, uint32_t** v) {
  return WickedPostfixEval_peekbp(eval, n * sizeof(uint32_t), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_peekiv(WickedPostfixEval_t* eval, uint16_t n, int32_t** v) {
  return WickedPostfixEval_peekbp(eval, n * sizeof(int32_t), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_peekfv(WickedPostfixEval_t* eval, uint16_t n, float** v) {
  return WickedPostfixEval_peekbp(eval, n * sizeof(float), reinterpret_cast<uint8_t**>(v));
}

static WickedEvalStatus_t WickedPostfixEval_jmp(WickedPostfixEval_t* eval, bool cond) {
  uint32_t target;
  WICKED_RETURN_IF_ERROR(WickedPostfixEval_popu(eval, &target));
  if (target > eval->prog_size) return WickedEvalStatus_IllegalAddress;
  if (cond) eval->pc = target;
  return WickedEvalStatus_Ok;
}

extern "C" WickedEvalStatus_t WickedPostfixEvaluate(WickedPostfixEval_t* eval) {
  eval->pc = 0;
  while (eval->pc < eval->prog_size) {
    const uint8_t op = eval->prog_data[eval->pc++];
    switch (op) {
    case WickedPostfixOp_Ret: {
      return WickedEvalStatus_Ok;
    }
    case WickedPostfixOp_Jmp: {
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_jmp(eval, true));
      break;
    }
    case WickedPostfixOp_Jz: {
      int32_t cond;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popi(eval, &cond));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_jmp(eval, cond == 0));
      break;
    }
    case WickedPostfixOp_Jn: {
      int32_t cond;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popi(eval, &cond));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_jmp(eval, cond < 0));
      break;
    }
    case WickedPostfixOp_Push: {
      uint8_t n;
      const uint8_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      uint16_t size = n * 4;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadpv(eval, size, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushbv(eval, size, v));
      break;
    }
    case WickedPostfixOp_Pop: {
      uint8_t n;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      uint32_t* discard;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, n, &discard));
      break;
    }
    case WickedPostfixOp_Dup: {
      uint8_t n;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekuv(eval, n + 1, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, *v));
      break;
    }
    case WickedPostfixOp_RotL: {
      uint8_t n;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      if (n <= 1) break;
      uint32_t* values;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekuv(eval, n, &values));
      uint32_t l = values[0];
      std::copy(&values[1], &values[n], &values[0]);
      values[n-1] = l;
      break;
    }
    case WickedPostfixOp_RotR: {
      uint8_t n;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      if (n <= 1) break;
      uint32_t* values;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekuv(eval, n, &values));
      uint32_t r = values[n-1];
      std::copy(&values[0], &values[n-1], &values[1]);
      values[0] = r;
      break;
    }
    case WickedPostfixOp_Rev: {
      uint8_t n;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &n));
      uint32_t* values;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekuv(eval, n, &values));
      std::reverse(values, values + n);
      break;
    }
    case WickedPostfixOp_Transpose: {
      uint8_t rows, cols;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &rows));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &cols));

      uint32_t* m;
      uint16_t size = rows * cols;
      uint32_t* temp_data = reinterpret_cast<uint32_t*>(eval->temp_data);
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, rows * cols, &m));
      if (size > eval->temp_capacity / sizeof(uint32_t)) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < rows; ++i) {
        for (uint8_t j = 0; j < cols; ++j) {
          size_t midx = cols * i + j;
          size_t tidx = rows * j + i;
          temp_data[tidx] = m[midx];
        }
      }
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushuv(eval, size, temp_data));
      break;
    }
    case WickedPostfixOp_AddU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_SubU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_MulU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAddU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 3, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_DivU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_ModU: {
      uint32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popuv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushu(eval, v[0] % v[1]));
      break;
    }
    case WickedPostfixOp_AddI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_SubI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_MulI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAddI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 3, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_DivI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_ModI: {
      int32_t* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popiv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v[0] % v[1]));
      break;
    }
    case WickedPostfixOp_NegI: {
      int32_t v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popi(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, -v));
      break;
    }
    case WickedPostfixOp_AbsI: {
      int32_t v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popi(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, v < 0 ? -v : v));
      break;
    }
    case WickedPostfixOp_ItoF: {
      int32_t v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popi(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, static_cast<float>(v)));
      break;
    }
    case WickedPostfixOp_FtoI: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushi(eval, static_cast<int32_t>(v)));
      break;
    }
    case WickedPostfixOp_AddF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, v[0] + v[1]));
      break;
    }
    case WickedPostfixOp_SubF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, v[0] - v[1]));
      break;
    }
    case WickedPostfixOp_MulF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, v[0] * v[1]));
      break;
    }
    case WickedPostfixOp_MulAddF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 3, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, v[0] * v[1] + v[2]));
      break;
    }
    case WickedPostfixOp_DivF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, v[0] / v[1]));
      break;
    }
    case WickedPostfixOp_ModF: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::fmod(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_NegF: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, -v));
      break;
    }
    case WickedPostfixOp_AbsF: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::abs(v)));
      break;
    }
    case WickedPostfixOp_Inv: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, 1.0f / v));
      break;
    }
    case WickedPostfixOp_Pow: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::pow(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_Sqrt: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::sqrt(v)));
      break;
    }
    case WickedPostfixOp_Exp: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::exp(v)));
      break;
    }
    case WickedPostfixOp_Ln: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::log(v)));
      break;
    }
    case WickedPostfixOp_Sin: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::sin(v)));
      break;
    }
    case WickedPostfixOp_Cos: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::cos(v)));
      break;
    }
    case WickedPostfixOp_Tan: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::tan(v)));
      break;
    }
    case WickedPostfixOp_Asin: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::asin(v)));
      break;
    }
    case WickedPostfixOp_Acos: {
      float v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::acos(v)));
      break;
    }
    case WickedPostfixOp_Atan2: {
      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, 2, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::atan2(v[0], v[1])));
      break;
    }
    case WickedPostfixOp_AddVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float *lhs, *rhs;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &rhs));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] += rhs[i];
      }
      break;
    }
    case WickedPostfixOp_SubVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float *lhs, *rhs;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &rhs));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] -= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float *lhs, *rhs;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &rhs));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekfv(eval, size, &lhs));
      for (uint8_t i = 0; i < size; ++i) {
        lhs[i] *= rhs[i];
      }
      break;
    }
    case WickedPostfixOp_MulAddVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float *a, *b, *c;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &c));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &b));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekfv(eval, size, &a));
      for (uint8_t i = 0; i < size; ++i) {
        a[i] = a[i] * b[i] + c[i];
      }
      break;
    }
    case WickedPostfixOp_ScaleVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float scalar, *v, *result;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &v));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &scalar));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_allocfv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = scalar * v[i];
      }
      break;
    }
    case WickedPostfixOp_NegVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_peekfv(eval, size, &v));
      for (uint8_t i = 0; i < size; ++i) {
        v[i] = -v[i];
      }
      break;
    }
    case WickedPostfixOp_NormVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float* v;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &v));
      float result = 0;
      for (uint8_t i = 0; i < size; ++i) {
        result += v[i] * v[i];
      }
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, std::sqrt(result)));
      break;
    }
    case WickedPostfixOp_MulMat: {
      uint8_t arows, brows, bcols;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &arows));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &brows));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &bcols));

      float *a, *b;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, brows * bcols, &b));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, arows * brows, &a));
      size_t temp_size = arows * bcols;
      float* temp_data = reinterpret_cast<float*>(eval->temp_data);
      if (temp_size > eval->temp_capacity / sizeof(float)) return WickedEvalStatus_TempOverflow;
      for (uint8_t i = 0; i < arows; ++i) {
        for (uint8_t j = 0; j < bcols; ++j) {
          float r = 0;
          for (uint8_t k = 0; k < brows; ++k) {
            size_t aidx = brows * i + k;
            size_t bidx = bcols * k + j;
            r += a[aidx] * b[bidx];
          }
          size_t cidx = bcols * i + j;
          temp_data[cidx] = r;
        }
      }
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushfv(eval, temp_size, temp_data));
      break;
    }
    case WickedPostfixOp_LerpVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float t, *v0, *v1, *result;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &v1));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &v0));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &t));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_allocfv(eval, size, &result));
      for (uint8_t i = 0; i < size; ++i) {
        result[i] = (1-t)*v0[i] + t*v1[i];
      }
      break;
    }
    case WickedPostfixOp_PolyVec: {
      uint8_t size;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &size));

      float result = 0;
      float p = 1;
      float t;
      float* coeff;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, size, &coeff));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &t));
      for (uint8_t n = 0; n < size; ++n) {
        result += coeff[n] * p;
        p *= t;
      }
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_pushf(eval, result));
      break;
    }
    case WickedPostfixOp_PolyMat: {
      uint8_t rows, cols;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &rows));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_loadp(eval, &cols));

      float t, *coeff, *result;
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popfv(eval, rows * cols, &coeff));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_popf(eval, &t));
      WICKED_RETURN_IF_ERROR(WickedPostfixEval_allocfv(eval, cols, &result));
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
    default:
      return WickedEvalStatus_UndefinedOperation;
    }
  }
  return WickedEvalStatus_Ok;
}
