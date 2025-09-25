// C API Header for PaddleOCR-json
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct PaddleOcrHandle PaddleOcrHandle;
typedef struct PaddleOcrResult PaddleOcrResult;
typedef struct PaddleOcrConfig PaddleOcrConfig;

// Error codes
typedef enum {
    PADDLE_OCR_SUCCESS = 0,
    PADDLE_OCR_ERROR_INIT = -1,
    PADDLE_OCR_ERROR_CONFIG = -2,
    PADDLE_OCR_ERROR_PROCESS = -3,
    PADDLE_OCR_ERROR_MEMORY = -4,
    PADDLE_OCR_ERROR_FILE = -5
} PaddleOcrError;

// Configuration structure
struct PaddleOcrConfig {
    const char* model_path;
    const char* config_path;
    bool use_gpu;
    int gpu_id;
    int gpu_mem;
    int cpu_threads;
    bool enable_mkldnn;
    bool enable_tensorrt;
    const char* precision;
    bool det;
    bool rec;
    bool cls;
    double det_db_thresh;
    double det_db_box_thresh;
    double det_db_unclip_ratio;
    const char* det_db_score_mode;
    bool use_dilation;
    const char* limit_type;
    int limit_side_len;
    float rec_thresh;
    int max_side_len;
};

// OCR Result structure
struct PaddleOcrResult {
    char* text;
    float score;
    int cls_label;
    float cls_score;
    int box[8]; // [x1,y1,x2,y2,x3,y3,x4,y4]
    size_t text_length;
};

// Function declarations
PaddleOcrError paddle_ocr_create(PaddleOcrConfig* config, PaddleOcrHandle** handle);
PaddleOcrError paddle_ocr_destroy(PaddleOcrHandle* handle);

PaddleOcrError paddle_ocr_process_image(PaddleOcrHandle* handle,
                                       const uint8_t* image_data,
                                       size_t data_size,
                                       int width,
                                       int height,
                                       int channels,
                                       PaddleOcrResult** results,
                                       size_t* result_count);

PaddleOcrError paddle_ocr_process_image_file(PaddleOcrHandle* handle,
                                            const char* image_path,
                                            PaddleOcrResult** results,
                                            size_t* result_count);

void paddle_ocr_free_results(PaddleOcrResult* results, size_t count);

const char* paddle_ocr_get_error_message(PaddleOcrError error);

#ifdef __cplusplus
}
#endif
