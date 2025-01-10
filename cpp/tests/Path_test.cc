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
  PathReader reader;

  TestEval eval;
  EXPECT_EQ(reader.Eval(0, eval.eval), WickedEvalStatus_UndefinedOperation);

  PathWriter writer;
  auto buffer = writer.Write();
  EXPECT_TRUE(reader.Read(buffer));
  EXPECT_EQ(reader.segment_header_size(), 0);

  EXPECT_EQ(reader.Eval(0, eval.eval), WickedEvalStatus_UndefinedOperation);
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

  TestEval eval;

  EXPECT_EQ(reader.Eval(500, eval.eval), WickedEvalStatus_UndefinedOperation);

  EXPECT_EQ(reader.Eval(1000, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(1750, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0.75}));

  EXPECT_EQ(reader.Eval(2000, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {99}));

  EXPECT_EQ(reader.Eval(2100, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {99}));
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

  TestEval eval;

  EXPECT_EQ(reader.Eval(-1000, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(-500, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0.5}));

  EXPECT_EQ(reader.Eval(0, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1}));

  EXPECT_EQ(reader.Eval(500, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {1.5}));

  EXPECT_EQ(reader.Eval(1000, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0}));

  EXPECT_EQ(reader.Eval(1500, eval.eval), WickedEvalStatus_Ok);
  EXPECT_THAT(eval.stack(), Pointwise(FloatEq(), {0.5}));
}

}
}
