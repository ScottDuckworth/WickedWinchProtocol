#include "Path.h"
#include "Postfix.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::FloatEq;
using ::testing::Pointwise;

namespace wickedwinch::protocol {
namespace {

struct TestStack : PostfixStack {
  float buffer[8];

  TestStack() {
    stack_data = buffer;
    stack_size = 0;
    stack_capacity = sizeof(buffer);
  }
};

TEST(PathEvalTest, Empty) {
  PathReader reader;

  TestStack stack;
  EXPECT_EQ(reader.Eval(0, stack), EvalStatus::UndefinedOperation);

  PathWriter writer;
  writer.set_target(123);

  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));
  EXPECT_EQ(reader.target(), 123);
  EXPECT_EQ(reader.segment_header_size(), 0);

  EXPECT_EQ(reader.Eval(0, stack), EvalStatus::UndefinedOperation);
}

TEST(PathEvalTest, Eval) {
  PathWriter writer;
  PathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = 1000;
  segment = writer.add_segments();
  segment->start_time = 2000;
  segment->expr.Pop(1);
  segment->expr.Push({99});

  PathReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));
  EXPECT_EQ(reader.flags(), 0);
  EXPECT_EQ(reader.segment_header_size(), 2);

  TestStack stack;

  EXPECT_EQ(reader.Eval(500, stack), EvalStatus::UndefinedOperation);

  EXPECT_EQ(reader.Eval(1000, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(1750, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0.75}));

  EXPECT_EQ(reader.Eval(2000, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {99}));

  EXPECT_EQ(reader.Eval(2100, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {99}));
}

TEST(PathEvalTest, Overflow) {
  PathWriter writer;
  PathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = -1000;
  segment = writer.add_segments();
  segment->start_time = 1000;

  PathReader reader;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));
  EXPECT_EQ(reader.flags(), PathHeader::Overflow);
  EXPECT_EQ(reader.segment_header_size(), 2);

  TestStack stack;

  EXPECT_EQ(reader.Eval(-1000, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(-500, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0.5}));

  EXPECT_EQ(reader.Eval(0, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {1}));

  EXPECT_EQ(reader.Eval(500, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {1.5}));

  EXPECT_EQ(reader.Eval(1000, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(1500, stack), EvalStatus::Ok);
  EXPECT_THAT(stack, Pointwise(FloatEq(), {0.5}));
}

}
}
