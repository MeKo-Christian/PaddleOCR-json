#!/bin/bash -e

# Cross-compilation script for PaddleOCR-json
# Supports ARM64, ARM32, and other architectures

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
cd "$SCRIPT_DIR/.."

# Configuration
TARGET_ARCH="${1:-aarch64}"  # Default to ARM64
BUILD_TYPE="Release"
OUTPUT_DIR="$SCRIPT_DIR/../build-$TARGET_ARCH"

echo "=== PaddleOCR-json Cross-Compilation ==="
echo "Target architecture: $TARGET_ARCH"
echo "Build type: $BUILD_TYPE"
echo "Output dir: $OUTPUT_DIR"

# Set toolchain prefix based on architecture
case "$TARGET_ARCH" in
    "aarch64"|"arm64")
        TOOLCHAIN_PREFIX="aarch64-linux-gnu-"
        ;;
    "arm"|"arm32"|"armhf")
        TOOLCHAIN_PREFIX="arm-linux-gnueabihf-"
        ;;
    "x86_64")
        TOOLCHAIN_PREFIX="x86_64-linux-gnu-"
        ;;
    "i386"|"x86")
        TOOLCHAIN_PREFIX="i386-linux-gnu-"
        ;;
    *)
        echo "Unsupported architecture: $TARGET_ARCH"
        echo "Supported: aarch64, arm, x86_64, i386"
        exit 1
        ;;
esac

# Check if toolchain is installed
if ! command -v "${TOOLCHAIN_PREFIX}gcc" &> /dev/null; then
    echo "Cross-compiler ${TOOLCHAIN_PREFIX}gcc not found."
    echo "Please install with: sudo apt install gcc-${TARGET_ARCH}-linux-gnu g++-${TARGET_ARCH}-linux-gnu"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Set up cross-compilation environment
export CC="${TOOLCHAIN_PREFIX}gcc"
export CXX="${TOOLCHAIN_PREFIX}g++"
export STRIP="${TOOLCHAIN_PREFIX}strip"
export AR="${TOOLCHAIN_PREFIX}ar"
export RANLIB="${TOOLCHAIN_PREFIX}ranlib"

# Build OpenCV for target architecture
OPENCV_BUILD_DIR="$OUTPUT_DIR/opencv-build"
if [ ! -d "$OPENCV_BUILD_DIR" ]; then
    echo "Building OpenCV for $TARGET_ARCH..."

    # Download OpenCV if not exists
    OPENCV_VERSION="4.10.0"
    if [ ! -d "opencv-$OPENCV_VERSION" ]; then
        wget -O opencv-$OPENCV_VERSION.tar.gz https://github.com/opencv/opencv/archive/refs/tags/$OPENCV_VERSION.tar.gz
        tar -xzf opencv-$OPENCV_VERSION.tar.gz
    fi

    mkdir -p "$OPENCV_BUILD_DIR"
    cd "$OPENCV_BUILD_DIR"

    cmake "../opencv-$OPENCV_VERSION" \
        -DCMAKE_INSTALL_PREFIX="$OUTPUT_DIR/opencv-install" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_SYSTEM_NAME="Linux" \
        -DCMAKE_SYSTEM_PROCESSOR="$TARGET_ARCH" \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DCMAKE_STRIP="$STRIP" \
        -DCMAKE_AR="$AR" \
        -DCMAKE_RANLIB="$RANLIB" \
        -DBUILD_LIST=core,imgcodecs,imgproc \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_opencv_world=OFF \
        -DOPENCV_FORCE_3RDPARTY_BUILD=ON \
        -DWITH_ZLIB=ON \
        -DWITH_JPEG=ON \
        -DWITH_PNG=ON \
        -DWITH_TIFF=ON \
        -DWITH_OPENJPEG=ON \
        -DWITH_JASPER=ON \
        -DWITH_WEBP=ON \
        -DBUILD_PERF_TESTS=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_DOCS=OFF \
        -DBUILD_JAVA=OFF \
        -DBUILD_opencv_python2=OFF \
        -DBUILD_opencv_python3=OFF

    make -j$(nproc)
    make install
    cd ..
fi

# Build PaddleOCR-json
echo "Building PaddleOCR-json for $TARGET_ARCH..."
PADDLE_LIB="$(pwd)/$(ls -d .source/*paddle_inference*/ | head -n1)"

cmake -S .. -B "build-$TARGET_ARCH" \
    -DENABLE_CROSS_COMPILE=ON \
    -DCROSS_COMPILE_PREFIX="$TOOLCHAIN_PREFIX" \
    -DPADDLE_LIB="$PADDLE_LIB" \
    -DOPENCV_DIR="$OUTPUT_DIR/opencv-install" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DWITH_GPU=OFF \
    -DCMAKE_SYSTEM_NAME="Linux" \
    -DCMAKE_SYSTEM_PROCESSOR="$TARGET_ARCH"

cmake --build "build-$TARGET_ARCH" --config "$BUILD_TYPE"

echo "=== Cross-Compilation Complete ==="
echo "Binary location: $OUTPUT_DIR/build-$TARGET_ARCH/bin/PaddleOCR-json"

# Strip binary to reduce size
if [ -f "$OUTPUT_DIR/build-$TARGET_ARCH/bin/PaddleOCR-json" ]; then
    "$STRIP" "$OUTPUT_DIR/build-$TARGET_ARCH/bin/PaddleOCR-json"
    echo "Stripped binary size: $(stat -c%s "$OUTPUT_DIR/build-$TARGET_ARCH/bin/PaddleOCR-json") bytes"
fi
