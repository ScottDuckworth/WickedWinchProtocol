#include "WickedPostfix.h"

#include <cmath>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace {

struct TestEval {
  const uint8_t* prog_data = nullptr;
  uint16_t prog_size = 0;
  float* stack_data = nullptr;
  uint16_t stack_size = 0;
  uint16_t stack_capacity = 0;
  uint16_t min_capacity = 0;
  uint16_t final_stack_size = 0;

  TestEval(uint16_t capacity, std::initializer_list<float> stack_values) {
    assert(capacity >= stack_values.size());

    stack_data = new float[capacity];
    stack_size = static_cast<uint16_t>(stack_values.size());
    stack_capacity = capacity;

    float* v = stack_data;
    for (float value : stack_values) {
      *v++ = value;
    }
  }

  ~TestEval() {
    delete[] stack_data;
  }

  std::span<float> stack() { return std::span<float>(stack_data, stack_size); }
};

bool WickedPostfixRead(const uint8_t* data, size_t size, TestEval* eval) {
  return ::WickedPostfixRead(data, size, &eval->prog_data, &eval->prog_size);
}

WickedEvalStatus_t WickedPostfixValidate(TestEval* eval, uint16_t expected_final_stack_size) {
  WickedEvalStatus_t status = ::WickedPostfixValidate(
      eval->prog_data, eval->prog_size, eval->stack_size,
      &eval->min_capacity, &eval->final_stack_size);
  if (status != WickedEvalStatus_Ok) {
    return status;
  }
  if (eval->min_capacity > eval->stack_capacity) {
    return WickedEvalStatus_StackOverflow;
  }
  if (eval->final_stack_size != expected_final_stack_size) {
    return WickedEvalStatus_UnexpectedStackSize;
  }
  return WickedEvalStatus_Ok;
}

void WickedPostfixEvaluate(TestEval* eval) {
  ::WickedPostfixEvaluate(eval->prog_data, eval->prog_size, eval->stack_size, eval->stack_data);
  eval->stack_size = eval->final_stack_size;
}

TEST(EvalTest, Empty) {
  WickedPostfixWriter writer;
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 1), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(42));
}

TEST(EvalTest, Push) {
  WickedPostfixWriter writer;
  writer.Push({1, 2});
  EXPECT_EQ(writer.prog_size(), 10);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 3), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 1, 2));
}

TEST(EvalTest, PushSingle) {
  WickedPostfixWriter writer;
  writer.Push(42.0f);
  EXPECT_EQ(writer.prog_size(), 6);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 42));
}

TEST(EvalTest, PushMany) {
  WickedPostfixWriter writer;
  writer.Push({1, 2});
  writer.Push({3});
  EXPECT_EQ(writer.prog_size(), 16);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 1, 2, 3));
}

TEST(EvalTest, PushIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Push);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushDataUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Push);
  writer.EmitByte(2);
  writer.EmitFloat(1);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}
TEST(EvalTest, Pop) {
  WickedPostfixWriter writer;
  writer.Pop(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 1), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1));
}

TEST(EvalTest, PopStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Pop(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PopIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Pop);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Dup) {
  WickedPostfixWriter writer;
  writer.Dup(1);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 2, 3, 2));
}

TEST(EvalTest, DupStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Dup(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, DupIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Dup);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotL) {
  WickedPostfixWriter writer;
  writer.RotL(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 3, 4, 2));
}

TEST(EvalTest, RotLStackUnderflow) {
  WickedPostfixWriter writer;
  writer.RotL(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotLIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_RotL);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotR) {
  WickedPostfixWriter writer;
  writer.RotR(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 2, 3));
}

TEST(EvalTest, RotRStackUnderflow) {
  WickedPostfixWriter writer;
  writer.RotR(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotRIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_RotR);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Rev) {
  WickedPostfixWriter writer;
  writer.Rev(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 3, 2));
}

TEST(EvalTest, RevStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Rev(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RevIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Rev);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Transpose) {
  WickedPostfixWriter writer;
  writer.Transpose(2, 3);
  auto buffer = writer.Write();

  TestEval eval(14, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 7), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, TransposeSquare) {
  WickedPostfixWriter writer;
  writer.Transpose(2, 2);
  auto buffer = writer.Write();

  TestEval eval(5, {0, 1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 5), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 3, 2, 4));
}

TEST(EvalTest, TransposeStackOverflow) {
  WickedPostfixWriter writer;
  writer.Transpose(2, 3);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackOverflow);
}

TEST(EvalTest, TransposeStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Transpose(2, 3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, TransposeIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_Transpose);
  writer.EmitByte(2);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, StoreLoadScalarPeek) {
  WickedPostfixWriter writer;
  writer.Store(0, 1, /*pop=*/false);
  writer.Push(10.0f);
  writer.Load(0, 1);
  auto buffer = writer.Write();

  TestEval eval(4, {5.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 3), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(5.0f, 10.0f, 5.0f));
}

TEST(EvalTest, StoreLoadScalarPop) {
  WickedPostfixWriter writer;
  writer.Store(1, 1, /*pop=*/true);
  writer.Push(10.0f);
  writer.Load(1, 1);
  auto buffer = writer.Write();

  TestEval eval(4, {5.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(10.0f, 5.0f));
}

TEST(EvalTest, StoreLoadVectorPeek) {
  WickedPostfixWriter writer;
  writer.Store(0, 3, /*pop=*/false);
  writer.Push(99.0f);
  writer.Load(0, 3);
  auto buffer = writer.Write();

  TestEval eval(8, {1.0f, 2.0f, 3.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 7), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(1.0f, 2.0f, 3.0f, 99.0f, 1.0f, 2.0f, 3.0f));
}

TEST(EvalTest, StoreLoadVectorPop) {
  WickedPostfixWriter writer;
  writer.Store(1, 3, /*pop=*/true);
  writer.Push(99.0f);
  writer.Load(1, 3);
  auto buffer = writer.Write();

  TestEval eval(8, {0.0f, 1.0f, 2.0f, 3.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 5), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0.0f, 99.0f, 1.0f, 2.0f, 3.0f));
}

TEST(EvalTest, StoreStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Store(0, 2, /*pop=*/false);
  auto buffer = writer.Write();

  TestEval eval(4, {1.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, StoreOutOfBounds) {
  WickedPostfixWriter writer;
  writer.Store(3, 2, /*pop=*/false);
  auto buffer = writer.Write();

  TestEval eval(4, {1.0f, 2.0f, 3.0f});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, LoadOutOfBounds) {
  WickedPostfixWriter writer;
  writer.Load(2, 3);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Add) {
  WickedPostfixWriter writer;
  writer.Add();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3));
}

TEST(EvalTest, AddStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Add();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sub) {
  WickedPostfixWriter writer;
  writer.Sub();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -1));
}

TEST(EvalTest, SubStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Sub();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Mul) {
  WickedPostfixWriter writer;
  writer.Mul();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6));
}

TEST(EvalTest, MulStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Mul();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAdd) {
  WickedPostfixWriter writer;
  writer.MulAdd();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 3, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 7));
}

TEST(EvalTest, MulAddStackUnderflow) {
  WickedPostfixWriter writer;
  writer.MulAdd();
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Div) {
  WickedPostfixWriter writer;
  writer.Div();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, DivStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Div();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Mod) {
  WickedPostfixWriter writer;
  writer.Mod();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 8, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, ModStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Mod();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Neg) {
  WickedPostfixWriter writer;
  writer.Neg();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -2));
}

TEST(EvalTest, NegStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Neg();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Abs) {
  WickedPostfixWriter writer;
  writer.Abs();
  auto buffer = writer.Write();

  TestEval eval(4, {0, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, AbsStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Abs();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Inv) {
  WickedPostfixWriter writer;
  writer.Inv();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, InvStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Inv();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Pow) {
  WickedPostfixWriter writer;
  writer.Pow();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 8));
}

TEST(EvalTest, PowStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Pow();
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sqrt) {
  WickedPostfixWriter writer;
  writer.Sqrt();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(7)));
}

TEST(EvalTest, SqrtStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Sqrt();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Exp) {
  WickedPostfixWriter writer;
  writer.Exp();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::exp(4)));
}

TEST(EvalTest, ExpStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Exp();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Ln) {
  WickedPostfixWriter writer;
  writer.Ln();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::log(5)));
}

TEST(EvalTest, LnStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Ln();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sin) {
  WickedPostfixWriter writer;
  writer.Sin();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sin(5)));
}

TEST(EvalTest, SinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Sin();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Cos) {
  WickedPostfixWriter writer;
  writer.Cos();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::cos(5)));
}

TEST(EvalTest, CosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Cos();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Tan) {
  WickedPostfixWriter writer;
  writer.Tan();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::tan(5)));
}

TEST(EvalTest, TanStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Tan();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Asin) {
  WickedPostfixWriter writer;
  writer.Asin();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::asin(0.5)));
}

TEST(EvalTest, AsinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Asin();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Acos) {
  WickedPostfixWriter writer;
  writer.Acos();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::acos(0.5)));
}

TEST(EvalTest, AcosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.Acos();
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Atan2) {
  WickedPostfixWriter writer;
  writer.Atan2();
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::atan2(5, 4)));
}

TEST(EvalTest, Atan2StackUnderflow) {
  WickedPostfixWriter writer;
  writer.Atan2();
  auto buffer = writer.Write();

  TestEval eval(4, {5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVec) {
  WickedPostfixWriter writer;
  writer.PolyVec({3, 4, 5, 6});
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 79));
}

TEST(EvalTest, PolyVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.PolyVec({3, 4, 5});
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_PolyVec);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PolyVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_PolyVec);
  writer.EmitByte(3);
  writer.EmitFloat(1.0f);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PolyMat) {
  WickedPostfixWriter writer;
  writer.PolyMat(4, 2, {3, 4, 5, 6, 7, 8, 9, 10});
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 3), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3 + 2 * 5 + 4 * 7 + 8 * 9, 4 + 2 * 6 + 4 * 8 + 8 * 10));
}

TEST(EvalTest, PolyMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.PolyMat(4, 2, {3, 4, 5, 6, 7, 8, 9, 10});
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyMatIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_PolyMat);
  writer.EmitByte(4);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PolyMatDataUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_PolyMat);
  writer.EmitByte(4);
  writer.EmitByte(2);
  writer.EmitFloat(1.0f);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, AddVec) {
  WickedPostfixWriter writer;
  writer.AddVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, AddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.AddVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, AddVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_AddVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, SubVec) {
  WickedPostfixWriter writer;
  writer.SubVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, SubVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.SubVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, SubVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_SubVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulVec) {
  WickedPostfixWriter writer;
  writer.MulVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, MulVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.MulVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_MulVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulAddVec) {
  WickedPostfixWriter writer;
  writer.MulAddVec(3);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 3, -1, 0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, MulAddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.MulAddVec(3);
  auto buffer = writer.Write();

  TestEval eval(12, {1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAddVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_MulAddVec);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, ScaleVec) {
  WickedPostfixWriter writer;
  writer.ScaleVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, ScaleVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.ScaleVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, ScaleVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_ScaleVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, NegVec) {
  WickedPostfixWriter writer;
  writer.NegVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, NegVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.NegVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NegVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_NegVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, NormVec) {
  WickedPostfixWriter writer;
  writer.NormVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(2 * 2 + 3 * 3 + 4 * 4)));
}

TEST(EvalTest, NormVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.NormVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NormVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_NormVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulMat) {
  WickedPostfixWriter writer;
  writer.MulMat(2, 3, 4);
  auto buffer = writer.Write();

  TestEval eval(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 9), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(
                                0,
                                1 * 1 + 2 * 5 + 3 * 9, 1 * 2 + 2 * 6 + 3 * 10, 1 * 3 + 2 * 7 + 3 * 11, 1 * 4 + 2 * 8 + 3 * 12,
                                4 * 1 + 5 * 5 + 6 * 9, 4 * 2 + 5 * 6 + 6 * 10, 4 * 3 + 5 * 7 + 6 * 11, 4 * 4 + 5 * 8 + 6 * 12));
}

TEST(EvalTest, MulMatStackOverflow) {
  WickedPostfixWriter writer;
  writer.MulMat(2, 3, 4);
  auto buffer = writer.Write();

  TestEval eval(26, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackOverflow);
}

TEST(EvalTest, MulMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.MulMat(2, 3, 4);
  auto buffer = writer.Write();

  TestEval eval(32, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulMatIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_MulMat);
  writer.EmitByte(2);
  writer.EmitByte(3);
  auto buffer = writer.Write();

  TestEval eval(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, LerpVec) {
  WickedPostfixWriter writer;
  writer.LerpVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25, 2, 3, 4, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 4), WickedEvalStatus_Ok);
  WickedPostfixEvaluate(&eval);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, LerpVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.LerpVec(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0.25, 2, 3, 4, 6, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, LerpVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.EmitOp(WickedPostfixOp_LerpVec);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 0.25, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  EXPECT_EQ(WickedPostfixValidate(&eval, 0), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, UnexpectedStackSize) {
  WickedPostfixWriter writer;
  writer.Push({1, 2});
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval));
  // Initial stack has 1 element, pushes 2 -> final size is 3.
  EXPECT_EQ(WickedPostfixValidate(&eval, 2), WickedEvalStatus_UnexpectedStackSize);
  EXPECT_EQ(WickedPostfixValidate(&eval, 3), WickedEvalStatus_Ok);
}

TEST(EvalTest, DirectValidateAndEvaluate) {
  WickedPostfixWriter writer;
  writer.Push({1, 2});
  auto buffer = writer.Write();

  const uint8_t* prog_data = nullptr;
  uint16_t prog_size = 0;
  EXPECT_TRUE(::WickedPostfixRead(buffer.data(), buffer.size(), &prog_data, &prog_size));

  uint16_t min_capacity = 0;
  uint16_t final_stack_size = 0;
  EXPECT_EQ(::WickedPostfixValidate(prog_data, prog_size, 1, &min_capacity, &final_stack_size), WickedEvalStatus_Ok);
  EXPECT_EQ(min_capacity, 3);
  EXPECT_EQ(final_stack_size, 3);

  float stack[4] = {42, 0, 0, 0};
  ::WickedPostfixEvaluate(prog_data, prog_size, 1, stack);
  EXPECT_THAT(std::span<float>(stack, final_stack_size), ElementsAre(42, 1, 2));
}

TEST(EvalTest, StackCapacityExceedsUint16) {
  WickedPostfixWriter writer;
  writer.Transpose(255, 254);
  auto buffer = writer.Write();

  const uint8_t* prog_data = nullptr;
  uint16_t prog_size = 0;
  EXPECT_TRUE(::WickedPostfixRead(buffer.data(), buffer.size(), &prog_data, &prog_size));

  uint16_t min_capacity = 0;
  uint16_t final_stack_size = 0;
  EXPECT_EQ(::WickedPostfixValidate(prog_data, prog_size, 64770, &min_capacity, &final_stack_size), WickedEvalStatus_StackOverflow);
}

TEST(EvalTest, ReadUnsafe) {
  WickedPostfixWriter writer;
  writer.Push({1.0f, 2.0f});
  auto buffer = writer.Write();

  const uint8_t* safe_prog_data = nullptr;
  uint16_t safe_prog_size = 0;
  EXPECT_TRUE(::WickedPostfixRead(buffer.data(), buffer.size(), &safe_prog_data, &safe_prog_size));

  const uint8_t* unsafe_prog_data = nullptr;
  uint16_t unsafe_prog_size = 0;
  WickedPostfixReadUnsafe(buffer.data(), &unsafe_prog_data, &unsafe_prog_size);

  EXPECT_EQ(safe_prog_data, unsafe_prog_data);
  EXPECT_EQ(safe_prog_size, unsafe_prog_size);
  EXPECT_NE(unsafe_prog_data, nullptr);
  EXPECT_GT(unsafe_prog_size, 0);
}

TEST(EvalTest, ReadUnaligned) {
  WickedPostfixWriter writer;
  writer.Push({1.0f, 2.0f});
  auto buffer = writer.Write();

  std::vector<uint8_t> unaligned_buffer(buffer.size() + 1);
  unaligned_buffer[0] = 0xAA;
  memcpy(unaligned_buffer.data() + 1, buffer.data(), buffer.size());

  const uint8_t* unaligned_data = unaligned_buffer.data() + 1;
  const uint8_t* prog_data = nullptr;
  uint16_t prog_size = 0;

  EXPECT_TRUE(::WickedPostfixRead(unaligned_data, buffer.size(), &prog_data, &prog_size));
  EXPECT_EQ(prog_data, unaligned_data + sizeof(WickedPostfixHeader_t));
  EXPECT_GT(prog_size, 0);

  const uint8_t* unsafe_prog_data = nullptr;
  uint16_t unsafe_prog_size = 0;
  WickedPostfixReadUnsafe(unaligned_data, &unsafe_prog_data, &unsafe_prog_size);
  EXPECT_EQ(unsafe_prog_data, prog_data);
  EXPECT_EQ(unsafe_prog_size, prog_size);
}

TEST(EvalTest, HeaderPacked) {
  EXPECT_EQ(sizeof(WickedPostfixHeader_t), 2);
  EXPECT_EQ(alignof(WickedPostfixHeader_t), 1);
}

}  // namespace