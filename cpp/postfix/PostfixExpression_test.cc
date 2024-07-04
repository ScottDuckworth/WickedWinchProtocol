#include "PostfixExpression.h"
#include "proto/PathExpression.pb.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using wickedwinch::postfix::Eval;
using wickedwinch::postfix::EvalStatus;
using wickedwinch::proto::Operation;
using wickedwinch::proto::PostfixExpression;

TEST(EvalTest, Empty) {
  PostfixExpression expr;
  std::vector<float> stack = {42};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(42));
}

TEST(EvalTest, Push) {
  PostfixExpression expr;
  expr.add_op(Operation::Push);
  expr.add_i(2);
  expr.add_f(1);
  expr.add_f(2);
  std::vector<float> stack = {42};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(42, 1, 2));
}

TEST(EvalTest, PushIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Push);
  expr.add_f(1);
  std::vector<float> stack = {42};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(42));
}

TEST(EvalTest, PushFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Push);
  expr.add_i(2);
  expr.add_f(1);
  std::vector<float> stack = {42};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(42));
}

TEST(EvalTest, Pop) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_i(2);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, PopStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_i(3);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, PopIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_f(1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3));
}

TEST(EvalTest, Dup) {
  PostfixExpression expr;
  expr.add_op(Operation::Dup);
  expr.add_i(1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 2));
}

TEST(EvalTest, DupStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Dup);
  expr.add_i(2);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, DupIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Dup);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3));
}

TEST(EvalTest, RotL) {
  PostfixExpression expr;
  expr.add_op(Operation::RotL);
  expr.add_i(3);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(1, 3, 4, 2));
}

TEST(EvalTest, RotLStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::RotL);
  expr.add_i(3);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, RotLIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::RotL);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4));
}

TEST(EvalTest, RotR) {
  PostfixExpression expr;
  expr.add_op(Operation::RotR);
  expr.add_i(3);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(1, 4, 2, 3));
}

TEST(EvalTest, RotRStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::RotR);
  expr.add_i(3);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, RotRIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::RotR);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4));
}

TEST(EvalTest, Rev) {
  PostfixExpression expr;
  expr.add_op(Operation::Rev);
  expr.add_i(3);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(1, 4, 3, 2));
}

TEST(EvalTest, RevStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Rev);
  expr.add_i(3);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, RevIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Rev);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4));
}

TEST(EvalTest, Transpose) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, TransposeStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2, 3, 4, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(EvalTest, TransposeIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, PushTranspose) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  expr.add_i(3 << 1 | 1);
  expr.add_f(1);
  expr.add_f(2);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  expr.add_f(6);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, PushTransposeFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  expr.add_i(3 << 1 | 1);
  expr.add_f(1);
  expr.add_f(2);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0));
}

TEST(EvalTest, Add) {
  PostfixExpression expr;
  expr.add_op(Operation::Add);
  std::vector<float> stack = {0, 1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 3));
}

TEST(EvalTest, AddStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Add);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, Sub) {
  PostfixExpression expr;
  expr.add_op(Operation::Sub);
  std::vector<float> stack = {0, 1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -1));
}

TEST(EvalTest, SubStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Sub);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, Mul) {
  PostfixExpression expr;
  expr.add_op(Operation::Mul);
  std::vector<float> stack = {0, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 6));
}

TEST(EvalTest, MulStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Mul);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, MulAdd) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAdd);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 7));
}

TEST(EvalTest, MulAddStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAdd);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, Div) {
  PostfixExpression expr;
  expr.add_op(Operation::Div);
  std::vector<float> stack = {0, 1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 0.5));
}

TEST(EvalTest, DivStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Div);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, Mod) {
  PostfixExpression expr;
  expr.add_op(Operation::Mod);
  std::vector<float> stack = {0, 8, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2));
}

TEST(EvalTest, ModStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Mod);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}