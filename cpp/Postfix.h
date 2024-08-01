#pragma once

#include "EvalStatus.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <initializer_list>
#include <span>
#include <vector>

namespace wickedwinch::protocol {

static_assert(std::endian::native == std::endian::little);

enum class PostfixOp : uint8_t {
  Undefined = 0,
  Push      = 1,
  Pop       = 2,
  Dup       = 3,
  RotL      = 4,
  RotR      = 5,
  Rev       = 6,
  Transpose = 7,
  Add       = 8,
  Sub       = 9,
  Mul       = 10,
  MulAdd    = 11,
  Div       = 12,
  Mod       = 13,
  Neg       = 14,
  Abs       = 15,
  Inv       = 16,
  Pow       = 17,
  Sqrt      = 18,
  Exp       = 19,
  Ln        = 20,
  Sin       = 21,
  Cos       = 22,
  Tan       = 23,
  Asin      = 24,
  Acos      = 25,
  Atan2     = 26,
  AddVec    = 27,
  SubVec    = 28,
  MulVec    = 29,
  MulAddVec = 30,
  ScaleVec  = 31,
  NegVec    = 32,
  NormVec   = 33,
  MulMat    = 34,
  PolyVec   = 35,
  PolyMat   = 36,
  Lerp      = 37,
  Lut       = 38,
};

struct PostfixHeader {
	uint8_t op_size;
	uint8_t i_size;
	uint16_t f_size;
};

static_assert(sizeof(PostfixOp) == 1);
static_assert(sizeof(PostfixHeader) == 4);
static_assert(sizeof(float) == 4);

struct PostfixEvalContext {
  const PostfixOp* op_head;
  const uint8_t* i_head;
  const float* f_head;
  float* stack_data;
  uint8_t op_size;
  uint8_t i_size;
  uint16_t f_size;
  size_t stack_size;
  size_t stack_capacity;
  std::vector<float> temp;

  EvalStatus push(float v);
  EvalStatus pushv(std::span<const float> v);
  EvalStatus pushf(size_t n);
  EvalStatus allocv(size_t n, std::span<float>& v);
  EvalStatus pop(float& v);
  EvalStatus popv(size_t n, std::span<float>& v);
  EvalStatus peek(float& v);
  EvalStatus peekv(size_t n, std::span<float>& v);
  EvalStatus geti(uint8_t& n);
  EvalStatus implicitPushArg(uint8_t& arg, size_t multiple, uint8_t instances);
  EvalStatus Eval();
};

class PostfixReader {
public:
  bool Read(std::span<const uint8_t> buffer) { return Read(buffer.data(), buffer.size()); }
  bool Read(const uint8_t* data, size_t size);

	const PostfixOp* op_data() const { return reinterpret_cast<const PostfixOp*>(buffer_ + op_offset()); }
	uint8_t op_size() const { return header()->op_size; }
	PostfixOp op(uint8_t index) const { return op_data()[index]; }

	const uint8_t* i_data() const { return buffer_ + i_offset(); }
	uint8_t i_size() const { return header()->i_size; }
	uint8_t i(uint8_t index) const { return i_data()[index]; }

	const float* f_data() const {
		return reinterpret_cast<const float*>(buffer_ + f_offset());
	}
	uint16_t f_size() const { return header()->f_size; }
	float f(uint16_t index) const { return f_data()[index]; }

	uint16_t data_size() const { return f_offset() + f_size() * 4; }

private:
	const PostfixHeader* header() const { return reinterpret_cast<const PostfixHeader*>(buffer_); }

	constexpr uint16_t op_offset() const { return sizeof(PostfixHeader); }

	uint16_t i_offset() const { return op_offset() + op_size(); }

	uint16_t f_offset() const {
		uint16_t offset = op_offset() + op_size() + i_size();
		return (offset + uint16_t(3)) & ~uint16_t(3);
	}

	const uint8_t* buffer_ = nullptr;
};

class PostfixWriter {
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

	void add_op(PostfixOp op) { op_.push_back(op); }
	PostfixOp* op_data() { return op_.data(); }
	const PostfixOp* op_data() const { return op_.data(); }
	uint8_t op_size() const { return op_.size(); }
	PostfixOp op(uint8_t index) const { return op_data()[index]; }
	PostfixOp& op(uint8_t index) { return op_data()[index]; }

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

  void Push(std::initializer_list<float> values) {
    add_op(PostfixOp::Push);
    add_i(values.size());
    for (float value : values) add_f(value);
  }

  void Pop(uint8_t n) {
    add_op(PostfixOp::Pop);
    add_i(n);
  }

private:
	constexpr uint16_t op_offset() const { return sizeof(PostfixHeader); }

	uint16_t i_offset() const { return op_offset() + op_size(); }

	uint16_t f_offset() const {
		uint16_t offset = op_offset() + op_size() + i_size();
		return (offset + uint16_t(3)) & ~uint16_t(3);
	}

	std::vector<PostfixOp> op_;
	std::vector<uint8_t> i_;
	std::vector<float> f_;
};

struct PostfixStack {
  float* stack_data;
  size_t stack_size;
  size_t stack_capacity;

  void clear() { stack_size = 0; }

  bool push(float v) {
    if (stack_size + 1 > stack_capacity) return false;
    stack_data[stack_size++] = v;
    return true;
  }

  template <typename Expr>
  EvalStatus Eval(const Expr& expr) {
    PostfixEvalContext context{
      .op_head        = expr.op_data(),
      .i_head         = expr.i_data(),
      .f_head         = expr.f_data(),
      .stack_data     = stack_data,
      .op_size        = expr.op_size(),
      .i_size         = expr.i_size(),
      .f_size         = expr.f_size(),
      .stack_size     = stack_size,
      .stack_capacity = stack_capacity,
    };
    EvalStatus status = context.Eval();
    stack_size = context.stack_size;
    return status;
  }

  using value_type = float;
  using iterator = float*;
  using const_iterator = float*;

  float& operator[](size_t i) { return stack_data[i]; }
  float operator[](size_t i) const { return stack_data[i]; }

  float* data() { return stack_data; }
  const float* data() const { return stack_data; }

  size_t size() const { return stack_size; }

  iterator begin() { return stack_data; }
  iterator end() { return stack_data + stack_size; }

  const_iterator begin() const { return stack_data; }
  const_iterator end() const { return stack_data + stack_size; }
};

}
