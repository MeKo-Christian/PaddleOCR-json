#pragma once

// Stub AutoLogger to avoid fetching external dependency when offline.
// When USE_AUTOLOG=ON in CMake, the real implementation from AutoLog is used.

#include <string>
#include <vector>

class AutoLogger {
public:
    AutoLogger(const std::string& /*module_name*/, bool /*use_gpu*/, bool /*use_trt*/, bool /*use_mkldnn*/,
               int /*cpu_threads*/, int /*batch_size*/, const std::string& /*precision_mode*/,
               const std::string& /*precision*/, const std::vector<double>& /*time_info*/, int /*img_num*/) {}

    void report() const {}
};

