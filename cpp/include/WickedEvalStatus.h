#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WickedEvalStatus {
  WickedEvalStatus_Ok,
  WickedEvalStatus_UndefinedOperation,
  WickedEvalStatus_IllegalOperation,
  WickedEvalStatus_IllegalAddress,
  WickedEvalStatus_StackOverflow,
  WickedEvalStatus_StackUnderflow,
  WickedEvalStatus_TempOverflow,
} WickedEvalStatus_t;

const char* WickedEvalStatus_ToString(WickedEvalStatus_t status);

#ifdef __cplusplus
}

#include <ostream>

inline std::ostream& operator<<(std::ostream& out, WickedEvalStatus_t status) {
  return out << WickedEvalStatus_ToString(status);
}

#endif
