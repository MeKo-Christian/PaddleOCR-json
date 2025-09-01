// Copyright (c) 2020 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>
#include <fstream>
#include <include/utility.h>

#include <gflags/gflags.h>

// Operating mode
DEFINE_string(image_path, "", "Set image_path to run a single task.");                                                          // If an image path is provided, perform a single OCR task.
DEFINE_int32(port, -1, "Set to 0 enable random port, set to 1~65535 enables specified port.");                                  // Set to 0 for random port, 1~65535 for specified port. Default enables anonymous pipe mode.
DEFINE_string(addr, "loopback", "Socket server addr, the value can be 'loopback', 'localhost', 'any', or other IPv4 address."); // Socket server address mode, loopback or any available.

// common args
DEFINE_bool(use_gpu, false, "Infering with GPU or CPU.");                                              // Enable GPU if true (requires inference library support)
DEFINE_bool(use_tensorrt, false, "Whether use tensorrt.");                                             // Enable tensorrt if true
DEFINE_int32(gpu_id, 0, "Device id of GPU to execute.");                                               // GPU id, valid when using GPU
DEFINE_int32(gpu_mem, 4000, "GPU memory when infering with GPU.");                                     // Requested GPU memory
DEFINE_int32(cpu_threads, 10, "Num of threads with CPU.");                                             // CPU threads
DEFINE_int32(cpu_mem, 2000, "CPU memory limit in MB. Cleanup if exceeded. -1 means no limit.");        // CPU memory usage limit in MB. -1 means no limit
DEFINE_bool(enable_mkldnn, true, "Whether use mkldnn with CPU.");                                      // Enable mkldnn if true
DEFINE_string(precision, "fp32", "Precision be one of fp32/fp16/int8");                                // Prediction precision, supports fp32, fp16, int8
DEFINE_bool(benchmark, false, "Whether use benchmark.");                                               // Enable benchmark if true, statistics on prediction speed, memory usage, etc.
DEFINE_string(output, "./output/", "Save benchmark log path.");                                        // Path to save visualization results TODO
DEFINE_string(type, "ocr", "Perform ocr or structure, the value is selected in ['ocr','structure']."); // Task type (not available yet)
DEFINE_string(config_path, "", "Path of config file.");                                                // Config file path
DEFINE_string(models_path, "", "Path of models folder.");                                              // Inference library path
DEFINE_bool(ensure_ascii, true, "Enable JSON ascii escape.");                                          // Enable JSON ascii escape if true

// detection related DET
DEFINE_string(det_model_dir, "models/ch_PP-OCRv4_det_infer", "Path of det inference model.");                     // Detection model library path
DEFINE_string(limit_type, "max", "limit_type of input image, the value is selected in ['max','min'].");           // Limit image size by long side or short side
DEFINE_int32(limit_side_len, 960, "limit_side_len of input image.");                                              // Limit value for long/short side
DEFINE_double(det_db_thresh, 0.3, "Threshold of det_db_thresh.");                                                 // Threshold for filtering DB prediction binarized image, 0.-0.3 has little impact on results
DEFINE_double(det_db_box_thresh, 0.6, "Threshold of det_db_box_thresh.");                                         // DB post-processing box filtering threshold, reduce if missing boxes
DEFINE_double(det_db_unclip_ratio, 1.5, "Threshold of det_db_unclip_ratio.");                                     // Text box tightness, smaller value makes box closer to text
DEFINE_bool(use_dilation, false, "Whether use the dilation on output map.");                                      // Dilate segmentation results for better detection if true
DEFINE_string(det_db_score_mode, "slow", "Whether use polygon score, the value is selected in ['slow','fast']."); // slow: use polygon to calculate bbox score, fast: use rectangle. Rectangle is faster, polygon more accurate for curved text
DEFINE_bool(visualize, false, "Whether show the detection results.");                                             // Visualize results if true, saved in output folder with same name as input image.

// classification related CLS
DEFINE_bool(use_angle_cls, false, "Whether use use_angle_cls."); // Enable direction classifier if true
DEFINE_string(cls_model_dir, "models/ch_ppocr_mobile_v2.0_cls_infer", "Path of cls inference model.");
DEFINE_double(cls_thresh, 0.9, "Threshold of cls_thresh."); // Direction classifier score threshold
DEFINE_int32(cls_batch_num, 1, "cls_batch_num.");           // Direction classifier batch size

// recognition related REC
DEFINE_string(rec_model_dir, "models/ch_PP-OCRv4_rec_infer", "Path of rec inference model.");
DEFINE_int32(rec_batch_num, 6, "rec_batch_num.");                                    // Text recognition model batch size
DEFINE_string(rec_char_dict_path, "models/dict_chinese.txt", "Path of dictionary."); // Dictionary path
DEFINE_int32(rec_img_h, 48, "rec image height");                                     // Text recognition model input image height. V3 is 48, V2 should be 32
DEFINE_int32(rec_img_w, 320, "rec image width");                                     // Text recognition model input image width. Same for V3 and V2

// layout model related
DEFINE_string(layout_model_dir, "", "Path of table layout inference model.");
DEFINE_string(layout_dict_path, "../../ppocr/utils/dict/layout_dict/layout_publaynet_dict.txt", "Path of dictionary."); // Layout dictionary path
DEFINE_double(layout_score_threshold, 0.5, "Threshold of score.");                                                      // Detection box score threshold
DEFINE_double(layout_nms_threshold, 0.5, "Threshold of nms.");                                                          // NMS threshold
// structure model related
DEFINE_string(table_model_dir, "", "Path of table struture inference model.");
DEFINE_int32(table_max_len, 488, "max len size of input image."); // Table recognition model input image long side size
DEFINE_int32(table_batch_num, 1, "table_batch_num.");
DEFINE_bool(merge_no_span_structure, true, "Whether merge <td> and </td> to <td></td>");                          // Merge <td> and </td> to <td></td> if true
DEFINE_string(table_char_dict_path, "../../ppocr/utils/dict/table_structure_dict_ch.txt", "Path of dictionary."); // Table dictionary path

// ocr forward related
DEFINE_bool(det, true, "Whether use det in forward.");
DEFINE_bool(rec, true, "Whether use rec in forward.");
DEFINE_bool(cls, false, "Whether use cls in forward.");
DEFINE_bool(table, false, "Whether use table structure in forward.");
DEFINE_bool(layout, false, "Whether use layout analysis in forward.");

// Check if a path exists, write info to msg
void check_path(const std::string &path, const std::string &name, std::string &msg)
{
    if (path.empty())
    {
        msg += (name + " is empty. ");
    }
    else if (!PaddleOCR::Utility::PathExists(path))
    {
        msg += (name + " [" + path + "] does not exist. ");
    }
}

// Prepend models path to value
void prepend_models(const std::string &models_path_base, std::string &value)
{
    if (PaddleOCR::Utility::str_starts_with(value, "models"))
    {
        value.erase(0, 6);
        value = PaddleOCR::Utility::pathjoin(models_path_base, value);
    }
}

// Read config from file, return log string.
std::string read_config()
{
    // Set default models path
    std::string models_path_base = "models";
    // If valid models path parameter is provided
    if (!FLAGS_models_path.empty() && PaddleOCR::Utility::PathExists(FLAGS_models_path))
    {
        // Update models path
        models_path_base = FLAGS_models_path;
        // Then use this models path to update all other parameter paths
    }

    if (!PaddleOCR::Utility::PathExists(FLAGS_config_path))
    {
        return ("config_path [" + FLAGS_config_path + "] does not exist. ");
    }
    std::ifstream infile(FLAGS_config_path);
    if (!infile)
    {
        return ("[WARNING] Unable to open config_path [" + FLAGS_config_path + "]. ");
    }
    std::string msg = "Load config from [" + FLAGS_config_path + "] : ";
    std::string line;
    int num = 0;
    while (getline(infile, line))
    {
        int length = line.length();
        if (length < 3 || line[0] == '#') // Skip empty lines and comments
            continue;
        int split = 0; // Key-value pair separator
        for (; split < length; split++)
        {
            if (line[split] == ' ' || line[split] == '=')
                break;
        }
        if (split >= length - 1 || split == 0) // Skip invalid key-value pairs
            continue;
        std::string key = line.substr(0, split);
        std::string value = line.substr(split + 1);
        prepend_models(models_path_base, value);
        // Set config, lower priority than command line args.
        std::string res = google::SetCommandLineOptionWithMode(key.c_str(), value.c_str(), google::SET_FLAG_IF_DEFAULT);
        if (!res.empty())
        {
            num++;
            msg += res.substr(0, res.length() - 1);
        }
    }
    infile.close();
    if (num == 0)
        msg += "No valid config found.";
    else
        msg += ". ";
    return msg;
}

// Check parameter validity. Return empty string on success, error message on failure.
std::string check_flags()
{
    // Set default models path
    std::string models_path_base = "models";
    // If valid models path parameter is provided
    if (!FLAGS_models_path.empty() && PaddleOCR::Utility::PathExists(FLAGS_models_path))
    {
        // Update models path
        models_path_base = FLAGS_models_path;
        // Then use this models path to update all other parameter paths
    }

    std::string msg = "";
    if (FLAGS_det)
    { // Check det
        prepend_models(models_path_base, FLAGS_det_model_dir);
        check_path(FLAGS_det_model_dir, "det_model_dir", msg);
    }
    if (FLAGS_rec)
    { // Check rec
        prepend_models(models_path_base, FLAGS_rec_model_dir);
        check_path(FLAGS_rec_model_dir, "rec_model_dir", msg);
    }
    if (FLAGS_cls && FLAGS_use_angle_cls)
    { // Check cls
        prepend_models(models_path_base, FLAGS_cls_model_dir);
        check_path(FLAGS_cls_model_dir, "cls_model_dir", msg);
    }
    if (!FLAGS_rec_char_dict_path.empty())
    { // Check rec_char_dict_path
        prepend_models(models_path_base, FLAGS_rec_char_dict_path);
        check_path(FLAGS_rec_char_dict_path, "rec_char_dict_path", msg);
    }
    if (FLAGS_table)
    { // Check table
        prepend_models(models_path_base, FLAGS_table_model_dir);
        check_path(FLAGS_table_model_dir, "table_model_dir", msg);
        if (!FLAGS_det)
            check_path(FLAGS_det_model_dir, "det_model_dir", msg);
        if (!FLAGS_rec)
            check_path(FLAGS_rec_model_dir, "rec_model_dir", msg);
    }
    if (FLAGS_layout)
    { // Layout
        prepend_models(models_path_base, FLAGS_layout_model_dir);
        check_path(FLAGS_layout_model_dir, "layout_model_dir", msg);
    }
    if (!FLAGS_config_path.empty())
    { // Check config path exists if not empty
        check_path(FLAGS_config_path, "config_path", msg);
    }
    // Check enum values
    if (FLAGS_precision != "fp32" && FLAGS_precision != "fp16" && FLAGS_precision != "int8")
    {
        msg += "precison should be 'fp32'(default), 'fp16' or 'int8', not " + FLAGS_precision + ". ";
    }
    if (FLAGS_type != "ocr" && FLAGS_type != "structure")
    {
        msg += "type should be 'ocr'(default) or 'structure', not " + FLAGS_type + ". ";
    }
    if (FLAGS_limit_type != "max" && FLAGS_limit_type != "min")
    {
        msg += "limit_type should be 'max'(default) or 'min', not " + FLAGS_limit_type + ". ";
    }
    if (FLAGS_det_db_score_mode != "slow" && FLAGS_det_db_score_mode != "fast")
    {
        msg += "limit_type should be 'slow'(default) or 'fast', not " + FLAGS_det_db_score_mode + ". ";
    }
    return msg;
}