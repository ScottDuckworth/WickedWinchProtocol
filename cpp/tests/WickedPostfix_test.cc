#include "WickedPostfix.h"

#include <cmath>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace {

struct TestEval {
  struct WickedPostfixEval eval;

  TestEval(uint8_t stack_capacity, std::initializer_list<float> stack_values)
      : TestEval(stack_capacity, 0, stack_values) {}

  TestEval(uint8_t stack_capacity, uint8_t temp_capacity, std::initializer_list<float> stack_values) {
    assert(stack_capacity >= stack_values.size());

    eval.stack_data = new float[stack_capacity];
    eval.stack_size = stack_values.size();
    eval.stack_capacity = stack_capacity;

    eval.temp_data = new float[temp_capacity];
    eval.temp_capacity = temp_capacity;

    float* v = eval.stack_data;
    for (float value : stack_values) {
      *v++ = value;
    }
  }

  ~TestEval() {
    delete[] eval.stack_data;
    delete[] eval.temp_data;
  }

  std::span<float> stack() { return std::span<float>(eval.stack_data, eval.stack_size); }
};

TEST(EvalTest, Empty) {
  WickedPostfixWriter writer;
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42));
}

TEST(EvalTest, Push) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Push);
  writer.add_i(2);
  writer.add_f(1);
  writer.add_f(2);
  EXPECT_EQ(writer.i_size(), 2);
  EXPECT_EQ(writer.d_size(), 2);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 1, 2));
}

TEST(EvalTest, PushMany) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Push);
  writer.add_i(2);
  writer.add_i(WickedPostfixOp_Push);
  writer.add_i(1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  EXPECT_EQ(writer.i_size(), 4);
  EXPECT_EQ(writer.d_size(), 3);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 1, 2, 3));
}

TEST(EvalTest, PushIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Push);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Push);
  writer.add_i(2);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, Pop) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Pop);
  writer.add_i(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1));
}

TEST(EvalTest, PopStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Pop);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PopIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Pop);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Dup) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Dup);
  writer.add_i(1);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 2, 3, 2));
}

TEST(EvalTest, DupStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Dup);
  writer.add_i(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, DupIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Dup);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotL) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotL);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 3, 4, 2));
}

TEST(EvalTest, RotLStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotL);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotLIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotL);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotR) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotR);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 2, 3));
}

TEST(EvalTest, RotRStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotR);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotRIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_RotR);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Rev) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Rev);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 3, 2));
}

TEST(EvalTest, RevStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Rev);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RevIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Rev);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Transpose) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, 6, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, TransposeTempOverflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, 5, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_TempOverflow);
}

TEST(EvalTest, TransposeStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, TransposeIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushTranspose) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  auto buffer = writer.Write();

  TestEval eval(8, 6, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, PushTransposeDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, Add) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Add);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3));
}

TEST(EvalTest, AddStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Add);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sub) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sub);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -1));
}

TEST(EvalTest, SubStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sub);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Mul) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Mul);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6));
}

TEST(EvalTest, MulStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Mul);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAdd) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAdd);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 3, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 7));
}

TEST(EvalTest, MulAddStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAdd);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Div) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Div);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, DivStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Div);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Mod) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Mod);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 8, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, ModStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Mod);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Neg) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Neg);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -2));
}

TEST(EvalTest, NegStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Neg);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Abs) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Abs);
  auto buffer = writer.Write();

  TestEval eval(4, {0, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, AbsStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Abs);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Inv) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Inv);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, InvStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Inv);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Pow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Pow);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 8));
}

TEST(EvalTest, PowStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Pow);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sqrt) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sqrt);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(7)));
}

TEST(EvalTest, SqrtStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sqrt);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Exp) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Exp);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::exp(4)));
}

TEST(EvalTest, ExpStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Exp);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Ln) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Ln);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::log(5)));
}

TEST(EvalTest, LnStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Ln);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sin) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sin);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sin(5)));
}

TEST(EvalTest, SinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Sin);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Cos) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Cos);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::cos(5)));
}

TEST(EvalTest, CosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Cos);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Tan) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Tan);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::tan(5)));
}

TEST(EvalTest, TanStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Tan);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Asin) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Asin);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::asin(0.5)));
}

TEST(EvalTest, AsinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Asin);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Acos) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Acos);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::acos(0.5)));
}

TEST(EvalTest, AcosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Acos);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Atan2) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Atan2);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::atan2(5, 4)));
}

TEST(EvalTest, Atan2StackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Atan2);
  auto buffer = writer.Write();

  TestEval eval(4, {5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyVec);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 79));
}

TEST(EvalTest, PolyVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(6, {2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyVec);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushPolyVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyVec);
  writer.add_i(4 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 79));
}

TEST(EvalTest, PushPolyVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyVec);
  writer.add_i(4 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, PolyMat) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PolyMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1);
  auto buffer = writer.Write();

  TestEval eval(12, {2, 3, 4, 5, 6, 7, 8, 9});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyMatIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyMat);
  writer.add_i(4);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushPolyMat) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  writer.add_f(9);
  writer.add_f(10);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PushPolyMatDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  writer.add_f(9);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, AddVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_AddVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, AddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_AddVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, AddVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_AddVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushAddVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_AddVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, PushAddVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_AddVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, SubVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_SubVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, SubVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_SubVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, SubVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_SubVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushSubVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_SubVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(2);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, PushSubVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_SubVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, MulVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, MulVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushMulVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(-1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, PushMulVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, MulAddVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  writer.add_i(3 << 2);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 3, -1, 0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, MulAddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  writer.add_i(3 << 2);
  auto buffer = writer.Write();

  TestEval eval(12, {1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAddVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushMulAddVec1) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  writer.add_i(3 << 2 | 1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, PushMulAddVec2) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  writer.add_i(3 << 2 | 2);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(-1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, PushMulAddVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulAddVec);
  writer.add_i(3 << 2 | 1);
  writer.add_f(0);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, ScaleVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_ScaleVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, ScaleVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_ScaleVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, ScaleVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_ScaleVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushScaleVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_ScaleVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(-2);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, PushScaleVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_ScaleVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, NegVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NegVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, NegVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NegVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NegVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NegVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushNegVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NegVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(-2);
  auto buffer = writer.Write();

  TestEval eval(8, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, PushNegVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NegVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(8, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, NormVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NormVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, NormVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NormVec);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NormVecIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NormVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushNormVec) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NormVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  auto buffer = writer.Write();

  TestEval eval(8, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, PushNormVecDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_NormVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(2);
  writer.add_f(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, MulMat) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(32, 8, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(
    0,
    1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
    4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12));
}

TEST(EvalTest, MulMatTempOverflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(32, 7, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_TempOverflow);
}

TEST(EvalTest, MulMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(32, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulMatIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushMulMat) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  writer.add_f(9);
  writer.add_f(10);
  writer.add_f(11);
  writer.add_f(12);
  auto buffer = writer.Write();

  TestEval eval(32, 8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(
    0,
    1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
    4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12));
}

TEST(EvalTest, PushMulMatDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  writer.add_f(9);
  writer.add_f(10);
  writer.add_f(11);
  auto buffer = writer.Write();

  TestEval eval(32, {1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, Lerp) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  writer.add_i(3<<2);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25, 2, 3, 4, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, LerpStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  writer.add_i(3<<2);
  auto buffer = writer.Write();

  TestEval eval(8, {0.25, 2, 3, 4, 6, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, LerpIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 0.25, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PushLerp1) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  writer.add_i(3<<2|1);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, PushLerp2) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  writer.add_i(3<<2|2);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, PushLerpDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_Lerp);
  writer.add_i(3<<2|1);
  writer.add_f(6);
  writer.add_f(7);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

TEST(EvalTest, LerpTable_n1) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, -1, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, LerpTable_0) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 0, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, LerpTable_0_5) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 0.5, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1.75, 2.25, 4));
}

TEST(EvalTest, LerpTable_2) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 2, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4, 3, 7));
}

TEST(EvalTest, LerpTable_4) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 4, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, LerpTable_6) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 6, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, LerpTable_7) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 7, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, LerpTableStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(3 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0.5, 0, 1, 2, 2, 4, 3, 6, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, LerpTableIntUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 0.5, 0, 1, 2, 2, 4, 3, 6, 8, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, LerpTableIllegalOperation) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(0);
  writer.add_i(4 << 1);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4));
}

TEST(EvalTest, PushLerpTable) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1 | 1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(2);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(7);
  writer.add_f(6);
  writer.add_f(8);
  writer.add_f(2);
  writer.add_f(0);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, PushLerpTableDataUnderflow) {
  WickedPostfixWriter writer;
  writer.add_i(WickedPostfixOp_LerpTable);
  writer.add_i(3);
  writer.add_i(4 << 1 | 1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(2);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(7);
  writer.add_f(6);
  writer.add_f(8);
  writer.add_f(2);
  auto buffer = writer.Write();

  TestEval eval(16, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_DataUnderflow);
}

}
