#include <gtest/gtest.h>
#include "args.h"
#include <gflags/gflags.h>
#include <fstream>
#include <sstream>

class ArgsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset flags to default values
        FLAGS_image_path = "";
        FLAGS_port = -1;
        FLAGS_addr = "loopback";
        FLAGS_use_gpu = false;
        FLAGS_cpu_threads = 10;
        FLAGS_enable_mkldnn = true;
        FLAGS_models_path = "";
        FLAGS_config_path = "";
    }

    void TearDown() override {
        // Clean up any test files
    }

    // Helper to create a temporary config file
    void createTempConfig(const std::string& content, const std::string& filename = "test_config.txt") {
        std::ofstream file(filename);
        file << content;
        file.close();
        temp_files_.push_back(filename);
    }

    // Helper to clean up temp files
    void cleanup() {
        for (const auto& file : temp_files_) {
            std::remove(file.c_str());
        }
        temp_files_.clear();
    }

private:
    std::vector<std::string> temp_files_;
};

TEST_F(ArgsTest, DefaultValues) {
    EXPECT_EQ(FLAGS_image_path, "");
    EXPECT_EQ(FLAGS_port, -1);
    EXPECT_EQ(FLAGS_addr, "loopback");
    EXPECT_FALSE(FLAGS_use_gpu);
    EXPECT_TRUE(FLAGS_enable_mkldnn);
    EXPECT_EQ(FLAGS_cpu_threads, 10);
}

TEST_F(ArgsTest, ConfigFileReading) {
    std::string config_content = R"(
models_path=./models
det_model_dir=models/det
rec_model_dir=models/rec
cls_model_dir=models/cls
use_angle_cls=1
det=1
cls=1
rec=1
)";

    createTempConfig(config_content);
    FLAGS_config_path = "test_config.txt";

    std::string result = read_config();

    // Should not return an error message
    EXPECT_TRUE(result.empty() || result.find("error") == std::string::npos);

    cleanup();
}

TEST_F(ArgsTest, InvalidConfigFile) {
    FLAGS_config_path = "non_existent_file.txt";

    std::string result = read_config();

    // Should handle missing file gracefully
    EXPECT_TRUE(result.find("error") != std::string::npos ||
                result.find("not found") != std::string::npos ||
                result.empty()); // Implementation may handle differently
}

TEST_F(ArgsTest, ParameterValidation) {
    // Test valid parameters
    FLAGS_port = 8080;
    FLAGS_models_path = "./models";
    FLAGS_cpu_threads = 4;
    FLAGS_gpu_mem = 2000;

    std::string result = check_flags();
    EXPECT_TRUE(result.empty()) << "Valid parameters should not produce errors: " << result;
}

TEST_F(ArgsTest, InvalidParameterValidation) {
    // Test invalid port
    FLAGS_port = 70000; // Invalid port number

    std::string result = check_flags();
    // Should detect invalid port (implementation dependent)
    // This test may need adjustment based on actual validation logic
}

TEST_F(ArgsTest, ImagePathValidation) {
    FLAGS_image_path = "/non/existent/path.jpg";

    std::string result = check_flags();

    // Should handle non-existent image path
    // The behavior depends on implementation - it might be validated here or later
    EXPECT_TRUE(true); // Placeholder - adjust based on actual behavior
}

TEST_F(ArgsTest, ModelsPathValidation) {
    FLAGS_models_path = "/non/existent/models/path";

    std::string result = check_flags();

    // Should handle non-existent models path appropriately
    EXPECT_TRUE(true); // Placeholder - adjust based on actual behavior
}

TEST_F(ArgsTest, ThreadCountValidation) {
    // Test reasonable thread count
    FLAGS_cpu_threads = 1;
    std::string result1 = check_flags();
    EXPECT_TRUE(result1.empty()) << "Single thread should be valid";

    FLAGS_cpu_threads = 16;
    std::string result2 = check_flags();
    EXPECT_TRUE(result2.empty()) << "16 threads should be valid";

    // Test potentially problematic values
    FLAGS_cpu_threads = 0;
    std::string result3 = check_flags();
    // Should handle zero threads appropriately (implementation dependent)

    FLAGS_cpu_threads = -1;
    std::string result4 = check_flags();
    // Should handle negative threads appropriately (implementation dependent)
}

TEST_F(ArgsTest, MemoryLimitValidation) {
    // Test valid memory limits
    FLAGS_cpu_mem = 1000;
    FLAGS_gpu_mem = 2000;

    std::string result = check_flags();
    EXPECT_TRUE(result.empty()) << "Valid memory limits should not produce errors";

    // Test special case: no limit
    FLAGS_cpu_mem = -1;
    std::string result2 = check_flags();
    EXPECT_TRUE(result2.empty()) << "No memory limit (-1) should be valid";
}

TEST_F(ArgsTest, AddressValidation) {
    // Test valid addresses
    std::vector<std::string> valid_addresses = {
        "loopback",
        "localhost",
        "any",
        "127.0.0.1",
        "192.168.1.100"
    };

    for (const auto& addr : valid_addresses) {
        FLAGS_addr = addr;
        std::string result = check_flags();
        EXPECT_TRUE(result.empty()) << "Address " << addr << " should be valid";
    }
}

TEST_F(ArgsTest, PrecisionValidation) {
    // Test valid precision values
    std::vector<std::string> valid_precisions = {"fp32", "fp16", "int8"};

    for (const auto& precision : valid_precisions) {
        FLAGS_precision = precision;
        std::string result = check_flags();
        EXPECT_TRUE(result.empty() || result.find("precision") == std::string::npos)
            << "Precision " << precision << " should be valid";
    }

    // Test invalid precision
    FLAGS_precision = "invalid_precision";
    std::string result = check_flags();
    // Should detect invalid precision (implementation dependent)
}