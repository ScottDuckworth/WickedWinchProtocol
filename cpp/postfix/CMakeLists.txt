add_library(postfix PostfixExpression.h PostfixExpression.cc)
target_link_libraries(postfix proto)

if(NOT WICKEDWINCHPROTOCOL_TESTING_DISABLED)
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
endif()