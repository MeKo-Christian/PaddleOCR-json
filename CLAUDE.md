# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

PaddleOCR-json is an offline OCR (Optical Character Recognition) component based on PaddleOCR that provides fast text recognition through a JSON API. It can be used as a subprocess by parent programs or as a separate process over TCP.

## Common Development Commands

### Building the Project

**Linux Build:**
```bash
# Prerequisites (from cpp/.source directory)
export PADDLE_LIB="$(pwd)/$(ls -d *paddle_inference*/ | head -n1)"
export MODELS="$(pwd)/models"
export OPENCV_DIR="$(pwd)/opencv-release"  # If using custom OpenCV

# Build
mkdir build
cmake -S . -B build/ \
    -DPADDLE_LIB=$PADDLE_LIB \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENCV_DIR=$OPENCV_DIR
cmake --build build/

# One-click build script
./tools/linux_build.sh

# One-click run script
./tools/linux_run.sh [config_parameters]
```

**Windows Build:**
- Refer to `cpp/README.md` for detailed Windows build instructions
- Uses CMake with Visual Studio build tools

### Running and Testing

```bash
# Basic usage
./build/bin/PaddleOCR-json -image_path="test.jpg"

# With models path
LD_LIBRARY_PATH="$(pwd)/build/bin/" ./build/bin/PaddleOCR-json \
    -models_path="$MODELS" \
    -config_path="$MODELS/config_chinese.txt" \
    -image_path="/path/to/image.jpg"

# Install to system
sudo cmake --install build
```

### Testing APIs

**Python API:**
```bash
cd api/python
python -c "from PPOCR_api import GetOcrApi; ocr = GetOcrApi('path/to/PaddleOCR-json.exe'); print(ocr.run('test.jpg'))"
```

**Node.js API:**
```bash
cd api/node.js
npm install
node test/app.js
```

## Code Architecture

### Core C++ Engine (`cpp/`)

**Key Components:**
- `src/main.cpp` - Entry point and command line processing
- `include/paddleocr.h` & `src/ocr_*.cpp` - Core OCR functionality (detection, classification, recognition)
- `include/task.h` & `src/task_linux.cpp` - Task processing and JSON communication
- `include/args.h` & `src/args.cpp` - Command line argument parsing
- `src/base64.cpp` - Base64 encoding/decoding for image data

**Architecture Flow:**
1. **Initialization**: Load PaddleOCR models and configure parameters
2. **Input Processing**: Accept images via file path, base64, or clipboard
3. **OCR Pipeline**: Text detection → Classification (optional) → Recognition  
4. **Output**: Return structured JSON with bounding boxes, text, and confidence scores

**Error Codes:** Defined in `task.h` with comprehensive status codes (100=success, 101=no text, 200+=errors)

### Language APIs (`api/`)

**Python API (`api/python/`):**
- `PPOCR_api.py` - Main API with pipe and socket communication modes
- Classes: `PPOCR_pipe`, `PPOCR_socket` for different IPC methods
- Supports local subprocess spawning and remote TCP connections

**Node.js API (`api/node.js/`):**
- TypeScript source in `ts/`, compiled to `esnext/` and `es5/`
- Package available as `paddleocrjson` on npm
- Supports same communication patterns as Python API

### Communication Modes

1. **Single Recognition**: Command line with `-image_path`
2. **Pipe Mode**: stdin/stdout JSON communication with subprocess
3. **Socket Mode**: TCP server for network-based OCR requests

### Configuration System

- **Model Switching**: Use `config_path` to specify language models (Chinese, English, Japanese, Korean, etc.)
- **Models Directory**: Configurable via `models_path` parameter  
- **Performance Tuning**: Parameters like `limit_side_len`, `enable_mkldnn`, `det`, `cls` for optimization

### Build System Details

- **CMake-based** with cross-platform support (Windows/Linux)
- **Key Dependencies**: PaddleOCR Inference Library, OpenCV, MKL/OpenBLAS
- **Static/Dynamic Linking**: Configurable via `WITH_STATIC_LIB`
- **Optional Features**: Clipboard support (`ENABLE_CLIPBOARD`), remote shutdown (`ENABLE_REMOTE_EXIT`)

## Development Guidelines

### Working with APIs
- Always test both pipe and socket modes when modifying communication
- Python API supports both local subprocess and remote server connections
- Error handling follows standardized codes defined in `cpp/include/task.h`

### Model and Configuration Changes
- Language models are in `models/` directory with config files like `config_chinese.txt`
- Test with different model configurations when making core changes
- Memory usage can be tuned via `cpu_mem` parameter (Linux)

### Platform Considerations
- Linux builds require AVX instruction set support
- Windows clipboard functionality only available on Windows builds
- Socket mode works cross-platform, pipe mode is platform-specific

### Testing
- Use provided test images and scripts in API directories
- Verify JSON output format consistency across communication modes
- Test memory usage with large images and long-running processes