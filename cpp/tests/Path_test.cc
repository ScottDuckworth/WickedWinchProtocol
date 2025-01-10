#include <WickedWinchProtocol/Path.h>
#include <WickedWinchProtocol/Postfix.h>

#include <span>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::FloatEq;
using ::testing::Pointwise;

namespace wickedwinch::protocol {
namespace {

struct TestEval {
  struct WickedPostfixEval eval;
  float stack_buffer[8];
  float temp_buffer[4];

  TestEval() {
    eval.stack_data = stack_buffer;
    eval.stack_size = 0;
    eval.stack_capacity = sizeof(stack_buffer) / sizeof(float);

    eval.temp_data = temp_buffer;
    eval.temp_capacity = sizeof(temp_buffer) / sizeof(float);
  }

  std::span<float> stack() { return std::span<float>(eval.stack_data, eval.stack_size); }
};

TEST(PathEvalTest, Empty) {
  EXPECT_FALSE(WickedPathValidate(nullptr, 3));

  WickedPathWriter writer;
  auto buffer = writer.Write();
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size()));

  TestEval eval;
  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, &eval.eval), WickedEvalStatus_UndefinedOperation);
}

TEST(PathEvalTest, Eval) {
  WickedPathWriter writer;
  WickedPathSegmentWriter* segment;
  segment = writer.add_segments();
  segment->start_time = 1000;
  segment = writer.add_segments();
  segment->start_time = 2000;
  segment->expr.Pop(1);
  segment->expr.Push({99});
  auto buffer = writer.Write();
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size()));

  TestEval eval;

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1000, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1750, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {750}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 2000, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {99}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 2100, &eval.eval), WickedEvalStatus_Ok);
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
  EXPECT_TRUE(WickedPathValidate(buffer.data(), buffer.size()));

  TestEval eval;

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), -1000, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), -500, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {500}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 0, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1000}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 500, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1500}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1000, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(WickedPathEvaluate(buffer.data(), 1500, &eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {500}));
}

}
}
