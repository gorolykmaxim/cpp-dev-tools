#include <gtest/gtest.h>

#include <iostream>

TEST(ExampleTest, TestCase1) {
  std::cout << "Hello World Test\n";
  EXPECT_TRUE(true);
}

TEST(ExampleTest2, TestCase1) { EXPECT_FALSE(false); }

class TestGroup : public testing::Test {};

TEST_F(TestGroup, TestCase3) {
  EXPECT_EQ(1, 1);
  EXPECT_STREQ("hello", "hello");
}
