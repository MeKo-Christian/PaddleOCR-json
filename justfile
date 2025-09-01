# Justfile for PaddleOCR-json Build Orchestration
# Install just: https://github.com/casey/just
# Usage: just <recipe>

# Default recipe
default: help

# Help
help:
    @echo "PaddleOCR-json Build System"
    @echo ""
    @echo "Available recipes:"
    @echo "  setup          - Install system dependencies"
    @echo "  download       - Download required libraries and models"
    @echo "  build          - Build standard version"
    @echo "  build-static   - Build static binary"
    @echo "  build-musl     - Build musl static binary"
    @echo "  build-cross    - Build for different architectures"
    @echo "  build-all      - Build all variants"
    @echo "  install        - Install to system"
    @echo "  test           - Run basic tests"
    @echo "  clean          - Clean build artifacts"
    @echo "  docker         - Build Docker image"
    @echo "  c-api-test     - Test C API"
    @echo "  help           - Show this help"
    @echo "  install-just   - Install Just (if not installed)"
    @echo "  check-just     - Check Just installation"

# Variables
CPP_DIR := "cpp"
SOURCE_DIR := CPP_DIR + "/.source"
BUILD_DIR := "build"
MODELS_DIR := SOURCE_DIR + "/models"
PADDLE_DIR := SOURCE_DIR + "/paddle_inference_manylinux_cpu_avx_mkl_gcc8.2"
OPENCV_DIR := SOURCE_DIR + "/opencv-release"

# Check if just is installed
check-just:
    @if ! command -v just &> /dev/null; then \
        echo "Just is not installed."; \
        echo "Install with one of:"; \
        echo "  curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to ~/.local/bin"; \
        echo "  cargo install just"; \
        echo "  snap install just"; \
        echo "  brew install just"; \
        exit 1; \
    fi

# Install just (Linux)
install-just:
    @echo "Installing just..."
    curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to ~/.local/bin
    @echo "Add ~/.local/bin to your PATH if not already done"

# Setup system dependencies
setup:
    @echo "Installing system dependencies..."
    sudo apt update
    sudo apt install -y \
        wget tar zip unzip git \
        gcc g++ cmake make \
        libgomp1 pkg-config \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
        musl-tools

# Download required libraries and models
download:
    @echo "Setting up directories..."
    mkdir -p {{SOURCE_DIR}}
    cd {{SOURCE_DIR}}

    @echo "Downloading Paddle Inference library..."
    wget -N https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Linux/CPU/gcc8.2_avx_mkl/paddle_inference.tgz
    tar -xzf paddle_inference.tgz
    mv paddle_inference paddle_inference_manylinux_cpu_avx_mkl_gcc8.2

    @echo "Downloading models..."
    wget -N https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip
    unzip -o models_v1.4.1.zip

    @echo "Downloading OpenCV..."
    wget -N https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.0-beta.2/opencv-release_debian_x86-64.zip
    unzip -o opencv-release_debian_x86-64.zip

# Build standard version
build: download
    @echo "Building standard version..."
    cd {{CPP_DIR}}
    chmod +x tools/linux_build_enhanced.sh
    ./tools/linux_build_enhanced.sh standard

# Build static binary
build-static: download
    @echo "Building static binary..."
    cd {{CPP_DIR}}
    chmod +x tools/linux_build_enhanced.sh
    ./tools/linux_build_enhanced.sh static

# Build musl static binary
build-musl: download
    @echo "Building musl static binary..."
    cd {{CPP_DIR}}
    chmod +x tools/linux_build_musl.sh tools/linux_build_enhanced.sh
    ./tools/linux_build_enhanced.sh musl

# Build for different architectures
build-cross ARCH="aarch64": download
    @echo "Building for {{ARCH}}..."
    cd {{CPP_DIR}}
    chmod +x tools/linux_build_cross.sh tools/linux_build_enhanced.sh
    ./tools/linux_build_enhanced.sh cross {{ARCH}}

# Build all variants
build-all: download
    @echo "Building all variants..."
    cd {{CPP_DIR}}
    chmod +x tools/linux_build_*.sh
    ./tools/linux_build_enhanced.sh standard
    ./tools/linux_build_enhanced.sh static
    ./tools/linux_build_enhanced.sh musl
    ./tools/linux_build_enhanced.sh cross aarch64

# Install to system
install: build
    @echo "Installing to system..."
    cd {{CPP_DIR}}
    sudo ./tools/linux_build_enhanced.sh standard Release install

# Run basic tests
test: build
    @echo "Running basic tests..."
    cd {{CPP_DIR}}

    # Test if binary exists and runs
    if [ -f "build-standard/bin/PaddleOCR-json" ]; then \
        echo "Testing binary execution..."; \
        ./build-standard/bin/PaddleOCR-json --help | head -10; \
    else \
        echo "Binary not found!"; \
        exit 1; \
    fi

    # Test C API library
    if [ -f "build-standard/lib/libpaddleocr_c.so" ]; then \
        echo "C API library found"; \
    else \
        echo "C API library not found!"; \
    fi

# Clean build artifacts
clean:
    @echo "Cleaning build artifacts..."
    rm -rf {{CPP_DIR}}/build-*
    rm -rf {{BUILD_DIR}}

# Build Docker image
docker:
    @echo "Building Docker image..."
    docker build -t paddleocr-json \
        --build-arg BUILD_MODE=standard \
        -f Dockerfile.just .

# Build Docker image with specific mode
docker-build MODE="standard":
    @echo "Building Docker image ({{MODE}} mode)..."
    docker build -t paddleocr-json-{{MODE}} \
        --build-arg BUILD_MODE={{MODE}} \
        -f Dockerfile.just .

# Run Docker container
docker-run: docker
    @echo "Running Docker container..."
    docker run --rm -it paddleocr-json

# Run with volume mount for testing
docker-test: docker
    @echo "Running Docker container with volume mount..."
    docker run --rm -it \
        -v $(pwd)/test.jpg:/app/test.jpg \
        paddleocr-json \
        -image_path=/app/test.jpg \
        -models_path=/app/models \
        -config_path=/app/models/config_chinese.txt

# Development setup
dev-setup: setup download
    @echo "Development environment ready!"

# Quick build and test
quick: build test

# Show build status
status:
    @echo "Build Status:"
    @echo "=============="
    @echo "Source directory: {{SOURCE_DIR}}"
    @echo "  Paddle: $([ -d '{{PADDLE_DIR}}' ] && echo '✓' || echo '✗')"
    @echo "  Models: $([ -d '{{MODELS_DIR}}' ] && echo '✓' || echo '✗')"
    @echo "  OpenCV: $([ -d '{{OPENCV_DIR}}' ] && echo '✓' || echo '✗')"
    @echo ""
    @echo "Build variants:"
    @echo "  Standard: $([ -f '{{CPP_DIR}}/build-standard/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  Static: $([ -f '{{CPP_DIR}}/build-static/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  Musl: $([ -f '{{CPP_DIR}}/build-musl/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  ARM64: $([ -f '{{CPP_DIR}}/build-aarch64/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"

# Package for distribution
package: build-all
    @echo "Creating distribution packages..."
    mkdir -p dist

    # Standard package
    mkdir -p dist/standard
    cp -r {{CPP_DIR}}/build-standard/bin dist/standard/
    cp -r {{CPP_DIR}}/build-standard/lib dist/standard/
    cp {{CPP_DIR}}/tools/linux_dist_tools/* dist/standard/
    cd dist && tar -czf paddleocr-json-standard.tar.gz standard/

    # Static package
    mkdir -p dist/static
    cp {{CPP_DIR}}/build-static/bin/PaddleOCR-json dist/static/
    cp {{CPP_DIR}}/tools/linux_dist_tools/run.sh dist/static/
    cd dist && tar -czf paddleocr-json-static.tar.gz static/

    # Musl package
    mkdir -p dist/musl
    cp {{CPP_DIR}}/build-musl/bin/PaddleOCR-json dist/musl/
    cp {{CPP_DIR}}/tools/linux_dist_tools/run.sh dist/musl/
    cd dist && tar -czf paddleocr-json-musl.tar.gz musl/

    @echo "Packages created in dist/"

# CI/CD build
ci: setup download build-all test package

# Benchmark different build types
benchmark: build-all
    @echo "Benchmarking build types..."
    cd {{CPP_DIR}}

    @echo "Binary sizes:"
    @ls -lh build-*/bin/PaddleOCR-json 2>/dev/null || echo "No binaries found"

    @echo ""
    @echo "Static linking check:"
    @echo "Standard: $(ldd build-standard/bin/PaddleOCR-json 2>/dev/null | wc -l) dependencies"
    @echo "Static: $(ldd build-static/bin/PaddleOCR-json 2>/dev/null | wc -l || echo '0 (static)') dependencies"
    @echo "Musl: $(ldd build-musl/bin/PaddleOCR-json 2>/dev/null | wc -l || echo '0 (static)') dependencies"
