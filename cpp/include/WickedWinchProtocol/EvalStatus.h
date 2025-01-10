#pragma once

enum WickedEvalStatus {
  WickedEvalStatus_Ok,
  WickedEvalStatus_UndefinedOperation,
  WickedEvalStatus_IllegalOperation,
  WickedEvalStatus_StackOverflow,
  WickedEvalStatus_StackUnderflow,
  WickedEvalStatus_IntLiteralsUnderflow,
  WickedEvalStatus_FloatLiteralsUnderflow,
  WickedEvalStatus_TempOverflow,
};

inline constexpr const char* WickedEvalStatus_ToString(WickedEvalStatus status) {
  switch (status) {
    case WickedEvalStatus_Ok:                     return "Ok";
    case WickedEvalStatus_UndefinedOperation:     return "Undefined Operation";
    case WickedEvalStatus_IllegalOperation:       return "Illegal Operation";
    case WickedEvalStatus_StackOverflow:          return "Stack Overflow";
    case WickedEvalStatus_StackUnderflow:         return "Stack Underflow";
    case WickedEvalStatus_IntLiteralsUnderflow:   return "Int Literals Underflow";
    case WickedEvalStatus_FloatLiteralsUnderflow: return "Float Literals Underflow";
    case WickedEvalStatus_TempOverflow:           return "Temp Overflow";
  }
  return "UNKNOWN";
}

#ifdef __cplusplus

#include <ostream>

inline std::ostream& operator<<(std::ostream& out, WickedEvalStatus status) {
  return out << WickedEvalStatus_ToString(status);
}

#endif
