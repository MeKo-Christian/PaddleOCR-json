// Example usage of PaddleOCR-json C API
#include "paddleocr_c_api.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Initialize configuration
    PaddleOcrConfig config = {
        .model_path = "/path/to/models",
        .config_path = "/path/to/config.txt",
        .use_gpu = false,
        .gpu_id = 0,
        .gpu_mem = 4000,
        .cpu_threads = 4,
        .enable_mkldnn = true,
        .enable_tensorrt = false,
        .precision = "fp32",
        .det = true,
        .rec = true,
        .cls = true,
        .det_db_thresh = 0.3,
        .det_db_box_thresh = 0.6,
        .det_db_unclip_ratio = 1.5,
        .det_db_score_mode = "slow",
        .use_dilation = false,
        .limit_type = "max",
        .limit_side_len = 960,
        .rec_thresh = 0.5,
        .max_side_len = 2000
    };

    // Create OCR handle
    PaddleOcrHandle* handle = NULL;
    PaddleOcrError error = paddle_ocr_create(&config, &handle);
    if (error != PADDLE_OCR_SUCCESS) {
        fprintf(stderr, "Failed to create OCR handle: %s\n",
                paddle_ocr_get_error_message(error));
        return 1;
    }

    // Process image file
    PaddleOcrResult* results = NULL;
    size_t result_count = 0;
    error = paddle_ocr_process_image_file(handle, "test.jpg", &results, &result_count);
    if (error != PADDLE_OCR_SUCCESS) {
        fprintf(stderr, "Failed to process image: %s\n",
                paddle_ocr_get_error_message(error));
        paddle_ocr_destroy(handle);
        return 1;
    }

    // Print results
    printf("Found %zu text regions:\n", result_count);
    for (size_t i = 0; i < result_count; ++i) {
        printf("Text: %s\n", results[i].text);
        printf("Score: %.3f\n", results[i].score);
        printf("Box: [%d,%d,%d,%d,%d,%d,%d,%d]\n",
               results[i].box[0], results[i].box[1], results[i].box[2], results[i].box[3],
               results[i].box[4], results[i].box[5], results[i].box[6], results[i].box[7]);
        printf("---\n");
    }

    // Clean up
    paddle_ocr_free_results(results, result_count);
    paddle_ocr_destroy(handle);

    return 0;
}
