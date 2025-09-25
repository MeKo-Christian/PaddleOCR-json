# PaddleOCR-json 🏆

[![GitHub release](https://img.shields.io/github/release/hiroi-sora/PaddleOCR-json.svg)](https://github.com/hiroi-sora/PaddleOCR-json/releases/latest)
[![GitHub license](https://img.shields.io/github/license/hiroi-sora/PaddleOCR-json.svg)](https://github.com/hiroi-sora/PaddleOCR-json/blob/main/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/hiroi-sora/PaddleOCR-json.svg)](https://github.com/hiroi-sora/PaddleOCR-json/stargazers)

> **Offline OCR Component Series Projects:**
> - **PaddleOCR-json** (You are here)
> - [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json)

A high-performance, offline OCR text recognition program based on PaddleOCR, designed for easy integration into applications across multiple programming languages.

## ✨ Features

- 🚀 **High Performance**: C++ engine with MKL-DNN acceleration
- 🎯 **Accurate**: Supports PP-OCR v3/v4 models with excellent recognition for various fonts
- 🔧 **Easy Integration**: Simple API calls in Python, Node.js, PowerShell, and more
- 📦 **Zero Dependencies**: Unzip and use, no complex environment setup
- 🌐 **Multi-language**: Chinese, English, Japanese, Korean, and more
- 🔄 **Flexible Input**: Local files, Base64 images, clipboard, TCP calls
- 🐳 **Docker Support**: Containerized deployment options

## 📊 Performance Comparison

| Feature | PaddleOCR-json | RapidOCR-json |
|---------|----------------|----------------|
| **CPU Requirements** | AVX instruction set required | No special requirements ✅ |
| **Inference Acceleration** | MKL-DNN ✅ | None |
| **Recognition Speed** | Fast ✅ | Medium |
| **Initialization Time** | ~0.6s | <0.1s ✅ |
| **Package Size** | 100MB | 70MB ✅ |
| **Deployed Size** | 369MB | 80MB ✅ |
| **CPU Usage** | High | Low ✅ |
| **Memory Usage** | 2000MB | 800MB ✅ |

## 🖥️ Compatibility

### System Requirements

- **OS**: Windows 7+ x64, Linux x64
- **Architecture**: x86-64
- **CPU**: Must support AVX instruction set

### CPU Support Matrix

| AVX Support | ✅ Supported | ❌ Not Supported |
|-------------|-------------|------------------|
| **Intel** | Core, Xeon | Atom, Itanium, Celeron, Pentium |
| **AMD** | Ryzen, Athlon, FX (Bulldozer+) | K10 and earlier |

> **Note**: For CPUs without AVX support, try [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json)

### Dependencies

- Windows 7: Install [VC Runtime Library](https://aka.ms/vs/17/release/vc_redist.x64.exe) if missing `VCOMP140.DLL`

## 🚀 Quick Start

### 1. Download

Get the latest release from [GitHub Releases](https://github.com/hiroi-sora/PaddleOCR-json/releases/latest)

### 2. Simple Test

```bash
# Windows
PaddleOCR-json.exe -image_path="test.jpg"

# Linux
./PaddleOCR-json -image_path="test.jpg"
```

### 3. API Usage

#### Linux Quick Start

This project uses `just` as a command runner. To build and run the project on Linux, follow these steps:

1.  **Install `just` and dependencies:**

    ```bash
    just setup
    ```

2.  **Build the project:**

    ```bash
    just build
    ```

3.  **Run the executable:**

    ```bash
    just run --image_path="test.jpg"
    ```


#### Python API

```python
from PPOCR_api import GetOcrApi

# Initialize OCR engine
ocr = GetOcrApi("./PaddleOCR-json.exe")

# Recognize image
result = ocr.run("path/to/image.png")
print(f"Status: {result['code']}")
print(f"Text: {result['data']}")
```

#### Node.js API

```javascript
const OCR = require('paddleocrjson');

const ocr = new OCR('PaddleOCR-json.exe');

ocr.flush({ image_path: 'path/to/image.png' })
  .then(data => console.log(data))
  .finally(() => ocr.terminate());
```

## 📚 API Reference

### Supported Languages

- Python ✅
- Node.js ✅
- PowerShell ✅
- Java ✅
- .NET ✅
- Rust ✅
- Go ✅

> **Want to add support for another language?** See our [Detailed Usage Guide](docs/Detailed-Usage-Guide.md)

### Input Methods

- 📁 **Local Files**: Image paths
- 📋 **Clipboard**: System clipboard images
- 🔤 **Base64**: Encoded image data
- 🌐 **TCP**: Network calls

## ⚙️ Configuration

### Common Parameters

| Parameter | Default | Description |
|-----------|---------|-------------|
| `ensure_ascii` | `true` | ASCII encoding for Unicode characters |
| `config_path` | `""` | Language configuration file path |
| `models_path` | `""` | Models directory path |
| `det` | `true` | Enable text detection |
| `cls` | `false` | Enable text direction classification |
| `use_angle_cls` | `false` | Use angle classification (requires `cls`) |
| `enable_mkldnn` | `true` | CPU inference acceleration |
| `limit_side_len` | `960` | Image size limit (multiple of 32/48) |

### Language Configuration

Switch languages by specifying config files:

```python
# English recognition
ocr = GetOcrApi("PaddleOCR-json.exe", {
    "config_path": "models/config_en.txt"
})

# Custom models path
ocr = GetOcrApi("PaddleOCR-json.exe", {
    "models_path": "/path/to/models",
    "config_path": "/path/to/models/config_en.txt"
})
```

**Supported Languages:**
- 🇺🇸 English
- 🇨🇳 Chinese (Simplified)
- 🇹🇼 Chinese (Traditional)
- 🇯🇵 Japanese
- 🇰🇷 Korean

## 📋 Return Values

### Success Response (Code: 100)

```json
{
  "code": 100,
  "data": [
    {
      "text": "Recognized text",
      "box": [[x1,y1], [x2,y2], [x3,y3], [x4,y4]],
      "score": 0.9876,
      "cls_label": 0,
      "cls_score": 0.95
    }
  ]
}
```

### Error Codes

| Code | Description |
|------|-------------|
| `100` | ✅ Recognition successful |
| `101` | ℹ️ No text found |
| `200` | ❌ Image path not found |
| `201` | ❌ Path encoding error |
| `202` | ❌ Cannot open image |
| `203` | ❌ Image decode failed |
| `300` | ❌ Base64 decode failed |
| `301` | ❌ Base64 image decode failed |
| `400` | ❌ JSON encoding error |
| `401` | ❌ JSON decoding error |
| `402` | ❌ JSON parsing error |
| `403` | ❌ No valid tasks |

## 🏗️ Building from Source

### Stable Version (PP-OCR v2.6)
- [Windows Build Guide](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README.md)
- [Linux Build Guide](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README-linux.md)
- [Docker Guide](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README-docker.md)

### Development Version (PP-OCR v2.8)
- [Windows Build Guide](cpp/README.md)
- [Linux Build Guide](cpp/README-linux.md)
- [Docker Guide](cpp/README-docker.md)
- [Porting Guide](cpp/docs/Porting-Guide.md)

## 📖 Documentation

- [Detailed Usage Guide](docs/Detailed-Usage-Guide.md)
- [Migration Guide](docs/Migration-Guide.md)
- [Build Guide](docs/Build-Guide.md)
- [Update v1.3 Guide](docs/update-v1.3.md)

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Adding Language Support
1. Follow the [Detailed Usage Guide](docs/Detailed-Usage-Guide.md)
2. Test thoroughly
3. Submit a pull request

## 🙏 Acknowledgments

- [PaddlePaddle/PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR) - Core OCR engine
- [ReneNyffenegger/cpp-base64](https://github.com/ReneNyffenegger/cpp-base64) - Base64 encoding/decoding
- [nlohmann/json](https://github.com/nlohmann/json) - JSON for Modern C++
- [Umi-OCR](https://github.com/hiroi-sora/Umi-OCR) - Text block post-processing

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆕 Changelog

### [v1.4.1](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.4.1) - 2024-08-28
- 🔄 Reverted to Paddle Inference 2.3.2 for stability
- 🐛 Fixed Traditional Chinese config file issue
- 🐧 Updated Linux build for better compatibility (glibc 2.31+)

### [v1.4.0](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.4.0) - 2024-07-22
- ➕ Added text direction classification support
- 🐧 Linux compatibility improvements
- 📋 Clipboard functionality (optional)

### [v1.3.1](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.3.1) - 2023-10-10
- 🪟 Windows 7 x64 support

### [v1.3.0](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.3.0) - 2023-06-19
- 🆕 Base64 image input support
- 🌐 Socket server mode
- 🐛 Various bug fixes

---

**Made with ❤️ for the OCR community**  
*Star this repo if you find it useful!*
