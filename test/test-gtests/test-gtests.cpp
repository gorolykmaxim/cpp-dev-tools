#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <thread>

static void display_file_links_data() {
    std::cout << "/a/b/c:10" << std::endl;
    std::cout << "some random data" << std::endl;
    std::cout << "/d/e/f:15:32 something" << std::endl;
}

TEST(normal_tests, hello_world) {
    EXPECT_TRUE(true);
}

TEST(test_suit_1, test1) {
    EXPECT_TRUE(true);
    display_file_links_data();
}

TEST(test_suit_1, test2) {
    EXPECT_FALSE(false);
}

TEST(test_suit_2, test1) {
    EXPECT_EQ(1, 1);
}

TEST(exit_tests, exit_with_0) {
    std::exit(0);
}

TEST(exit_tests, exit_with_1) {
    std::exit(1);
}

TEST(failed_test_suit_1, test1) {
    display_file_links_data();
    throw std::runtime_error("");
}

TEST(failed_test_suit_1, test2) {
    EXPECT_TRUE(true);
}

TEST(failed_test_suit_2, test1) {
    throw std::runtime_error("");
}

TEST(long_tests, test1) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1h);
}

TEST(sporadically_failing_tests, test1) {
    const auto file1 = "test-gtest-file1.tmp";
    const auto file2 = "test-gtest-file2.tmp";
    if (std::filesystem::exists(file1) && std::filesystem::exists(file2)) {
        std::filesystem::remove(file1);
        std::filesystem::remove(file2);
        throw std::runtime_error("");
    } else if (std::filesystem::exists(file1)) {
        std::ofstream f(file2);
    } else {
        std::ofstream f(file1);
    }
}