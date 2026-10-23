#include "WickedPath.h"
#include "WickedPostfix.h"

#include <array>
#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::FloatEq;
using ::testing::Pointwise;

namespace {

struct TestEval {
  uint16_t stack_size = 1;
  uint16_t stack_capacity = sizeof(stack_buffer) / sizeof(float);
  float stack_buffer[8];

  std::span<float> stack() { return std::span<float>(stack_buffer, stack_size); }
};

TEST(PathEvalTest, Empty) {
  TestEval eval;
  EXPECT_FALSE(WickedPathValidate(nullptr, 3, eval.stack_capacity));

  WickedPathWriter writer;
  auto buffer = writer.Write();
  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 0));
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size(), eval.stack_capacity));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, eval.stack_buffer), WickedEvalStatus_UndefinedOperation);
}

TEST(PathEvalTest, NoStack) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = 0;
  auto buffer = writer.Write();
  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 0));
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size(), 1));

  float dummy = 0;
  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, &dummy), WickedEvalStatus_Ok);
}

TEST(PathEvalTest, Eval) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = 1000;
  segment = writer.add_segments();
  segment->start_time = 2000;
  segment->expr.Pop(1);
  segment->expr.Push(std::array<float, 1>{99});
  auto buffer = writer.Write();

  TestEval eval;
  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 0));
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size(), eval.stack_capacity));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1000, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1750, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {750}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 2000, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {99}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 2100, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {99}));
}

TEST(PathEvalTest, Wraparound) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = -1000;
  segment = writer.add_segments();
  segment->start_time = 1000;

  auto buffer = writer.Write();

  TestEval eval;
  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 0));
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size(), eval.stack_capacity));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), -1000, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), -500, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {500}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1000}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 500, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1500}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1000, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1500, eval.stack_buffer), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {500}));
}

TEST(PathEvalTest, UnexpectedStackSize) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment = writer.add_segments();
  segment->start_time = 0;
  // Initial stack is 1, push 2 more -> final stack is 3 (expected 1)
  segment->expr.Push(std::array<float, 2>{1, 2});
  auto buffer = writer.Write();
  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 8));
}

TEST(PathEvalTest, InsufficientStackCapacity) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment = writer.add_segments();
  segment->start_time = 0;
  // Initial stack is 1, push 2 floats (depth 3), then pop 2 floats (depth back to 1)
  segment->expr.Push(std::array<float, 2>{1, 2});
  segment->expr.Pop(2);
  auto buffer = writer.Write();

  EXPECT_FALSE(WickedPathValidate(buffer.data(), buffer.size(), 2));
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size(), 3));

  float stack_data[3];
  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, stack_data), WickedEvalStatus_Ok);
  EXPECT_EQ(stack_data[0], 0);
}

}
