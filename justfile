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
TOOLCHAINS_DIR := SOURCE_DIR + "/toolchains"
CROSS_MUSL_DIR := TOOLCHAINS_DIR + "/x86_64-linux-musl-cross"

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
        build-essential wget tar zip unzip git \
        gcc g++ cmake make ninja-build \
        libgomp1 pkg-config \
        gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
        gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
        musl-tools

# Alias that only installs build deps (no downloads)
setup-deps: setup

# Install musl static build prerequisites and fetch a musl C++ cross toolchain
setup-musl-deps:
    @echo "Installing musl prerequisites..."
    sudo apt update
    sudo apt install -y musl-tools curl ca-certificates xz-utils
    @echo "Preparing musl cross toolchain cache at {{CROSS_MUSL_DIR}}..."
    mkdir -p {{TOOLCHAINS_DIR}}
    cd {{TOOLCHAINS_DIR}} && \
    if [ ! -d "x86_64-linux-musl-cross/bin" ]; then \
        echo "Downloading prebuilt toolchain from musl.cc..."; \
        curl -L -o x86_64-linux-musl-cross.tgz https://musl.cc/x86_64-linux-musl-cross.tgz; \
        tar -xzf x86_64-linux-musl-cross.tgz; \
    else \
        echo "Toolchain already present."; \
    fi && \
    echo "To use now: export PATH=$(pwd)/x86_64-linux-musl-cross/bin:\$$PATH" && \
    $(pwd)/x86_64-linux-musl-cross/bin/x86_64-linux-musl-g++ --version | head -1 || true

# Quick check for musl C++ compiler
check-musl:
    @echo "Checking for musl C++ toolchain..."
    @echo -n "x86_64-linux-musl-g++ in PATH: "; command -v x86_64-linux-musl-g++ >/dev/null && echo Found || echo Missing
    @if [ -x "{{CROSS_MUSL_DIR}}/bin/x86_64-linux-musl-g++" ]; then \
        echo "Local cache: Found at {{CROSS_MUSL_DIR}}/bin"; \
        echo "Use now: export PATH={{CROSS_MUSL_DIR}}/bin:\$$PATH"; \
    else \
        echo "Local cache: Not found at {{CROSS_MUSL_DIR}}/bin"; \
    fi

# Print PATH export to activate local musl toolchain in current shell
print-musl-env:
    @echo "export PATH={{CROSS_MUSL_DIR}}/bin:\$$PATH"

# Start a subshell with musl toolchain on PATH
use-musl-shell:
    @echo "Starting shell with musl toolchain in PATH..."
    env PATH={{CROSS_MUSL_DIR}}/bin:$$PATH bash -l

# Run the compiled executable
run *args:
    @echo "Running PaddleOCR-json..."
    export LD_LIBRARY_PATH={{PADDLE_DIR}}/third_party/install/onednn/lib:$LD_LIBRARY_PATH && \
    {{BUILD_DIR}}/standard/bin/PaddleOCR-json {{args}}

# Download required libraries and models
download:
    @echo "Preparing dependency cache in {{SOURCE_DIR}}..."
    mkdir -p {{SOURCE_DIR}} && cd {{SOURCE_DIR}} && \
    echo "-- Paddle Inference" && \
    if [ ! -d "{{PADDLE_DIR}}" ]; then \
        echo "   not found; fetching archive..."; \
        if [ ! -f "paddle_inference.tgz" ]; then \
            wget -N https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Linux/CPU/gcc8.2_avx_mkl/paddle_inference.tgz; \
        else \
            echo "   using existing paddle_inference.tgz"; \
        fi; \
        tar -xzf paddle_inference.tgz && \
        rm -rf paddle_inference_manylinux_cpu_avx_mkl_gcc8.2 && \
        mv -f paddle_inference paddle_inference_manylinux_cpu_avx_mkl_gcc8.2; \
    else \
        echo "   already present"; \
    fi && \
    echo "-- Models" && \
    if [ ! -d "{{MODELS_DIR}}" ]; then \
        echo "   not found; fetching archive..."; \
        if [ ! -f "models_v1.4.1.zip" ]; then \
            wget -N https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip; \
        else \
            echo "   using existing models_v1.4.1.zip"; \
        fi; \
        unzip -o models_v1.4.1.zip; \
    else \
        echo "   already present"; \
    fi && \
    echo "-- OpenCV" && \
    if [ ! -d "{{OPENCV_DIR}}" ]; then \
        echo "   not found; fetching archive..."; \
        if [ ! -f "opencv-release_debian_x86-64.zip" ]; then \
            wget -N https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.0-beta.2/opencv-release_debian_x86-64.zip; \
        else \
            echo "   using existing opencv-release_debian_x86-64.zip"; \
        fi; \
        unzip -o opencv-release_debian_x86-64.zip; \
    else \
        echo "   already present"; \
    fi

# Build standard version
build: download
    @echo "Building standard version..."
    chmod +x {{CPP_DIR}}/tools/linux_build_enhanced.sh
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh standard

# Build static binary
build-static: download
    @echo "Building static binary..."
    chmod +x {{CPP_DIR}}/tools/linux_build_enhanced.sh
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh static

# Build musl static binary
build-musl: download
    @echo "Building musl static binary..."
    chmod +x {{CPP_DIR}}/tools/linux_build_musl.sh {{CPP_DIR}}/tools/linux_build_enhanced.sh
    env PATH={{CROSS_MUSL_DIR}}/bin:$PATH ./{{CPP_DIR}}/tools/linux_build_enhanced.sh musl

# Build for different architectures
build-cross ARCH="aarch64": download
    @echo "Building for {{ARCH}}..."
    chmod +x {{CPP_DIR}}/tools/linux_build_cross.sh {{CPP_DIR}}/tools/linux_build_enhanced.sh
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh cross {{ARCH}}

# Build all variants
build-all: download
    @echo "Building all variants..."
    chmod +x {{CPP_DIR}}/tools/linux_build_*.sh
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh standard
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh static
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh musl
    ./{{CPP_DIR}}/tools/linux_build_enhanced.sh cross aarch64

# Install to system
install: build
    @echo "Installing to system..."
    sudo ./{{CPP_DIR}}/tools/linux_build_enhanced.sh standard Release install

# Run basic tests
test: build
    @echo "Running basic tests..."

    # Test if binary exists and runs
    if [ -f "{{BUILD_DIR}}/standard/bin/PaddleOCR-json" ]; then \
        echo "Testing binary execution..."; \
        {{BUILD_DIR}}/standard/bin/PaddleOCR-json --help | head -10; \
    else \
        echo "Binary not found!"; \
        exit 1; \
    fi

    # Test C API library
    if [ -f "{{BUILD_DIR}}/standard/lib/libpaddleocr_c.so" ]; then \
        echo "C API library found"; \
    else \
        echo "C API library not found!"; \
    fi

# Clean build artifacts
clean:
    @echo "Cleaning build artifacts (preserving downloads)..."
    rm -rf {{CPP_DIR}}/build-*
    rm -rf {{BUILD_DIR}}

# Remove downloaded dependencies cache (.source)
clean-downloads:
    @echo "Removing downloaded dependencies cache..."
    rm -rf {{SOURCE_DIR}}

# Move stray downloads at repo root into {{SOURCE_DIR}}
tidy:
    @echo "Tidying repository: moving downloads into {{SOURCE_DIR}}..."
    mkdir -p {{SOURCE_DIR}}
    for item in models opencv-release opencv-release_debian_x86-64 paddle_inference_manylinux_cpu_avx_mkl_gcc8.2 paddle_inference; do \
        if [ -e "$$item" ]; then \
            echo "Moving $$item -> {{SOURCE_DIR}}/"; \
            mv -f "$$item" "{{SOURCE_DIR}}/"; \
        fi; \
    done
    for arc in paddle_inference.tgz models_v1.4.1.zip opencv-release_debian_x86-64.zip; do \
        if [ -f "$$arc" ]; then \
            echo "Moving $$arc -> {{SOURCE_DIR}}/"; \
            mv -f "$$arc" "{{SOURCE_DIR}}/"; \
        fi; \
    done
    @echo "Ensuring paddle dir is normalized..."
    if [ -d "{{SOURCE_DIR}}/paddle_inference" ] && [ ! -d "{{PADDLE_DIR}}" ]; then \
        mv -f "{{SOURCE_DIR}}/paddle_inference" "{{PADDLE_DIR}}"; \
    fi
    @echo "Optionally normalize OpenCV dir name..."
    if [ -d "{{SOURCE_DIR}}/opencv-release_debian_x86-64" ] && [ ! -d "{{OPENCV_DIR}}" ]; then \
        mv -f "{{SOURCE_DIR}}/opencv-release_debian_x86-64" "{{OPENCV_DIR}}"; \
    fi

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
    @echo "  Standard: $([ -f '{{BUILD_DIR}}/standard/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  Static: $([ -f '{{BUILD_DIR}}/static/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  Musl: $([ -f '{{BUILD_DIR}}/musl/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"
    @echo "  ARM64: $([ -f '{{BUILD_DIR}}/aarch64/bin/PaddleOCR-json' ] && echo '✓' || echo '✗')"

# Package for distribution
package: build-all
    @echo "Creating distribution packages..."
    mkdir -p dist

    # Standard package
    mkdir -p dist/standard
    cp -r {{BUILD_DIR}}/standard/bin dist/standard/
    cp -r {{BUILD_DIR}}/standard/lib dist/standard/
    cp {{CPP_DIR}}/tools/linux_dist_tools/* dist/standard/
    cd dist && tar -czf paddleocr-json-standard.tar.gz standard/

    # Static package
    mkdir -p dist/static
    cp {{BUILD_DIR}}/static/bin/PaddleOCR-json dist/static/
    cp {{CPP_DIR}}/tools/linux_dist_tools/run.sh dist/static/
    cd dist && tar -czf paddleocr-json-static.tar.gz static/

    # Musl package
    mkdir -p dist/musl
    cp {{BUILD_DIR}}/musl/bin/PaddleOCR-json dist/musl/
    cp {{CPP_DIR}}/tools/linux_dist_tools/run.sh dist/musl/
    cd dist && tar -czf paddleocr-json-musl.tar.gz musl/

    @echo "Packages created in dist/"

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
