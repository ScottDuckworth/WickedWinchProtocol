#include "patheval/PathEval.h"
#include "postfix/PostfixExpression.h"

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using wickedwinch::patheval::PathAt;
using wickedwinch::patheval::PathEval;
using wickedwinch::proto::Path;
using wickedwinch::proto::PathSegment;
using wickedwinch::proto::PostfixExpression;
using wickedwinch::proto::Operation;

TEST(PathEvalTest, Empty) {
  Path path;
  PathEval eval = PathAt(path, 0);
  EXPECT_EQ(eval.segment, nullptr);
}

TEST(PathEvalTest, At50) {
  Path path;
  PathSegment* segment = path.add_segments();
  segment->set_start_time(100);
  PathEval eval = PathAt(path, 50);
  EXPECT_EQ(eval.segment, nullptr);
}

TEST(PathEvalTest, At100) {
  Path path;
  PathSegment* segment0 = path.add_segments();
  segment0->set_start_time(100);
  PathSegment* segment1 = path.add_segments();
  segment1->set_start_time(200);
  PathEval eval = PathAt(path, 100);
  EXPECT_EQ(eval.segment, segment0);
  EXPECT_EQ(eval.t, 0);
}

TEST(PathEvalTest, At150) {
  Path path;
  PathSegment* segment0 = path.add_segments();
  segment0->set_start_time(100);
  PathSegment* segment1 = path.add_segments();
  segment1->set_start_time(200);
  PathEval eval = PathAt(path, 150);
  EXPECT_EQ(eval.segment, segment0);
  EXPECT_EQ(eval.t, 50);
}

TEST(PathEvalTest, At200) {
  Path path;
  PathSegment* segment0 = path.add_segments();
  segment0->set_start_time(100);
  PathSegment* segment1 = path.add_segments();
  segment1->set_start_time(200);
  PathEval eval = PathAt(path, 200);
  EXPECT_EQ(eval.segment, segment1);
  EXPECT_EQ(eval.t, 0);
}

TEST(PathEvalTest, At210) {
  Path path;
  PathSegment* segment0 = path.add_segments();
  segment0->set_start_time(100);
  PathSegment* segment1 = path.add_segments();
  segment1->set_start_time(200);
  PathEval eval = PathAt(path, 210);
  EXPECT_EQ(eval.segment, segment1);
  EXPECT_EQ(eval.t, 10);
}

TEST(PathEvalTest, Eval) {
  Path path;
  PathSegment* segment0 = path.add_segments();
  segment0->set_start_time(100);
  segment0->mutable_expr()->add_i(1);
  segment0->mutable_expr()->add_f(10);
  segment0->mutable_expr()->add_op(Operation::Push);
  segment0->mutable_expr()->add_op(Operation::Add);
  PathSegment* segment1 = path.add_segments();
  segment1->set_start_time(200);
  PathEval eval = PathAt(path, 150);
  EXPECT_EQ(eval.segment, segment0);
  EXPECT_EQ(eval.t, 50);
  std::vector<float> stack;
  EXPECT_EQ(eval.Eval(stack), wickedwinch::postfix::EvalStatus::Ok);
  EXPECT_THAT(stack, testing::ElementsAre(10.05));
}