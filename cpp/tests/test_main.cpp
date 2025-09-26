#include <gtest/gtest.h>

// Main test runner - gtest_main handles this automatically
// This file can be used for global test setup if needed

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}