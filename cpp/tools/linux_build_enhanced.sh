#!/bin/bash -e

# Enhanced Linux build script for PaddleOCR-json
# Supports multiple build modes: standard, static, musl, cross-compile

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)

# Configuration
BUILD_MODE="${1:-standard}"  # standard, static, musl, cross
TARGET_ARCH="${2:-}"         # For cross-compilation
BUILD_TYPE="${3:-Release}"
OUTPUT_DIR="$SCRIPT_DIR/../../build/$BUILD_MODE"

echo "=== PaddleOCR-json Enhanced Build ==="
echo "Build mode: $BUILD_MODE"
echo "Target arch: ${TARGET_ARCH:-native}"
echo "Build type: $BUILD_TYPE"
echo "Output dir: $OUTPUT_DIR"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Set build-specific variables
CMAKE_EXTRA_ARGS=""
TOOLCHAIN_VARS=""

case "$BUILD_MODE" in
    "standard")
        echo "Building standard shared library version..."
        CMAKE_EXTRA_ARGS="-DWITH_STATIC_LIB=OFF -DENABLE_C_API=ON"
        ;;
    "static")
        echo "Building static binary..."
        CMAKE_EXTRA_ARGS="-DWITH_STATIC_LIB=ON -DBUILD_STATIC_BINARY=ON -DENABLE_C_API=ON"
        ;;
    "musl")
        echo "Building musl static binary..."
        if [ ! -f "$SCRIPT_DIR/linux_build_musl.sh" ]; then
            echo "Musl build script not found. Using standard static build."
            CMAKE_EXTRA_ARGS="-DWITH_STATIC_LIB=ON -DBUILD_STATIC_BINARY=ON -DENABLE_C_API=ON"
        else
            exec "$SCRIPT_DIR/linux_build_musl.sh"
        fi
        ;;
    "cross")
        echo "Cross-compiling for $TARGET_ARCH..."
        if [ -z "$TARGET_ARCH" ]; then
            echo "Error: TARGET_ARCH required for cross-compilation"
            echo "Usage: $0 cross <architecture>"
            exit 1
        fi
        if [ ! -f "$SCRIPT_DIR/linux_build_cross.sh" ]; then
            echo "Cross-compilation script not found."
            exit 1
        fi
        exec "$SCRIPT_DIR/linux_build_cross.sh" "$TARGET_ARCH"
        ;;
    *)
        echo "Unknown build mode: $BUILD_MODE"
        echo "Supported modes: standard, static, musl, cross"
        exit 1
        ;;
esac

# Set Paddle and OpenCV paths
PADDLE_LIB_DIR="$SCRIPT_DIR/../../cpp/.source"
PADDLE_LIB="$(ls -d $PADDLE_LIB_DIR/*paddle_inference*/ | head -n1)"
OPENCV_DIR="$PADDLE_LIB_DIR/opencv-release"

if [ ! -d "$PADDLE_LIB" ]; then
    echo "Error: Paddle inference library not found in .source/"
    echo "Please download it first. See README-linux.md for instructions."
    exit 1
fi

echo "Paddle lib: $PADDLE_LIB"
echo "OpenCV dir: $OPENCV_DIR"

# Build PaddleOCR-json
echo "Building PaddleOCR-json..."
GEN=""
if command -v ninja >/dev/null 2>&1; then GEN="-G Ninja"; fi
cmake -S "$SCRIPT_DIR/.." -B "$OUTPUT_DIR" $GEN \
    -DPADDLE_LIB="$PADDLE_LIB" \
    -DOPENCV_DIR="$OPENCV_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DWITH_MKL=ON \
    -DWITH_GPU=OFF \
    $CMAKE_EXTRA_ARGS \
    $TOOLCHAIN_VARS

cmake --build "$OUTPUT_DIR" --config "$BUILD_TYPE"

# Install if requested
if [ "${4:-}" = "install" ]; then
    echo "Installing to system..."
    sudo cmake --install "$OUTPUT_DIR"
fi

echo "=== Build Complete ==="
echo "Binary location: $OUTPUT_DIR/bin/PaddleOCR-json"

if [ -f "$OUTPUT_DIR/bin/PaddleOCR-json" ]; then
    echo "Binary size: $(stat -c%s "$OUTPUT_DIR/bin/PaddleOCR-json") bytes"

    # Check if static
    if ldd "$OUTPUT_DIR/bin/PaddleOCR-json" &>/dev/null; then
        echo "Binary type: Dynamic"
    else
        echo "Binary type: Static"
    fi
fi

# Build C API library info
if [ "$BUILD_MODE" != "musl" ] && [ "$BUILD_MODE" != "cross" ]; then
    echo "C API library: $OUTPUT_DIR/lib/libpaddleocr_c.so"
    echo "C API header:  $SCRIPT_DIR/../include/paddleocr_c_api.h"
fi

echo ""
echo "Usage examples:"
echo "  # Standard build"
echo "  $0 standard"
echo ""
echo "  # Static build"
echo "  $0 static"
echo ""
echo "  # Musl static build"
echo "  $0 musl"
echo ""
echo "  # Cross-compile for ARM64"
echo "  $0 cross aarch64"
echo ""
echo "  # Build and install"
echo "  $0 standard Release install"
