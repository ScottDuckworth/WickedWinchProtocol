#pragma once

#include <ostream>

namespace wickedwinch::protocol {

enum class [[nodiscard]] EvalStatus {
  Ok,
  UndefinedOperation,
  IllegalOperation,
  StackOverflow,
  StackUnderflow,
  IntLiteralsUnderflow,
  FloatLiteralsUnderflow,
  TempOverflow,
};

inline constexpr const char* ToString(EvalStatus status) {
  switch (status) {
    case EvalStatus::Ok:                     return "Ok";
    case EvalStatus::UndefinedOperation:     return "UndefinedOperation";
    case EvalStatus::IllegalOperation:       return "IllegalOperation";
    case EvalStatus::StackOverflow:          return "StackOverflow";
    case EvalStatus::StackUnderflow:         return "StackUnderflow";
    case EvalStatus::IntLiteralsUnderflow:   return "IntLiteralsUnderflow";
    case EvalStatus::FloatLiteralsUnderflow: return "FloatLiteralsUnderflow";
    case EvalStatus::TempOverflow:           return "TempOverflow";
  }
  return "UNKNOWN";
}

inline std::ostream& operator<<(std::ostream& out, EvalStatus status) {
  return out << ToString(status);
}

}
