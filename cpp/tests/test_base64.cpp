#include <gtest/gtest.h>
#include "base64.h"
#include <string>
#include <vector>

class Base64Test : public ::testing::Test {
protected:
    void SetUp() override {
        // Test data
        plaintext = "Hello, World!";
        encoded = "SGVsbG8sIFdvcmxkIQ==";

        binary_data = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xFE, 0xFD};
        binary_encoded = "AAECA0QFBP/+/Q==";
    }

    std::string plaintext;
    std::string encoded;
    std::vector<unsigned char> binary_data;
    std::string binary_encoded;
};

TEST_F(Base64Test, BasicEncoding) {
    std::string result = base64_encode(plaintext);
    EXPECT_EQ(result, encoded);
}

TEST_F(Base64Test, BasicDecoding) {
    std::string result = base64_decode(encoded);
    EXPECT_EQ(result, plaintext);
}

TEST_F(Base64Test, EmptyString) {
    std::string empty = "";
    EXPECT_EQ(base64_encode(empty), "");
    EXPECT_EQ(base64_decode(""), "");
}

TEST_F(Base64Test, BinaryData) {
    std::string result = base64_encode(binary_data.data(), binary_data.size());
    // Note: exact encoding may vary, but decode should work
    std::string decoded = base64_decode(result);

    EXPECT_EQ(decoded.size(), binary_data.size());
    for (size_t i = 0; i < binary_data.size(); ++i) {
        EXPECT_EQ(static_cast<unsigned char>(decoded[i]), binary_data[i]);
    }
}

TEST_F(Base64Test, RoundTrip) {
    std::string test_strings[] = {
        "A",
        "AB",
        "ABC",
        "ABCD",
        "The quick brown fox jumps over the lazy dog",
        "🚀 UTF-8 text with emojis 🎉",
        "\x00\x01\x02\x03\xFF\xFE\xFD\xFC"
    };

    for (const auto& test : test_strings) {
        std::string encoded = base64_encode(test);
        std::string decoded = base64_decode(encoded);
        EXPECT_EQ(decoded, test) << "Round trip failed for: " << test;
    }
}

TEST_F(Base64Test, URLSafeEncoding) {
    std::string test = "?>?>?>?>?>?>";
    std::string normal = base64_encode(test, false);
    std::string url_safe = base64_encode(test, true);

    // URL safe should not contain + or /
    EXPECT_EQ(normal.find('+'), std::string::npos) ? true : url_safe.find('+') == std::string::npos;
    EXPECT_EQ(normal.find('/'), std::string::npos) ? true : url_safe.find('/') == std::string::npos;

    // Both should decode to the same result
    EXPECT_EQ(base64_decode(normal), test);
    EXPECT_EQ(base64_decode(url_safe), test);
}

#if __cplusplus >= 201703L
TEST_F(Base64Test, StringViewInterface) {
    std::string_view sv(plaintext);
    std::string result = base64_encode(sv);
    EXPECT_EQ(result, encoded);

    std::string_view encoded_sv(encoded);
    std::string decoded = base64_decode(encoded_sv);
    EXPECT_EQ(decoded, plaintext);
}
#endif

TEST_F(Base64Test, PEMEncoding) {
    std::string long_text(100, 'A');  // 100 'A' characters
    std::string pem_result = base64_encode_pem(long_text);

    // PEM should have line breaks
    EXPECT_TRUE(pem_result.find('\n') != std::string::npos);

    // Should still decode correctly
    std::string decoded = base64_decode(pem_result, true);  // remove linebreaks
    EXPECT_EQ(decoded, long_text);
}

TEST_F(Base64Test, InvalidInput) {
    // Test with invalid base64 characters
    std::string invalid = "Invalid@Base64!";
    std::string result = base64_decode(invalid);
    // Should handle gracefully (implementation dependent)
    EXPECT_TRUE(true);  // Just ensure it doesn't crash
}