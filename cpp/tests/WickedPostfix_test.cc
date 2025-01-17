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

    eval.stack_data = new uint8_t[stack_capacity * 4];
    eval.stack_size = 0;
    eval.stack_capacity = stack_capacity * 4;

    eval.temp_data = new uint8_t[temp_capacity * 4];
    eval.temp_capacity = temp_capacity * 4;

    for (float value : stack_values) {
      WickedPostfixEval_pushf(&eval, value);
    }
  }

  ~TestEval() {
    delete[] eval.stack_data;
    delete[] eval.temp_data;
  }

  std::span<float> stack() { return std::span<float>(reinterpret_cast<float*>(eval.stack_data), eval.stack_size / sizeof(float)); }
};

TEST(EvalTest, Empty) {
  WickedPostfixWriter writer;
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42));
}

TEST(EvalTest, Return) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Ret);
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_f(5);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42));
}

TEST(EvalTest, Jmp) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_u(13);
  writer.add_b(WickedPostfixOp_Jmp);
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_f(5);
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_f(6);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 6));
}

TEST(EvalTest, JmpIllegalAddress) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_u(20);
  writer.add_b(WickedPostfixOp_Jmp);
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_f(5);
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(1);
  writer.add_f(6);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalAddress);
}

TEST(EvalTest, Push) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(2);
  writer.add_f(1);
  writer.add_f(2);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(42, 1, 2));
}

TEST(EvalTest, PushIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Push);
  writer.add_b(2);
  writer.add_f(1);
  auto buffer = writer.Write();

  TestEval eval(4, {42});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Pop) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Pop);
  writer.add_b(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1));
}

TEST(EvalTest, PopStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Pop);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PopIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Pop);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Dup) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Dup);
  writer.add_b(1);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 2, 3, 2));
}

TEST(EvalTest, DupStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Dup);
  writer.add_b(2);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, DupIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Dup);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotL) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotL);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 3, 4, 2));
}

TEST(EvalTest, RotLStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotL);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotLIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotL);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, RotR) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotR);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 2, 3));
}

TEST(EvalTest, RotRStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotR);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RotRIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_RotR);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Rev) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Rev);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(1, 4, 3, 2));
}

TEST(EvalTest, RevStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Rev);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, RevIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Rev);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, Transpose) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Transpose);
  writer.add_b(2);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, 6, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, TransposeTempOverflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Transpose);
  writer.add_b(2);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, 5, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_TempOverflow);
}

TEST(EvalTest, TransposeStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Transpose);
  writer.add_b(2);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, TransposeIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Transpose);
  writer.add_b(2);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, AddF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AddF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3));
}

TEST(EvalTest, AddFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AddF);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, SubF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_SubF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -1));
}

TEST(EvalTest, SubFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_SubF);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6));
}

TEST(EvalTest, MulFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulF);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAddF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulAddF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 3, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 7));
}

TEST(EvalTest, MulAddFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulAddF);
  auto buffer = writer.Write();

  TestEval eval(4, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, DivF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_DivF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, DivFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_DivF);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, ModF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_ModF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 8, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, ModFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_ModF);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NegF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NegF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -2));
}

TEST(EvalTest, NegFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NegF);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, AbsF) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AbsF);
  auto buffer = writer.Write();

  TestEval eval(4, {0, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 2));
}

TEST(EvalTest, AbsFStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AbsF);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Inv) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Inv);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 0.5));
}

TEST(EvalTest, InvStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Inv);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Pow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Pow);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 8));
}

TEST(EvalTest, PowStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Pow);
  auto buffer = writer.Write();

  TestEval eval(4, {1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sqrt) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Sqrt);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(7)));
}

TEST(EvalTest, SqrtStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Sqrt);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Exp) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Exp);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::exp(4)));
}

TEST(EvalTest, ExpStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Exp);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Ln) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Ln);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::log(5)));
}

TEST(EvalTest, LnStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Ln);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Sin) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Sin);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sin(5)));
}

TEST(EvalTest, SinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Sin);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Cos) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Cos);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::cos(5)));
}

TEST(EvalTest, CosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Cos);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Tan) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Tan);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::tan(5)));
}

TEST(EvalTest, TanStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Tan);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Asin) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Asin);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::asin(0.5)));
}

TEST(EvalTest, AsinStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Asin);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Acos) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Acos);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 0.5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::acos(0.5)));
}

TEST(EvalTest, AcosStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Acos);
  auto buffer = writer.Write();

  TestEval eval(4, {});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, Atan2) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Atan2);
  auto buffer = writer.Write();

  TestEval eval(4, {0, 5, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::atan2(5, 4)));
}

TEST(EvalTest, Atan2StackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_Atan2);
  auto buffer = writer.Write();

  TestEval eval(4, {5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyVec);
  writer.add_b(4);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 79));
}

TEST(EvalTest, PolyVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(6, {2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyVec);
  auto buffer = writer.Write();

  TestEval eval(6, {0, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, PolyMat) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyMat);
  writer.add_b(4);
  writer.add_b(2);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PolyMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyMat);
  writer.add_b(4);
  writer.add_b(2);
  auto buffer = writer.Write();

  TestEval eval(12, {2, 3, 4, 5, 6, 7, 8, 9});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, PolyMatIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_PolyMat);
  writer.add_b(4);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, AddVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AddVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, AddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AddVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, AddVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_AddVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, SubVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_SubVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 2, 1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, SubVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_SubVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, SubVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_SubVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, MulVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3, 4, 5});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulAddVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulAddVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 3, -1, 0, 1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, MulAddVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulAddVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(12, {1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulAddVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulAddVec);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, ScaleVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_ScaleVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, ScaleVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_ScaleVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, ScaleVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_ScaleVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, NegVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NegVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 3, 4, -2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, NegVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NegVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NegVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NegVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 1, 2, 3});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, NormVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NormVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, NormVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NormVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {1, 2});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, NormVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_NormVec);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 2, 3, 4});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, MulMat) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulMat);
  writer.add_b(2);
  writer.add_b(3);
  writer.add_b(4);
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
  writer.add_b(WickedPostfixOp_MulMat);
  writer.add_b(2);
  writer.add_b(3);
  writer.add_b(4);
  auto buffer = writer.Write();

  TestEval eval(32, 7, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_TempOverflow);
}

TEST(EvalTest, MulMatStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulMat);
  writer.add_b(2);
  writer.add_b(3);
  writer.add_b(4);
  auto buffer = writer.Write();

  TestEval eval(32, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, MulMatIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_MulMat);
  writer.add_b(2);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

TEST(EvalTest, LerpVec) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_LerpVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0, 0.25, 2, 3, 4, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, LerpVecStackUnderflow) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_LerpVec);
  writer.add_b(3);
  auto buffer = writer.Write();

  TestEval eval(8, {0.25, 2, 3, 4, 6, 7});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_StackUnderflow);
}

TEST(EvalTest, LerpVecIncomplete) {
  WickedPostfixWriter writer;
  writer.add_b(WickedPostfixOp_LerpVec);
  auto buffer = writer.Write();

  TestEval eval(12, {0, 0.25, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_TRUE(WickedPostfixRead(buffer.data(), buffer.size(), &eval.eval));
  EXPECT_EQ(WickedPostfixEvaluate(&eval.eval), WickedEvalStatus_IllegalOperation);
}

}
