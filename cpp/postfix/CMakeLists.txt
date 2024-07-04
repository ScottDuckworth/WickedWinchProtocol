add_library(postfix PostfixExpression.h PostfixExpression.cc)
target_link_libraries(postfix proto)
set_property(TARGET postfix PROPERTY CXX_STANDARD 20)