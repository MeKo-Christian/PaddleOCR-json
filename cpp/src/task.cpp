
#include <exception>
#include <regex>

#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"
#include "include/base64.h" // base64 library

// htonl function
#if defined(_WIN32)
#include <windows.h>
#else // Linux, Mac
#include <arpa/inet.h>
#endif

namespace PaddleOCR
{
    // ==================== Tools ====================

    // Convert json object to string
    std::string Task::json_dump(nlohmann::json j)
    {
        try
        {
            std::string str = j.dump(-1, ' ', FLAGS_ensure_ascii);
            return str;
        }
        catch (...)
        {
            nlohmann::json j2;
            j2["code"] = CODE_ERR_JSON_DUMP;
            j2["data"] = MSG_ERR_JSON_DUMP;
            std::string str = j2.dump(-1, ' ', FLAGS_ensure_ascii);
            return str;
        }
    }

    // Set state
    void Task::set_state(int code, std::string msg)
    {
        t_code = code;
        t_msg = msg;
    }

    // Get state json string
    std::string Task::get_state_json(int code, std::string msg)
    {
        nlohmann::json j;
        if (code == CODE_INIT && msg.empty())
        { // Leave empty, fill with current state
            code = t_code;
            msg = t_msg;
        }
        j["code"] = code;
        j["data"] = msg;
        return json_dump(j);
    }

    // Convert OCR result to json string
    std::string Task::get_ocr_result_json(const std::vector<OCRPredictResult> &ocr_result)
    {
        nlohmann::json outJ;
        outJ["code"] = 100;
        outJ["data"] = nlohmann::json::array();
        bool isEmpty = true;
        for (int i = 0; i < ocr_result.size(); i++)
        {
            nlohmann::json j;
            j["text"] = ocr_result[i].text;
            j["score"] = ocr_result[i].score;
            std::vector<std::vector<int>> b = ocr_result[i].box;
            // No bounding box
            if (b.empty())
            {
                if (FLAGS_det) // If det is enabled but still no bounding box, skip this group
                    continue;
                else // If det is not enabled, fill with empty bounding box
                    for (int bi = 0; bi < 4; bi++)
                        b.push_back(std::vector<int>{-1, -1});
            }
            // If rec is enabled but still no text, skip this group
            if (FLAGS_rec && (j["score"] <= 0 || j["text"] == ""))
            {
                continue;
            }
            else
            {
                j["box"] = {{b[0][0], b[0][1]}, {b[1][0], b[1][1]}, {b[2][0], b[2][1]}, {b[3][0], b[3][1]}};
            }
            // If cls is enabled, cls_label has actual value, then write direction classification related parameters
            if (ocr_result[i].cls_label != -1)
            {
                j["cls_label"] = ocr_result[i].cls_label; // Direction label, 0 means clockwise 0° or 90°, 1 means 180° or 270°
                j["cls_score"] = ocr_result[i].cls_score; // Direction label confidence, closer to 1 is more reliable
            }

            outJ["data"].push_back(j);
            isEmpty = false;
        }
        // Result 1: Recognition successful, no text (rec not detected)
        if (isEmpty)
        {
            return "";
        }
        // Result 2: Recognition successful, with text
        return json_dump(outJ);
    }

    // Input base64 encoded string, return Mat
    cv::Mat Task::imread_base64(std::string &b64str, int flag)
    {
        std::string decoded_string;
        try
        {
            decoded_string = base64_decode(b64str);
        }
        catch (...)
        {
            set_state(CODE_ERR_BASE64_DECODE, MSG_ERR_BASE64_DECODE); // Report status: parsing failed
            return cv::Mat();
        }
        try
        {
            std::vector<uchar> data(decoded_string.begin(), decoded_string.end());
            cv::Mat img = cv::imdecode(data, flag);
            if (img.empty())
            {
                set_state(CODE_ERR_BASE64_IM_DECODE, MSG_ERR_BASE64_IM_DECODE); // Report status: convert to Mat failed
            }
            return img;
        }
        catch (...)
        {
            set_state(CODE_ERR_BASE64_IM_DECODE, MSG_ERR_BASE64_IM_DECODE); // Report status: convert to Mat failed
            return cv::Mat();
        }
    }

    // Input json string, parse and read Mat
    cv::Mat Task::imread_json(std::string &str_in)
    {
#ifdef ENABLE_REMOTE_EXIT
        if (str_in == "exit")
        { // Exit command
            is_exit = true;
            return cv::Mat();
        }
#endif
        cv::Mat img;
        bool is_image_found = false; // Whether image is found currently
        std::string logstr = "";
        // Parse to json object
        auto j = nlohmann::json();
        try
        {
            j = nlohmann::json::parse(str_in); // Convert to json object
        }
        catch (...)
        {
            set_state(CODE_ERR_JSON_PARSE, MSG_ERR_JSON_PARSE); // Report status: parsing failed
            return cv::Mat();
        }
        for (auto &el : j.items())
        { // Traverse key-value pairs
#ifdef ENABLE_REMOTE_EXIT
            if (el.key() == "exit")
            { // Exit command
                is_exit = true;
                return cv::Mat();
            }
#endif
            try
            {
                std::string value = to_string(el.value());
                int vallen = value.length();
                if (vallen > 2 && value[0] == '\"' && value[vallen - 1] == '\"')
                {
                    value = value.substr(1, vallen - 2); // Remove quotes from both ends of nlohmann string
                }
                // Extract image
                if (!is_image_found)
                {
                    if (el.key() == "image_base64")
                    {                                // base64 string
                        FLAGS_image_path = "base64"; // Set image path flag for output when no text
                        img = imread_base64(value);  // Read image
                        is_image_found = true;
                    }
#ifdef ENABLE_JSON_IMAGE_PATH
                    else if (el.key() == "image_path")
                    { // Image path
                        FLAGS_image_path = value;
                        img = imread_u8(value); // Read image
                        is_image_found = true;
                    }
#endif
                }
                // else {} // TODO: Other parameters hot update
            }
            catch (...)
            {                                                                         // For safety, end this task when unknown exception occurs
                set_state(CODE_ERR_JSON_PARSE_KEY, MSG_ERR_JSON_PARSE_KEY(el.key())); // Report status: parse key failed
                return cv::Mat();
            }
        }
        if (!is_image_found)
        {
            set_state(CODE_ERR_NO_TASK, MSG_ERR_NO_TASK); // Report status: no valid task found
        }
        return img;
    }

    // ==================== Task Flow ====================

    std::string Task::run_ocr(std::string str_in)
    {
        cv::Mat img = imread_json(str_in);
        if (is_exit)
        { // Exit
            return "";
        }
        if (img.empty())
        { // Read image failed
            return get_state_json();
        }
        // Execute OCR
        std::vector<OCRPredictResult> res_ocr = ppocr->ocr(img, FLAGS_det, FLAGS_rec, FLAGS_cls);
        // Get result
        std::string res_json = get_ocr_result_json(res_ocr);
        // Result 1: Recognition successful, no text (rec not detected)
        if (res_json.empty())
        {
            return get_state_json(CODE_OK_NONE, MSG_OK_NONE(FLAGS_image_path));
        }
        // Result 2: Recognition successful, with text
        else
        {
            return res_json;
        }
    }

    void Task::init_engine()
    {
        auto init_start = std::chrono::steady_clock::now();
        this->ppocr.reset(new PPOCR()); // Create engine instance, ownership transferred to smart pointer ppocr
        auto init_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> duration = init_end - init_start;
        std::cerr << "OCR init time: " << duration.count() << "s" << std::endl;
    }

    void Task::memory_check_cleanup()
    {
        /*int mem1 = Task::get_memory_mb();
        auto time1 = std::chrono::steady_clock::now();
        if (this->ppocr->detector_)
        {
            this->ppocr->detector_->predictor_->ClearIntermediateTensor();
            this->ppocr->detector_->predictor_->TryShrinkMemory();
        }
        if (this->ppocr->classifier_)
        {
            this->ppocr->classifier_->predictor_->ClearIntermediateTensor();
            this->ppocr->classifier_->predictor_->TryShrinkMemory();
        }
        if (this->ppocr->recognizer_)
        {
            this->ppocr->recognizer_->predictor_->ClearIntermediateTensor();
            this->ppocr->recognizer_->predictor_->TryShrinkMemory();
        }
        int mem2 = Task::get_memory_mb();
        auto time2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_change = time2 - time1;
        std::cerr << "memory cleanup: " << mem1 << "->" << mem2 << "MB, time: " << time_change.count() << "s" << std::endl;
        return;*/
        auto cleanup_start = std::chrono::steady_clock::now();
        if (FLAGS_cpu_mem <= 0) // No limit
        {
            return;
        }
        int mem = Task::get_memory_mb(); // Current memory usage
        if (mem <= 0)                    // Get failed
        {
            return;
        }
        // Reach limit, perform cleanup
        if (mem >= FLAGS_cpu_mem)
        { 
            // Task::init_engine();
            // Call memory cleanup methods of det cls rec instances
			if (this->ppocr->detector_)
			{
				this->ppocr->detector_->predictor_->ClearIntermediateTensor();
				this->ppocr->detector_->predictor_->TryShrinkMemory();
			}
            if (this->ppocr->classifier_)
            {
                this->ppocr->classifier_->predictor_->ClearIntermediateTensor();
                this->ppocr->classifier_->predictor_->TryShrinkMemory();
            }
            if (this->ppocr->recognizer_)
            {
                this->ppocr->recognizer_->predictor_->ClearIntermediateTensor();
                this->ppocr->recognizer_->predictor_->TryShrinkMemory();
            }
            auto cleanup_end = std::chrono::steady_clock::now();
            std::chrono::duration<double> duration = cleanup_end - cleanup_start;
            int mem2 = Task::get_memory_mb(); // Current memory usage
            std::cerr << "memory cleanup: " << mem << "->" << mem2 << "MB, time: " << duration.count() << "s" << std::endl;
            // Task::init_engine();
        }
        else
        {
            std::cerr << "memory used: " << mem << std::endl;
        }
    }

    // Entry point
    int Task::ocr()
    {
        Task::init_engine(); // Initialize engine
        int flag;

#if defined(_WIN32) && defined(ENABLE_CLIPBOARD)
        std::cout << "OCR clipboard enbaled." << std::endl;
#endif

        // Single image recognition mode
        if (!FLAGS_image_path.empty())
        {
            std::cout << "OCR single image mode. Path: " << FLAGS_image_path << std::endl;
            flag = 1;
        }
        // Socket server mode
        else if (FLAGS_port >= 0 && !FLAGS_addr.empty())
        {
            std::cout << "OCR socket mode. Addr: " << FLAGS_addr << ", Port: " << FLAGS_port << std::endl;
            flag = 2;
        }
        // Anonymous pipe mode
        else
        {
            std::cout << "OCR anonymous pipe mode." << std::endl;
            flag = 3;
        }
        std::cout << "OCR init completed." << std::endl;

        switch (flag)
        {
        case 1:
            return single_image_mode();
        case 2:
            return socket_mode();
        case 3:
            return anonymous_pipe_mode();
        }
        return 0;
    }

    // Single image recognition mode
    int Task::single_image_mode()
    {
        set_state();
        cv::Mat img = imread_u8(FLAGS_image_path);
        if (img.empty())
        { // Read image failed
            std::cout << get_state_json() << std::endl;
            return 0;
        }
        // Execute OCR
        std::vector<OCRPredictResult> res_ocr = ppocr->ocr(img, FLAGS_det, FLAGS_rec, FLAGS_cls);
        // Get result
        std::string res_json = get_ocr_result_json(res_ocr);
        // Result 1: Recognition successful, no text (rec not detected)
        if (res_json.empty())
        {
            std::cout << get_state_json(CODE_OK_NONE, MSG_OK_NONE(FLAGS_image_path)) << std::endl;
        }
        // Result 2: Recognition successful, with text
        else
        {
            std::cout << res_json << std::endl;
        }
        return 0;
    }

    // Anonymous pipe mode
    int Task::anonymous_pipe_mode()
    {
        while (1)
        {
            set_state(); // Initialize state
            // Read one line of input
            std::string str_in;
            getline(std::cin, str_in);
            // Get ocr result and output
            std::string str_out = run_ocr(str_in);
            if (is_exit)
            { // Exit
                return 0;
            }
            // Send back result
            std::cout << str_out << std::endl;
            // Check and cleanup memory
            Task::memory_check_cleanup();
        }
        return 0;
    }

    // Socket server mode, defined in platform

    // Other functions

    // Convert ipv4 address to uint32_t
    int Task::addr_to_uint32(const std::string &addr, uint32_t &addr_out)
    {
        // Handle special cases
        if (addr == "loopback" || addr == "localhost")
        {
            addr_out = htonl(INADDR_LOOPBACK);
            return 0;
        }
        else if (addr == "any")
        {
            addr_out = htonl(INADDR_ANY);
            return 0;
        }

        // Use regex to handle IPv4 address
        std::regex rgx(R"((\d+)\.(\d+)\.(\d+)\.(\d+))");
        std::smatch matches;
        uint32_t output = 0;

        // If verified as IPv4 address, convert to uint32_t host byte order
        if (std::regex_search(addr, matches, rgx))
        {
            uint8_t octet;
            for (size_t i = 1; i < matches.size(); ++i)
            {
                octet = static_cast<uint8_t>(std::stoi(matches[i].str()));
                output |= octet << (8 * (4 - i));
            }
        }
        // Otherwise, report error
        else
        {
            return -1;
        }

        // Finally, convert uint32_t host byte order to network byte order
        addr_out = htonl(output);
        return 0;
    }
}

// ./PaddleOCR-json.exe -config_path="models/zh_CN.txt" -image_path="D:/Test/t2.png" -ensure_ascii=0