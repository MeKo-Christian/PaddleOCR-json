#!/bin/bash -e

# Musl-based static build script for PaddleOCR-json
# This creates a fully static binary that can run on any Linux system

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
cd "$SCRIPT_DIR/.."

# Configuration
MUSL_VERSION="1.2.3"
OPENCV_VERSION="4.10.0"
BUILD_TYPE="Release"
OUTPUT_DIR="$SCRIPT_DIR/../build-musl"

echo "=== PaddleOCR-json Musl Static Build ==="
echo "Build type: $BUILD_TYPE"
echo "Output dir: $OUTPUT_DIR"

# Create output directory
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Download and build musl toolchain if not exists
MUSL_DIR="$OUTPUT_DIR/musl-$MUSL_VERSION"
if [ ! -d "$MUSL_DIR" ]; then
    echo "Downloading musl $MUSL_VERSION..."
    wget -O musl-$MUSL_VERSION.tar.gz https://musl.libc.org/releases/musl-$MUSL_VERSION.tar.gz
    tar -xzf musl-$MUSL_VERSION.tar.gz

    echo "Building musl toolchain..."
    cd "musl-$MUSL_VERSION"
    ./configure --prefix="$MUSL_DIR/install" --disable-shared
    make -j$(nproc)
    make install
    cd ..
fi

# Set musl toolchain
export CC="$MUSL_DIR/install/bin/musl-gcc"
export CXX="$MUSL_DIR/install/bin/musl-gcc"
export CFLAGS="-static -Os"
export CXXFLAGS="-static -Os"
export LDFLAGS="-static"

# Build OpenCV statically with musl
OPENCV_BUILD_DIR="$OUTPUT_DIR/opencv-build"
if [ ! -d "$OPENCV_BUILD_DIR" ]; then
    echo "Building OpenCV with musl..."

    # Download OpenCV if not exists
    if [ ! -d "opencv-$OPENCV_VERSION" ]; then
        wget -O opencv-$OPENCV_VERSION.tar.gz https://github.com/opencv/opencv/archive/refs/tags/$OPENCV_VERSION.tar.gz
        tar -xzf opencv-$OPENCV_VERSION.tar.gz
    fi

    mkdir -p "$OPENCV_BUILD_DIR"
    cd "$OPENCV_BUILD_DIR"

    cmake "../opencv-$OPENCV_VERSION" \
        -DCMAKE_INSTALL_PREFIX="$OUTPUT_DIR/opencv-install" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_LIST=core,imgcodecs,imgproc \
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
        -DBUILD_opencv_python3=OFF \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX"

    make -j$(nproc)
    make install
    cd ..
fi

# Build PaddleOCR-json
echo "Building PaddleOCR-json with musl..."
PADDLE_LIB="$(pwd)/$(ls -d .source/*paddle_inference*/ | head -n1)"

cmake -S .. -B build-musl \
    -DPADDLE_LIB="$PADDLE_LIB" \
    -DOPENCV_DIR="$OUTPUT_DIR/opencv-install" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DWITH_STATIC_LIB=ON \
    -DWITH_MKL=OFF \
    -DWITH_GPU=OFF \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"

cmake --build build-musl --config "$BUILD_TYPE"

echo "=== Build Complete ==="
echo "Static binary location: $OUTPUT_DIR/build-musl/bin/PaddleOCR-json"
echo "To test: $OUTPUT_DIR/build-musl/bin/PaddleOCR-json --help"

# Verify it's statically linked
echo "=== Verifying static linking ==="
ldd "$OUTPUT_DIR/build-musl/bin/PaddleOCR-json" || echo "No dynamic dependencies - fully static!"
