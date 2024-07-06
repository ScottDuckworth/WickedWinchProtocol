#include "PostfixExpression.h"
#include "proto/PathExpression.pb.h"

#include <cmath>
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

TEST(EvalTest, PushIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Push);
  expr.add_i(-1);
  expr.add_f(1);
  expr.add_f(2);
  std::vector<float> stack = {42};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, PopIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Pop);
  expr.add_i(-1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, DupIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Dup);
  expr.add_i(-1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, RotLIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::RotL);
  expr.add_i(-1);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, RotRIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::RotR);
  expr.add_i(-1);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, RevIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Rev);
  expr.add_i(-1);
  std::vector<float> stack = {1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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

TEST(EvalTest, TransposeIllegalOperationRows) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(-1);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, TransposeIllegalOperationCols) {
  PostfixExpression expr;
  expr.add_op(Operation::Transpose);
  expr.add_i(2);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
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
  std::vector<float> stack = {0, 3, 2, 1};
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

TEST(EvalTest, Neg) {
  PostfixExpression expr;
  expr.add_op(Operation::Neg);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -2));
}

TEST(EvalTest, NegStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Neg);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Abs) {
  PostfixExpression expr;
  expr.add_op(Operation::Abs);
  std::vector<float> stack = {0, -2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2));
}

TEST(EvalTest, AbsStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Abs);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Inv) {
  PostfixExpression expr;
  expr.add_op(Operation::Inv);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 0.5));
}

TEST(EvalTest, InvStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Inv);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Pow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pow);
  std::vector<float> stack = {0, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 8));
}

TEST(EvalTest, PowStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Pow);
  std::vector<float> stack = {1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1));
}

TEST(EvalTest, Sqrt) {
  PostfixExpression expr;
  expr.add_op(Operation::Sqrt);
  std::vector<float> stack = {0, 7};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::sqrt(7)));
}

TEST(EvalTest, SqrtStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Sqrt);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Exp) {
  PostfixExpression expr;
  expr.add_op(Operation::Exp);
  std::vector<float> stack = {0, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::exp(4)));
}

TEST(EvalTest, ExpStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Exp);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Ln) {
  PostfixExpression expr;
  expr.add_op(Operation::Ln);
  std::vector<float> stack = {0, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::log(5)));
}

TEST(EvalTest, LnStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Ln);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Sin) {
  PostfixExpression expr;
  expr.add_op(Operation::Sin);
  std::vector<float> stack = {0, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::sin(5)));
}

TEST(EvalTest, SinStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Sin);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Cos) {
  PostfixExpression expr;
  expr.add_op(Operation::Cos);
  std::vector<float> stack = {0, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::cos(5)));
}

TEST(EvalTest, CosStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Cos);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Tan) {
  PostfixExpression expr;
  expr.add_op(Operation::Tan);
  std::vector<float> stack = {0, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::tan(5)));
}

TEST(EvalTest, TanStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Tan);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Asin) {
  PostfixExpression expr;
  expr.add_op(Operation::Asin);
  std::vector<float> stack = {0, 0.5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::asin(0.5)));
}

TEST(EvalTest, AsinStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Asin);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Acos) {
  PostfixExpression expr;
  expr.add_op(Operation::Acos);
  std::vector<float> stack = {0, 0.5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::acos(0.5)));
}

TEST(EvalTest, AcosStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Acos);
  std::vector<float> stack = {};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre());
}

TEST(EvalTest, Atan2) {
  PostfixExpression expr;
  expr.add_op(Operation::Atan2);
  std::vector<float> stack = {0, 5, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::atan2(5, 4)));
}

TEST(EvalTest, Atan2StackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Atan2);
  std::vector<float> stack = {5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(5));
}

TEST(EvalTest, PolyVec) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 79));
}

TEST(EvalTest, PolyVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(2, 3, 4));
}

TEST(EvalTest, PolyVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  std::vector<float> stack = {0, 2, 3, 4, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, 5));
}

TEST(EvalTest, PolyVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, 5, 6));
}

TEST(EvalTest, PushPolyVec) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  expr.add_i(4 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  expr.add_f(6);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 79));
}

TEST(EvalTest, PushPolyVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyVec);
  expr.add_i(4 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2));
}

TEST(EvalTest, PolyMat) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  expr.add_i(2 << 1);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PolyMatStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  expr.add_i(2 << 1);
  std::vector<float> stack = {2, 3, 4, 5, 6, 7, 8, 9};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(2, 3, 4, 5, 6, 7, 8, 9));
}

TEST(EvalTest, PolyMatIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(EvalTest, PolyMatIllegalOperationRows) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(-1);
  expr.add_i(2 << 1);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(EvalTest, PolyMatIllegalOperationCols) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, 5, 6, 7, 8, 9, 10));
}

TEST(EvalTest, PushPolyMat) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  expr.add_i(2 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  expr.add_f(6);
  expr.add_f(7);
  expr.add_f(8);
  expr.add_f(9);
  expr.add_f(10);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PushPolyMatFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::PolyMat);
  expr.add_i(4);
  expr.add_i(2 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(5);
  expr.add_f(6);
  expr.add_f(7);
  expr.add_f(8);
  expr.add_f(9);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2));
}

TEST(EvalTest, AddVec) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, AddVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2, 3, 4, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(EvalTest, AddVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, AddVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, PushAddVec) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  expr.add_f(6);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, PushAddVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::AddVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, SubVec) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 2, 1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, SubVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2, 3, 4, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(EvalTest, SubVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, SubVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 2, 1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 2, 1));
}

TEST(EvalTest, PushSubVec) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(2);
  expr.add_f(1);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, PushSubVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::SubVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, MulVec) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 3, -1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, MulVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2, 3, 4, 5};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5));
}

TEST(EvalTest, MulVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6));
}

TEST(EvalTest, MulVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 3, -1};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 3, -1));
}

TEST(EvalTest, PushMulVec) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(3);
  expr.add_f(-1);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, PushMulVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, MulAddVec) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAddVec);
  expr.add_i(3);
  std::vector<float> stack = {0, 1, 2, 3, 4, 3, -1, 0, 1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, MulAddVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAddVec);
  expr.add_i(3);
  std::vector<float> stack = {1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5, 6, 7, 8));
}

TEST(EvalTest, MulAddVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAddVec);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8));
}

TEST(EvalTest, MulAddVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::MulAddVec);
  expr.add_i(-1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8));
}

TEST(EvalTest, ScaleVec) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 2, 3, 4, -2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, ScaleVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3));
}

TEST(EvalTest, ScaleVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  std::vector<float> stack = {0, 1, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4));
}

TEST(EvalTest, ScaleVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 2, 3, 4, -2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4, -2));
}

TEST(EvalTest, PushScaleVec) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(-2);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, PushScaleVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::ScaleVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2));
}

TEST(EvalTest, NegVec) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 3, 4, -2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, NegVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, NegVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  std::vector<float> stack = {0, 1, 2, 3};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, NegVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 3, 4, -2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 3, 4, -2));
}

TEST(EvalTest, PushNegVec) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(3);
  expr.add_f(4);
  expr.add_f(-2);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, PushNegVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NegVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(4);
  expr.add_f(5);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0));
}

TEST(EvalTest, NormVec) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, NormVecStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  expr.add_i(3 << 1);
  std::vector<float> stack = {1, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2));
}

TEST(EvalTest, NormVecIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  std::vector<float> stack = {0, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4));
}

TEST(EvalTest, NormVecIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 2, 3, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 2, 3, 4));
}

TEST(EvalTest, PushNormVec) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(2);
  expr.add_f(3);
  expr.add_f(4);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, PushNormVecFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::NormVec);
  expr.add_i(3 << 1 | 1);
  expr.add_f(2);
  expr.add_f(3);
  std::vector<float> stack = {0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0));
}

TEST(EvalTest, MulMat) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(2);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(
    0,
    1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
    4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12));
}

TEST(EvalTest, MulMatStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(2);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11));
}

TEST(EvalTest, MulMatIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(2);
  expr.add_i(3);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
}

TEST(EvalTest, MulMatIllegalOperationA) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(-1);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
}

TEST(EvalTest, MulMatIllegalOperationB) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(2);
  expr.add_i(-1);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
}

TEST(EvalTest, MulMatIllegalOperationC) {
  PostfixExpression expr;
  expr.add_op(Operation::MulMat);
  expr.add_i(2);
  expr.add_i(3);
  expr.add_i(-1 << 1);
  std::vector<float> stack = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12));
}

TEST(EvalTest, Lerp) {
  PostfixExpression expr;
  expr.add_op(Operation::Lerp);
  expr.add_i(3);
  std::vector<float> stack = {0, 0.25, 2, 3, 4, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, LerpStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Lerp);
  expr.add_i(3);
  std::vector<float> stack = {0.25, 2, 3, 4, 6, 7};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0.25, 2, 3, 4, 6, 7));
}

TEST(EvalTest, LerpIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Lerp);
  std::vector<float> stack = {0, 0.25, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 0.25, 2, 3, 4, 5, 6, 7, 8));
}

TEST(EvalTest, LerpIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Lerp);
  expr.add_i(-1);
  std::vector<float> stack = {0, 0.25, 2, 3, 4, 6, 7, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 0.25, 2, 3, 4, 6, 7, 8));
}

TEST(EvalTest, Lut_n1) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, -1, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, Lut_0) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 0, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, Lut_0_5) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 0.5, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 1.75, 2.25, 4));
}

TEST(EvalTest, Lut_2) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 2, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 4, 3, 7));
}

TEST(EvalTest, Lut_4) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 4, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, Lut_6) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 6, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, Lut_7) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 7, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, LutStackUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(3 << 1);
  std::vector<float> stack = {0.5, 0, 1, 2, 2, 4, 3, 6, 8};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::StackUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0.5, 0, 1, 2, 2, 4, 3, 6, 8));
}

TEST(EvalTest, LutIntUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  std::vector<float> stack = {0, 0.5, 0, 1, 2, 2, 4, 3, 6, 8, 2};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IntLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 0.5, 0, 1, 2, 2, 4, 3, 6, 8, 2));
}

TEST(EvalTest, LutIllegalOperation) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(0);
  expr.add_i(4 << 1);
  std::vector<float> stack = {0, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, testing::ElementsAre(0, 4));
}

TEST(EvalTest, PushLut) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1 | 1);
  expr.add_f(0);
  expr.add_f(1);
  expr.add_f(2);
  expr.add_f(3);
  expr.add_f(2);
  expr.add_f(4);
  expr.add_f(3);
  expr.add_f(7);
  expr.add_f(6);
  expr.add_f(8);
  expr.add_f(2);
  expr.add_f(0);
  std::vector<float> stack = {0, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, PushLutFloatUnderflow) {
  PostfixExpression expr;
  expr.add_op(Operation::Lut);
  expr.add_i(3);
  expr.add_i(4 << 1 | 1);
  expr.add_f(0);
  expr.add_f(1);
  expr.add_f(2);
  expr.add_f(3);
  expr.add_f(2);
  expr.add_f(4);
  expr.add_f(3);
  expr.add_f(7);
  expr.add_f(6);
  expr.add_f(8);
  expr.add_f(2);
  std::vector<float> stack = {0, 4};
  EXPECT_EQ(Eval(expr, stack), EvalStatus::FloatLiteralsUnderflow);
  EXPECT_THAT(stack, testing::ElementsAre(0, 4));
}