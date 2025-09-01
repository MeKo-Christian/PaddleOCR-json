// PaddleOCR-json
// https://github.com/hiroi-sora/PaddleOCR-json

#ifndef TASK_H
#define TASK_H

#include "include/nlohmann/json.hpp" // json library

namespace PaddleOCR
{

// ==================== Flag codes ====================
#define CODE_INIT 0 // Initial value per round, still this at end of round means no errors found in controlled area
// Recognition successful
#define CODE_OK 100      // Success, and text recognized
#define CODE_OK_NONE 101 // Success, and no text recognized
#define MSG_OK_NONE(p) "No text found in image. Path: \"" + p + "\""
// Read image by path, failed
#define CODE_ERR_PATH_EXIST 200 // Image path does not exist
#define MSG_ERR_PATH_EXIST(p) "Image path dose not exist. Path: \"" + p + "\""
#define CODE_ERR_PATH_CONV 201 // Image path string cannot convert to wstring
#define MSG_ERR_PATH_CONV(p) "Image path failed to convert to utf-16 wstring. Path: \"" + p + "\""
#define CODE_ERR_PATH_READ 202 // Image path exists, but cannot open file
#define MSG_ERR_PATH_READ(p) "Image open failed. Path: \"" + p + "\""
#define CODE_ERR_PATH_DECODE 203 // Image opened successfully, but content cannot be decoded by opencv
#define MSG_ERR_PATH_DECODE(p) "Image decode failed. Path: \"" + p + "\""

#if defined(_WIN32) && defined(ENABLE_CLIPBOARD)
// Read image from clipboard, failed
#define CODE_ERR_CLIP_OPEN 210 // Clipboard open failed ( OpenClipboard )
#define MSG_ERR_CLIP_OPEN "Clipboard open failed."
#define CODE_ERR_CLIP_EMPTY 211 // Clipboard is empty ( GetPriorityClipboardFormat NULL )
#define MSG_ERR_CLIP_EMPTY "Clipboard is empty."
#define CODE_ERR_CLIP_FORMAT 212 // Clipboard format not supported ( GetPriorityClipboardFormat -1 )
#define MSG_ERR_CLIP_FORMAT "Clipboard format is not valid."
#define CODE_ERR_CLIP_DATA 213 // Getting clipboard data handle failed, usually caused by another program occupying clipboard ( GetClipboardData NULL )
#define MSG_ERR_CLIP_DATA "Getting clipboard data handle failed."
#define CODE_ERR_CLIP_FILES 214 // Number of files queried from clipboard is not 1 ( DragQueryFile != 1 )
#define MSG_ERR_CLIP_FILES(n) "Clipboard number of query files is not valid. Number: " + std::to_string(n)
#define CODE_ERR_CLIP_GETOBJ 215 // Clipboard retrieve graphic object info failed ( GetObject NULL )
#define MSG_ERR_CLIP_GETOBJ "Clipboard get bitmap object failed."
#define CODE_ERR_CLIP_BITMAP 216 // Getting clipboard bitmap data failed ( GetBitmapBits copied bytes empty )
#define MSG_ERR_CLIP_BITMAP "Getting clipboard bitmap bits failed."
#define CODE_ERR_CLIP_CHANNEL 217 // Number of channels in clipboard bitmap not supported ( nChannels not 1,3,4 )
#define MSG_ERR_CLIP_CHANNEL(n) "Clipboard number of image channels is not valid. Number: " + std::to_string(n)
#endif

// Read image from base64, failed
#define CODE_ERR_BASE64_DECODE 300 // Base64 string parse to string failed
#define MSG_ERR_BASE64_DECODE "Base64 decode failed."
#define CODE_ERR_BASE64_IM_DECODE 301 // Base64 string parse successful, but content cannot be decoded by opencv
#define MSG_ERR_BASE64_IM_DECODE "Base64 data imdecode failed."
// Json related
#define CODE_ERR_JSON_DUMP 400 // Json object to string failed
#define MSG_ERR_JSON_DUMP "Json dump failed."
#define CODE_ERR_JSON_PARSE 401 // Json string to object failed
#define MSG_ERR_JSON_PARSE "Json parse failed."
#define CODE_ERR_JSON_PARSE_KEY 402 // Json object parse certain key failed
#define MSG_ERR_JSON_PARSE_KEY(k) "Json parse key [" + k + "] failed."
#define CODE_ERR_NO_TASK 403 // No valid task found
#define MSG_ERR_NO_TASK "No valid tasks."

    // ==================== Task calling class ====================
    class Task
    {

    public:
        int ocr(); // OCR image

    private:
        bool is_exit = false;         // Exit task loop when true
        std::unique_ptr<PPOCR> ppocr; // OCR engine smart pointer
        int t_code;                   // Current round task status code
        std::string t_msg;            // Current round task status message

        // Task flow
        void init_engine();               // Initialize OCR engine
        void memory_check_cleanup();        // Check memory usage, release memory when reaching limit
        std::string run_ocr(std::string); // Input user passed value (string), return result json string
        int single_image_mode();          // Single recognition mode
        int socket_mode();                // Socket mode
        int anonymous_pipe_mode();        // Anonymous pipe mode
        int get_memory_mb();           // Get current memory usage. Return integer in MB. Return -1 on failure.

        // Output related
        void set_state(int code = CODE_INIT, std::string msg = "");             // Set state
        std::string get_state_json(int code = CODE_INIT, std::string msg = ""); // Get state json string
        std::string get_ocr_result_json(const std::vector<OCRPredictResult> &); // Input OCR result, return json string

        // Input related
        std::string json_dump(nlohmann::json);                             // Json object to string
        cv::Mat imread_json(std::string &);                                // Input json string, parse json and return image Mat
        cv::Mat imread_u8(std::string path, int flag = cv::IMREAD_COLOR);  // Replace cv imread, input utf-8 string, return Mat. Set error code on failure and return empty Mat.
        cv::Mat imread_clipboard(int flag = cv::IMREAD_COLOR);             // Read image from current clipboard
        cv::Mat imread_base64(std::string &, int flag = cv::IMREAD_COLOR); // Input base64 encoded string, return Mat
#ifdef _WIN32
        cv::Mat imread_wstr(std::wstring pathW, int flags = cv::IMREAD_COLOR); // Input unicode wstring, return Mat.
#endif

        // Other

        // IPv4 address to uint32_t
        int addr_to_uint32(const std::string &addr, uint32_t &addr_out);
    };

} // namespace PaddleOCR

#endif // TASK_H