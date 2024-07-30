add_library(patheval PathEval.h PathEval.cc)
target_link_libraries(patheval proto postfix)

if(NOT WICKEDWINCHPROTOCOL_TESTING_DISABLED)
  include(GoogleTest)

  add_executable(
    PathEval_test
    PathEval_test.cc
  )
  target_link_libraries(
    PathEval_test
    GTest::gmock
    GTest::gtest_main
    patheval
  )
  gtest_discover_tests(PathEval_test)
endif()