#include "paddleocr_c_api.h"
#include "paddleocr.h"
#include "utility.h"
#include <opencv2/opencv.hpp>
#include <cstring>
#include <vector>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

// Internal handle structure
struct PaddleOcrHandle {
    std::unique_ptr<PaddleOCR::PPOCR> ocr;
    PaddleOcrConfig config;
};

// Convert OCR result to C structure
static PaddleOcrResult* convert_results(const std::vector<PaddleOCR::OCRPredictResult>& cpp_results,
                                       size_t* count) {
    *count = cpp_results.size();
    if (*count == 0) return nullptr;

    PaddleOcrResult* results = (PaddleOcrResult*)malloc(*count * sizeof(PaddleOcrResult));
    if (!results) return nullptr;

    for (size_t i = 0; i < *count; ++i) {
        const auto& cpp_result = cpp_results[i];

        // Copy text
        results[i].text_length = cpp_result.text.length();
        results[i].text = (char*)malloc(results[i].text_length + 1);
        if (results[i].text) {
            strcpy(results[i].text, cpp_result.text.c_str());
        }

        // Copy other fields
        results[i].score = cpp_result.score;
        results[i].cls_label = cpp_result.cls_label;
        results[i].cls_score = cpp_result.cls_score;

        // Copy bounding box
        if (cpp_result.box.size() >= 4) {
            for (int j = 0; j < 4; ++j) {
                if (cpp_result.box[j].size() >= 2) {
                    results[i].box[j*2] = cpp_result.box[j][0];
                    results[i].box[j*2 + 1] = cpp_result.box[j][1];
                }
            }
        }
    }

    return results;
}

// Convert image data to OpenCV Mat
static cv::Mat create_mat_from_data(const uint8_t* data, size_t size, int width, int height, int channels) {
    // Assume RGB/BGR format
    int cv_type = (channels == 3) ? CV_8UC3 : CV_8UC1;
    cv::Mat img(height, width, cv_type, const_cast<uint8_t*>(data));
    return img.clone(); // Make a copy since we don't own the data
}

PaddleOcrError paddle_ocr_create(PaddleOcrConfig* config, PaddleOcrHandle** handle) {
    if (!config || !handle) {
        return PADDLE_OCR_ERROR_INIT;
    }

    try {
        *handle = new PaddleOcrHandle();
        (*handle)->config = *config;

        // Initialize PPOCR with configuration
        (*handle)->ocr = std::make_unique<PaddleOCR::PPOCR>();

        return PADDLE_OCR_SUCCESS;
    } catch (const std::exception& e) {
        return PADDLE_OCR_ERROR_INIT;
    }
}

PaddleOcrError paddle_ocr_destroy(PaddleOcrHandle* handle) {
    if (!handle) {
        return PADDLE_OCR_ERROR_INIT;
    }

    delete handle;
    return PADDLE_OCR_SUCCESS;
}

PaddleOcrError paddle_ocr_process_image(PaddleOcrHandle* handle,
                                       const uint8_t* image_data,
                                       size_t data_size,
                                       int width,
                                       int height,
                                       int channels,
                                       PaddleOcrResult** results,
                                       size_t* result_count) {
    if (!handle || !image_data || !results || !result_count) {
        return PADDLE_OCR_ERROR_PROCESS;
    }

    try {
        cv::Mat img = create_mat_from_data(image_data, data_size, width, height, channels);
        if (img.empty()) {
            return PADDLE_OCR_ERROR_PROCESS;
        }

        // Process image
        auto cpp_results = handle->ocr->ocr(img,
                                          handle->config.det,
                                          handle->config.rec,
                                          handle->config.cls);

        *results = convert_results(cpp_results, result_count);
        if (!*results && *result_count > 0) {
            return PADDLE_OCR_ERROR_MEMORY;
        }

        return PADDLE_OCR_SUCCESS;
    } catch (const std::exception& e) {
        return PADDLE_OCR_ERROR_PROCESS;
    }
}

PaddleOcrError paddle_ocr_process_image_file(PaddleOcrHandle* handle,
                                            const char* image_path,
                                            PaddleOcrResult** results,
                                            size_t* result_count) {
    if (!handle || !image_path || !results || !result_count) {
        return PADDLE_OCR_ERROR_PROCESS;
    }

    try {
        cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
        if (img.empty()) {
            return PADDLE_OCR_ERROR_FILE;
        }

        // Process image
        auto cpp_results = handle->ocr->ocr(img,
                                          handle->config.det,
                                          handle->config.rec,
                                          handle->config.cls);

        *results = convert_results(cpp_results, result_count);
        if (!*results && *result_count > 0) {
            return PADDLE_OCR_ERROR_MEMORY;
        }

        return PADDLE_OCR_SUCCESS;
    } catch (const std::exception& e) {
        return PADDLE_OCR_ERROR_PROCESS;
    }
}

void paddle_ocr_free_results(PaddleOcrResult* results, size_t count) {
    if (!results) return;

    for (size_t i = 0; i < count; ++i) {
        if (results[i].text) {
            free(results[i].text);
        }
    }
    free(results);
}

const char* paddle_ocr_get_error_message(PaddleOcrError error) {
    switch (error) {
        case PADDLE_OCR_SUCCESS:
            return "Success";
        case PADDLE_OCR_ERROR_INIT:
            return "Initialization error";
        case PADDLE_OCR_ERROR_CONFIG:
            return "Configuration error";
        case PADDLE_OCR_ERROR_PROCESS:
            return "Processing error";
        case PADDLE_OCR_ERROR_MEMORY:
            return "Memory allocation error";
        case PADDLE_OCR_ERROR_FILE:
            return "File error";
        default:
            return "Unknown error";
    }
}

#ifdef __cplusplus
}
#endif
