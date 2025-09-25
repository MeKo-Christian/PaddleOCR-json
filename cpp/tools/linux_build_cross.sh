#!/bin/bash -e

# Cross-compilation script for PaddleOCR-json
# Supports ARM64, ARM32, and other architectures

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
cd "$SCRIPT_DIR/.."
SOURCE_DIR="$SCRIPT_DIR/../.source"

# Configuration
TARGET_ARCH="${1:-aarch64}"  # Default to ARM64
BUILD_TYPE="Release"
# Place outputs under repo root ./build/<arch>
OUTPUT_DIR="$SCRIPT_DIR/../../build/$TARGET_ARCH"

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

# Resolve absolute tool paths for CMake
AR_BIN="$(command -v "$AR")"
RANLIB_BIN="$(command -v "$RANLIB")"
STRIP_BIN="$(command -v "$STRIP")"

# Prefer externally provided OpenCV for target arch if available
if [ -n "$OPENCV_AARCH_DIR" ] && [ -f "$OPENCV_AARCH_DIR/lib/cmake/opencv4/OpenCVConfig.cmake" ]; then
    echo "Using external OpenCV at $OPENCV_AARCH_DIR"
    OPENCV_INSTALL_DIR="$OPENCV_AARCH_DIR"
    OPENCV_CONFIG_CMAKE="$OPENCV_INSTALL_DIR/lib/cmake/opencv4/OpenCVConfig.cmake"
fi

# Build OpenCV for target architecture (minimal feature set to ease cross build)
OPENCV_BUILD_DIR="$OUTPUT_DIR/opencv-build"
OPENCV_INSTALL_DIR="$OUTPUT_DIR/opencv-install"
OPENCV_CONFIG_CMAKE="$OPENCV_INSTALL_DIR/lib/cmake/opencv4/OpenCVConfig.cmake"
if [ ! -f "$OPENCV_CONFIG_CMAKE" ]; then
    echo "Building OpenCV for $TARGET_ARCH..."

    # Download OpenCV if not exists
    OPENCV_VERSION="4.10.0"
    if [ ! -d "opencv-$OPENCV_VERSION" ]; then
        wget -O opencv-$OPENCV_VERSION.tar.gz https://github.com/opencv/opencv/archive/refs/tags/$OPENCV_VERSION.tar.gz
        tar -xzf opencv-$OPENCV_VERSION.tar.gz
    fi

    # Clean any previous build directory to avoid generator mismatches
    rm -rf "$OPENCV_BUILD_DIR"
    mkdir -p "$OPENCV_BUILD_DIR"
    cd "$OPENCV_BUILD_DIR"

    GEN=""; if command -v ninja >/dev/null 2>&1; then GEN="-G Ninja"; fi
    cmake $GEN "../opencv-$OPENCV_VERSION" \
        -DCMAKE_INSTALL_PREFIX="$OPENCV_INSTALL_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_SYSTEM_NAME="Linux" \
        -DCMAKE_SYSTEM_PROCESSOR="$TARGET_ARCH" \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX" \
        -DCMAKE_ASM_COMPILER="$CC" \
        -DCMAKE_STRIP="$STRIP_BIN" \
        -DCMAKE_AR="$AR_BIN" \
        -DCMAKE_RANLIB="$RANLIB_BIN" \
        -DBUILD_LIST=core,imgcodecs,imgproc \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_opencv_world=OFF \
        -DOPENCV_FORCE_3RDPARTY_BUILD=ON \
        -DWITH_ZLIB=ON \
        -DWITH_JPEG=ON \
        -DWITH_PNG=ON \
        -DWITH_TIFF=OFF \
        -DWITH_OPENJPEG=OFF \
        -DWITH_JASPER=OFF \
        -DWITH_WEBP=OFF \
        -DBUILD_PERF_TESTS=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_DOCS=OFF \
        -DBUILD_JAVA=OFF \
        -DBUILD_opencv_python2=OFF \
        -DBUILD_opencv_python3=OFF

    cmake --build . -- -j$(nproc)
    cmake --install .
    cd ..
fi

# Build PaddleOCR-json
echo "Building PaddleOCR-json for $TARGET_ARCH..."
PADDLE_LIB="$(ls -d "$SOURCE_DIR"/*paddle_inference*/ 2>/dev/null | head -n1)"
if [ -z "$PADDLE_LIB" ]; then
    echo "ERROR: Paddle inference directory not found in $SOURCE_DIR"
    exit 1
fi

GEN=""; if command -v ninja >/dev/null 2>&1; then GEN="-G Ninja"; fi
cmake $GEN -S "$SCRIPT_DIR/.." -B "$OUTPUT_DIR" \
    -DENABLE_CROSS_COMPILE=ON \
    -DCROSS_COMPILE_PREFIX="$TOOLCHAIN_PREFIX" \
    -DPADDLE_LIB="$PADDLE_LIB" \
    -DOPENCV_DIR="$OPENCV_INSTALL_DIR" \
    -DOpenCV_DIR="$OPENCV_INSTALL_DIR/lib/cmake/opencv4" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DWITH_GPU=OFF \
    -DCMAKE_SYSTEM_NAME="Linux" \
    -DCMAKE_SYSTEM_PROCESSOR="$TARGET_ARCH"

cmake --build "$OUTPUT_DIR" --config "$BUILD_TYPE"

echo "=== Cross-Compilation Complete ==="
echo "Binary location: $OUTPUT_DIR/bin/PaddleOCR-json"

# Strip binary to reduce size
if [ -f "$OUTPUT_DIR/bin/PaddleOCR-json" ]; then
    "$STRIP" "$OUTPUT_DIR/bin/PaddleOCR-json"
    echo "Stripped binary size: $(stat -c%s "$OUTPUT_DIR/bin/PaddleOCR-json") bytes"
fi
