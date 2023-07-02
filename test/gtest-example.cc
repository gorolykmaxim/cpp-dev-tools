#include <gtest/gtest.h>

#include <iostream>
#include <thread>

TEST(ExampleTest, TestCase1) {
  std::cout << "Hello World Test\n";
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_TRUE(true);
}

TEST(ExampleTest2, TestCase1) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_FALSE(false);
}

class TestGroup : public testing::Test {};

TEST_F(TestGroup, TestCase3) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(1, 1);
  EXPECT_STREQ("hello", "hello");
}

TEST_F(TestGroup, TestCase4) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(1, 1);
}
