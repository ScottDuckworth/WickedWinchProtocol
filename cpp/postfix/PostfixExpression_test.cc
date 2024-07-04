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

TEST(EvalTest, PopIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_f(1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3));
}

TEST(EvalTest, PopFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_i(2);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3));
}