add_library(postfix PostfixExpression.h PostfixExpression.cc)
target_link_libraries(postfix proto)

enable_testing()
include(GoogleTest)

add_executable(
  PostfixExpression_test
  PostfixExpression_test.cc
)
target_link_libraries(
  PostfixExpression_test
  GTest::gmock
  GTest::gtest_main
  postfix
)
gtest_discover_tests(PostfixExpression_test)