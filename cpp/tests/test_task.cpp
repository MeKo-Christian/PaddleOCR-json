#include <gtest/gtest.h>
#include "task.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class TaskTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }

    void TearDown() override {
        // Clean up
    }
};

TEST_F(TaskTest, ErrorCodes) {
    // Test that error codes are properly defined
    EXPECT_EQ(CODE_INIT, 0);
    EXPECT_EQ(CODE_OK, 100);
    EXPECT_EQ(CODE_OK_NONE, 101);

    // Path-related errors
    EXPECT_EQ(CODE_ERR_PATH_EXIST, 200);
    EXPECT_EQ(CODE_ERR_PATH_CONV, 201);
    EXPECT_EQ(CODE_ERR_PATH_READ, 202);
    EXPECT_EQ(CODE_ERR_PATH_DECODE, 203);

    // Base64-related errors
    EXPECT_EQ(CODE_ERR_BASE64_DECODE, 300);
}

TEST_F(TaskTest, ErrorMessages) {
    std::string test_path = "/test/image.jpg";

    // Test path error messages
    std::string path_exist_msg = MSG_ERR_PATH_EXIST(test_path);
    EXPECT_TRUE(path_exist_msg.find(test_path) != std::string::npos);
    EXPECT_TRUE(path_exist_msg.find("not exist") != std::string::npos ||
                path_exist_msg.find("dose not exist") != std::string::npos);

    std::string path_conv_msg = MSG_ERR_PATH_CONV(test_path);
    EXPECT_TRUE(path_conv_msg.find(test_path) != std::string::npos);
    EXPECT_TRUE(path_conv_msg.find("convert") != std::string::npos);

    std::string path_read_msg = MSG_ERR_PATH_READ(test_path);
    EXPECT_TRUE(path_read_msg.find(test_path) != std::string::npos);
    EXPECT_TRUE(path_read_msg.find("open") != std::string::npos ||
                path_read_msg.find("failed") != std::string::npos);

    std::string path_decode_msg = MSG_ERR_PATH_DECODE(test_path);
    EXPECT_TRUE(path_decode_msg.find(test_path) != std::string::npos);
    EXPECT_TRUE(path_decode_msg.find("decode") != std::string::npos);

    // Test OK_NONE message
    std::string ok_none_msg = MSG_OK_NONE(test_path);
    EXPECT_TRUE(ok_none_msg.find(test_path) != std::string::npos);
    EXPECT_TRUE(ok_none_msg.find("No text") != std::string::npos);
}

#if defined(_WIN32) && defined(ENABLE_CLIPBOARD)
TEST_F(TaskTest, ClipboardErrorCodes) {
    // Test clipboard error codes are properly defined
    EXPECT_EQ(CODE_ERR_CLIP_OPEN, 210);
    EXPECT_EQ(CODE_ERR_CLIP_EMPTY, 211);
    EXPECT_EQ(CODE_ERR_CLIP_FORMAT, 212);
    EXPECT_EQ(CODE_ERR_CLIP_DATA, 213);
    EXPECT_EQ(CODE_ERR_CLIP_FILES, 214);
    EXPECT_EQ(CODE_ERR_CLIP_GETOBJ, 215);
    EXPECT_EQ(CODE_ERR_CLIP_BITMAP, 216);
    EXPECT_EQ(CODE_ERR_CLIP_CHANNEL, 217);
}

TEST_F(TaskTest, ClipboardErrorMessages) {
    // Test clipboard error messages
    EXPECT_STREQ(MSG_ERR_CLIP_OPEN, "Clipboard open failed.");
    EXPECT_STREQ(MSG_ERR_CLIP_EMPTY, "Clipboard is empty.");
    EXPECT_STREQ(MSG_ERR_CLIP_FORMAT, "Clipboard format is not valid.");
    EXPECT_STREQ(MSG_ERR_CLIP_DATA, "Getting clipboard data handle failed.");

    // Test parameterized messages
    int test_n = 5;
    std::string clip_files_msg = MSG_ERR_CLIP_FILES(test_n);
    EXPECT_TRUE(clip_files_msg.find(std::to_string(test_n)) != std::string::npos);
    EXPECT_TRUE(clip_files_msg.find("files") != std::string::npos);

    int test_channels = 2;
    std::string clip_channel_msg = MSG_ERR_CLIP_CHANNEL(test_channels);
    EXPECT_TRUE(clip_channel_msg.find(std::to_string(test_channels)) != std::string::npos);
    EXPECT_TRUE(clip_channel_msg.find("channels") != std::string::npos);
}
#endif

TEST_F(TaskTest, Base64ErrorConstants) {
    // Test base64 error code
    EXPECT_EQ(CODE_ERR_BASE64_DECODE, 300);
    EXPECT_STREQ(MSG_ERR_BASE64_DECODE, "Base64 decode failed.");
}

TEST_F(TaskTest, JSONHandling) {
    // Test that nlohmann/json is properly included and functional
    json test_json;
    test_json["code"] = CODE_OK;
    test_json["message"] = "Test message";
    test_json["data"] = json::array();

    EXPECT_EQ(test_json["code"], CODE_OK);
    EXPECT_EQ(test_json["message"], "Test message");
    EXPECT_TRUE(test_json["data"].is_array());
    EXPECT_EQ(test_json["data"].size(), 0);
}

TEST_F(TaskTest, SuccessResponseFormat) {
    // Test typical success response structure
    json success_response;
    success_response["code"] = CODE_OK;
    success_response["data"] = json::array();

    // Add a typical OCR result
    json ocr_result;
    ocr_result["box"] = json::array({
        json::array({100, 50}),
        json::array({200, 50}),
        json::array({200, 80}),
        json::array({100, 80})
    });
    ocr_result["score"] = 0.95;
    ocr_result["text"] = "Sample text";

    success_response["data"].push_back(ocr_result);

    // Verify structure
    EXPECT_EQ(success_response["code"], CODE_OK);
    EXPECT_TRUE(success_response["data"].is_array());
    EXPECT_EQ(success_response["data"].size(), 1);

    auto& result = success_response["data"][0];
    EXPECT_TRUE(result["box"].is_array());
    EXPECT_EQ(result["box"].size(), 4);  // 4 corner points
    EXPECT_TRUE(result["score"].is_number());
    EXPECT_TRUE(result["text"].is_string());
}

TEST_F(TaskTest, ErrorResponseFormat) {
    // Test typical error response structure
    json error_response;
    error_response["code"] = CODE_ERR_PATH_EXIST;
    error_response["data"] = MSG_ERR_PATH_EXIST("/test/path.jpg");

    EXPECT_EQ(error_response["code"], CODE_ERR_PATH_EXIST);
    EXPECT_TRUE(error_response["data"].is_string());

    std::string error_msg = error_response["data"];
    EXPECT_TRUE(error_msg.find("/test/path.jpg") != std::string::npos);
}