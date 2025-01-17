#include "WickedEvalStatus.h"

extern "C" const char* WickedEvalStatus_ToString(WickedEvalStatus_t status) {
  switch (status) {
    case WickedEvalStatus_Ok:                 return "Ok";
    case WickedEvalStatus_UndefinedOperation: return "Undefined Operation";
    case WickedEvalStatus_IllegalOperation:   return "Illegal Operation";
    case WickedEvalStatus_IllegalAddress:     return "Illegal Address";
    case WickedEvalStatus_StackOverflow:      return "Stack Overflow";
    case WickedEvalStatus_StackUnderflow:     return "Stack Underflow";
    case WickedEvalStatus_TempOverflow:       return "Temp Overflow";
  }
  return "UNKNOWN";
}
