#include <WickedWinchProtocol/Postfix.h>

#include <cmath>
#include <span>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ElementsAre;

namespace wickedwinch::protocol {
namespace {

struct TestStack : PostfixStack {
  TestStack(size_t capacity, std::initializer_list<float> values) {
    assert(capacity >= values.size());
    stack_data = new float[capacity];
    stack_size = values.size();
    stack_capacity = capacity;
    float* v = stack_data;
    for (float value : values) {
      *v++ = value;
    }
  }

  ~TestStack() {
    delete stack_data;
  }
};

TEST(EvalTest, Empty) {
  PostfixWriter writer;

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {42});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(42));
}

TEST(EvalTest, Push) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Push);
  writer.add_i(2);
  writer.add_f(1);
  writer.add_f(2);
  EXPECT_EQ(writer.op_size(), 1);
  EXPECT_EQ(writer.i_size(), 1);
  EXPECT_EQ(writer.f_size(), 2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {42});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(42, 1, 2));
}

TEST(EvalTest, PushMany) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Push);
  writer.add_op(PostfixOp::Push);
  writer.add_i(2);
  writer.add_i(1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  EXPECT_EQ(writer.op_size(), 2);
  EXPECT_EQ(writer.i_size(), 2);
  EXPECT_EQ(writer.f_size(), 3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {42});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(42, 1, 2, 3));
}

TEST(EvalTest, PushIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Push);
  writer.add_f(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {42});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Push);
  writer.add_i(2);
  writer.add_f(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {42});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, Pop) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Pop);
  writer.add_i(2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(1));
}

TEST(EvalTest, PopStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Pop);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, PopIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Pop);
  writer.add_f(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, Dup) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Dup);
  writer.add_i(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(1, 2, 3, 2));
}

TEST(EvalTest, DupStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Dup);
  writer.add_i(2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, DupIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Dup);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, RotL) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotL);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(1, 3, 4, 2));
}

TEST(EvalTest, RotLStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotL);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, RotLIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotL);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, RotR) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotR);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(1, 4, 2, 3));
}

TEST(EvalTest, RotRStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotR);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, RotRIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::RotR);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, Rev) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Rev);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(1, 4, 3, 2));
}

TEST(EvalTest, RevStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Rev);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, RevIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Rev);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, Transpose) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, TransposeStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2, 3, 4, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, TransposeIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Transpose);
  writer.add_i(2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushTranspose) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1, 4, 2, 5, 3, 6));
}

TEST(EvalTest, PushTransposeFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Transpose);
  writer.add_i(2);
  writer.add_i(3 << 1 | 1);
  writer.add_f(1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, Add) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Add);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3));
}

TEST(EvalTest, AddStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Add);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Sub) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sub);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -1));
}

TEST(EvalTest, SubStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sub);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Mul) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Mul);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 6));
}

TEST(EvalTest, MulStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Mul);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, MulAdd) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAdd);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 3, 2, 1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 7));
}

TEST(EvalTest, MulAddStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAdd);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Div) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Div);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 0.5));
}

TEST(EvalTest, DivStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Div);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Mod) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Mod);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 8, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 2));
}

TEST(EvalTest, ModStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Mod);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Neg) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Neg);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -2));
}

TEST(EvalTest, NegStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Neg);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Abs) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Abs);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, -2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 2));
}

TEST(EvalTest, AbsStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Abs);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Inv) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Inv);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 0.5));
}

TEST(EvalTest, InvStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Inv);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Pow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Pow);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 8));
}

TEST(EvalTest, PowStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Pow);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Sqrt) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sqrt);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 7});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::sqrt(7)));
}

TEST(EvalTest, SqrtStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sqrt);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Exp) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Exp);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::exp(4)));
}

TEST(EvalTest, ExpStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Exp);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Ln) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Ln);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::log(5)));
}

TEST(EvalTest, LnStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Ln);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Sin) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sin);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::sin(5)));
}

TEST(EvalTest, SinStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Sin);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Cos) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Cos);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::cos(5)));
}

TEST(EvalTest, CosStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Cos);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Tan) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Tan);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::tan(5)));
}

TEST(EvalTest, TanStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Tan);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Asin) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Asin);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 0.5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::asin(0.5)));
}

TEST(EvalTest, AsinStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Asin);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Acos) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Acos);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 0.5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::acos(0.5)));
}

TEST(EvalTest, AcosStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Acos);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, Atan2) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Atan2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {0, 5, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::atan2(5, 4)));
}

TEST(EvalTest, Atan2StackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Atan2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(4, {5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, PolyVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyVec);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(6, {0, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 79));
}

TEST(EvalTest, PolyVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(6, {2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, PolyVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(6, {0, 2, 3, 4, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushPolyVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyVec);
  writer.add_i(4 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(6, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 79));
}

TEST(EvalTest, PushPolyVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyVec);
  writer.add_i(4 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(6, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, PolyMat) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PolyMatStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {2, 3, 4, 5, 6, 7, 8, 9});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, PolyMatIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyMat);
  writer.add_i(4);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 2, 3, 4, 5, 6, 7, 8, 9, 10});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushPolyMat) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyMat);
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

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3 + 2*5 + 4*7 + 8*9, 4 + 2*6 + 4*8 + 8*10));
}

TEST(EvalTest, PushPolyMatFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::PolyMat);
  writer.add_i(4);
  writer.add_i(2 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);
  writer.add_f(9);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, AddVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::AddVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, AddVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::AddVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2, 3, 4, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, AddVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::AddVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushAddVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::AddVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);
  writer.add_f(6);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 5, 7, 9));
}

TEST(EvalTest, PushAddVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::AddVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, SubVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::SubVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 2, 1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, SubVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::SubVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2, 3, 4, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, SubVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::SubVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushSubVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::SubVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(2);
  writer.add_f(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -3, 0, 2));
}

TEST(EvalTest, PushSubVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::SubVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, MulVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, MulVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2, 3, 4, 5});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, MulVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushMulVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(-1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 4, 6, -3));
}

TEST(EvalTest, PushMulVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, MulAddVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);
  writer.add_i(3 << 2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 1, 2, 3, 4, 3, -1, 0, 1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, MulAddVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);
  writer.add_i(3 << 2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, MulAddVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushMulAddVec1) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);
  writer.add_i(3 << 2 | 1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, PushMulAddVec2) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);
  writer.add_i(3 << 2 | 2);
  writer.add_f(4);
  writer.add_f(3);
  writer.add_f(-1);
  writer.add_f(0);
  writer.add_f(1);
  writer.add_f(2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1 * 4 + 0, 2 * 3 + 1, 3 * -1 + 2));
}

TEST(EvalTest, PushMulAddVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulAddVec);
  writer.add_i(3 << 2 | 1);
  writer.add_f(0);
  writer.add_f(1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 1, 2, 3, 4, 3, -1});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, ScaleVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::ScaleVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 2, 3, 4, -2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, ScaleVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::ScaleVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, ScaleVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::ScaleVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushScaleVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::ScaleVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(-2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 6, 8, -4));
}

TEST(EvalTest, PushScaleVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::ScaleVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, NegVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NegVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 3, 4, -2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, NegVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NegVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, NegVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NegVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 1, 2, 3});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushNegVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NegVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(-2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, -3, -4, 2));
}

TEST(EvalTest, PushNegVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NegVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(4);
  writer.add_f(5);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, NormVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NormVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, NormVecStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NormVec);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {1, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, NormVecIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NormVec);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushNormVec) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NormVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, std::sqrt(2*2 + 3*3 + 4*4)));
}

TEST(EvalTest, PushNormVecFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::NormVec);
  writer.add_i(3 << 1 | 1);
  writer.add_f(2);
  writer.add_f(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, MulMat) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(
    0,
    1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
    4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12));
}

TEST(EvalTest, MulMatStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulMat);
  writer.add_i(2);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(32, {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, MulMatIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulMat);
  writer.add_i(2);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(32, {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushMulMat) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulMat);
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

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(32, {0, 1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(
    0,
    1*1 + 2*5 + 3*9, 1*2 + 2*6 + 3*10, 1*3 + 2*7 + 3*11, 1*4 + 2*8 + 3*12,
    4*1 + 5*5 + 6*9, 4*2 + 5*6 + 6*10, 4*3 + 5*7 + 6*11, 4*4 + 5*8 + 6*12));
}

TEST(EvalTest, PushMulMatFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::MulMat);
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

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(32, {1, 2, 3, 4, 5, 6});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, Lerp) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);
  writer.add_i(3<<2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 0.25, 2, 3, 4, 6, 7, 8});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, LerpStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);
  writer.add_i(3<<2);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0.25, 2, 3, 4, 6, 7});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, LerpIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(12, {0, 0.25, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, PushLerp1) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);
  writer.add_i(3<<2|1);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 0.25, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, PushLerp2) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);
  writer.add_i(3<<2|2);
  writer.add_f(2);
  writer.add_f(3);
  writer.add_f(4);
  writer.add_f(6);
  writer.add_f(7);
  writer.add_f(8);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 0.25});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 3, 4, 5));
}

TEST(EvalTest, PushLerpFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lerp);
  writer.add_i(3<<2|1);
  writer.add_f(6);
  writer.add_f(7);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(8, {0, 0.25, 2, 3, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

TEST(EvalTest, Lut_n1) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, -1, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, Lut_0) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 0, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1, 2, 3));
}

TEST(EvalTest, Lut_0_5) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 0.5, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 1.75, 2.25, 4));
}

TEST(EvalTest, Lut_2) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 2, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 4, 3, 7));
}

TEST(EvalTest, Lut_4) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 4, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, Lut_6) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 6, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, Lut_7) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 7, 0, 1, 2, 3, 2, 4, 3, 7, 6, 8, 2, 0});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 8, 2, 0));
}

TEST(EvalTest, LutStackUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);
  writer.add_i(3 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0.5, 0, 1, 2, 2, 4, 3, 6, 8});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::StackUnderflow);
}

TEST(EvalTest, LutIntUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(3);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 0.5, 0, 1, 2, 2, 4, 3, 6, 8, 2});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IntLiteralsUnderflow);
}

TEST(EvalTest, LutIllegalOperation) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
  writer.add_i(0);
  writer.add_i(4 << 1);

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::IllegalOperation);
  EXPECT_THAT(stack, ElementsAre(0, 4));
}

TEST(EvalTest, PushLut) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
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

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::Ok);
  EXPECT_THAT(stack, ElementsAre(0, 6, 2.5, 3.5));
}

TEST(EvalTest, PushLutFloatUnderflow) {
  PostfixWriter writer;
  writer.add_op(PostfixOp::Lut);
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

  PostfixReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));

  TestStack stack(16, {0, 4});
  EXPECT_EQ(stack.Eval(reader), EvalStatus::FloatLiteralsUnderflow);
}

}
}
