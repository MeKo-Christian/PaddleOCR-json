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

#pragma once

#include <include/ocr_cls.h>
#include <include/ocr_det.h>
#include <include/ocr_rec.h>

namespace PaddleOCR
{
    class PPOCR
    {
    public:
        explicit PPOCR();
        ~PPOCR() = default; // Default destructor

        // OCR method, process image list, return OCR result vector for each image
        std::vector<std::vector<OCRPredictResult>> ocr(std::vector<cv::Mat> img_list,
                                                       bool det = true,
                                                       bool rec = true,
                                                       bool cls = true);
        // OCR method, process single image, return OCR result
        std::vector<OCRPredictResult> ocr(cv::Mat img, bool det = true,
                                          bool rec = true, bool cls = true);

        void reset_timer();              // Reset timer
        void benchmark_log(int img_num); // Log benchmark, parameter is image count

        // Smart pointers
        std::unique_ptr<DBDetector> detector_;       // Point to text detector instance
        std::unique_ptr<Classifier> classifier_;     // Point to direction classifier instance
        std::unique_ptr<CRNNRecognizer> recognizer_; // Point to text recognizer instance

    protected:
        // Time information
        std::vector<double> time_info_det = {0, 0, 0};
        std::vector<double> time_info_rec = {0, 0, 0};
        std::vector<double> time_info_cls = {0, 0, 0};

        // Text detection: input single image, store single line text fragment detection info in ocr_results vector
        void det(cv::Mat img,
                 std::vector<OCRPredictResult> &ocr_results);
        // Direction classification: input single line fragment vector, store direction flag for each fragment in ocr_results vector
        void cls(std::vector<cv::Mat> img_list,
                 std::vector<OCRPredictResult> &ocr_results);
        // Text recognition: input single line fragment vector, store text for each fragment in ocr_results vector
        void rec(std::vector<cv::Mat> img_list,
                 std::vector<OCRPredictResult> &ocr_results);
    };
} // namespace PaddleOCR
