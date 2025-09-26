#!/bin/bash -e

# Musl-based static build script for PaddleOCR-json
# This creates a fully static binary that can run on any Linux system

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
SRC_DIR="$SCRIPT_DIR/.."
SOURCE_DIR="$SRC_DIR/.source"

# Configuration
MUSL_VERSION="1.2.3"
OPENCV_VERSION="4.10.0"
BUILD_TYPE="Release"
# Place outputs under repo root ./build/musl
OUTPUT_DIR="$SCRIPT_DIR/../../build/musl"

echo "=== PaddleOCR-json Musl Static Build ==="
echo "Build type: $BUILD_TYPE"
echo "Output dir: $OUTPUT_DIR"

# Create output directory
mkdir -p "$OUTPUT_DIR"

MUSL_DIR="$OUTPUT_DIR/musl-$MUSL_VERSION"
# Only build a local musl toolchain if a musl C++ compiler is not available
if ! command -v x86_64-linux-musl-g++ >/dev/null 2>&1; then
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
fi

# Set musl toolchain
# Try cached prebuilt toolchain first
if [ -d "$SOURCE_DIR/toolchains/x86_64-linux-musl-cross/bin" ]; then
  export PATH="$SOURCE_DIR/toolchains/x86_64-linux-musl-cross/bin:$PATH"
fi
# Prefer system-provided cross C++ compiler if available (e.g., x86_64-linux-musl-g++)
if command -v x86_64-linux-musl-g++ >/dev/null 2>&1; then
  export CC="x86_64-linux-musl-gcc"
  export CXX="x86_64-linux-musl-g++"
elif [ -x "$MUSL_DIR/install/bin/musl-gcc" ]; then
  export CC="$MUSL_DIR/install/bin/musl-gcc"
  # musl-tools often lacks a dedicated g++; musl-gcc may not handle C++ standard headers
  # If musl-g++ exists next to musl-gcc, prefer it
  if [ -x "${MUSL_DIR}/install/bin/musl-g++" ]; then
    export CXX="${MUSL_DIR}/install/bin/musl-g++"
  else
    export CXX="$MUSL_DIR/install/bin/musl-gcc"
  fi
else
  echo "ERROR: No musl C/C++ compiler found. Install a cross toolchain (e.g., x86_64-linux-musl-g++)."
  exit 1
fi
export CFLAGS="-Os"
export CXXFLAGS="-Os"
export LDFLAGS=""

# Build OpenCV statically with musl
OPENCV_BUILD_DIR="$OUTPUT_DIR/opencv-build"
OPENCV_INSTALL_DIR="$OUTPUT_DIR/opencv-install"
OPENCV_CONFIG_CMAKE="$OPENCV_INSTALL_DIR/lib/cmake/opencv4/OpenCVConfig.cmake"
if [ ! -f "$OPENCV_CONFIG_CMAKE" ]; then
    echo "Building OpenCV with musl..."

    # Ensure OpenCV source is present under $OUTPUT_DIR
    if [ ! -d "$OUTPUT_DIR/opencv-$OPENCV_VERSION" ]; then
        if [ ! -f "$OUTPUT_DIR/opencv-$OPENCV_VERSION.tar.gz" ]; then
            wget -O "$OUTPUT_DIR/opencv-$OPENCV_VERSION.tar.gz" https://github.com/opencv/opencv/archive/refs/tags/$OPENCV_VERSION.tar.gz
        fi
        tar -xzf "$OUTPUT_DIR/opencv-$OPENCV_VERSION.tar.gz" -C "$OUTPUT_DIR"
    fi

    mkdir -p "$OPENCV_BUILD_DIR"
    cd "$OPENCV_BUILD_DIR"

    GEN=""; if command -v ninja >/dev/null 2>&1; then GEN="-G Ninja"; fi
    cmake $GEN "$OUTPUT_DIR/opencv-$OPENCV_VERSION" \
        -DCMAKE_INSTALL_PREFIX="$OPENCV_INSTALL_DIR" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_LIST=core,imgcodecs,imgproc \
        -DBUILD_opencv_world=OFF \
        -DOPENCV_GENERATE_PKGCONFIG=ON \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_opencv_apps=OFF \
        -DOPENCV_ENABLE_NONFREE=OFF \
        -DOPENCV_FORCE_3RDPARTY_BUILD=ON \
        -DWITH_ZLIB=ON \
        -DWITH_JPEG=ON \
        -DWITH_PNG=ON \
        -DWITH_TIFF=OFF \
        -DWITH_OPENEXR=OFF \
        -DWITH_OPENJPEG=OFF \
        -DWITH_JASPER=OFF \
        -DWITH_WEBP=OFF \
        -DWITH_IPP=OFF \
        -DWITH_TBB=OFF \
        -DWITH_EIGEN=OFF \
        -DWITH_OPENCL=OFF \
        -DWITH_GTK=OFF \
        -DWITH_QT=OFF \
        -DWITH_FFMPEG=OFF \
        -DWITH_GSTREAMER=OFF \
        -DBUILD_PERF_TESTS=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_DOCS=OFF \
        -DBUILD_JAVA=OFF \
        -DBUILD_opencv_python2=OFF \
        -DBUILD_opencv_python3=OFF \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_CXX_COMPILER="$CXX"

    cmake --build . -- -j$(nproc)
    cmake --install .
    cd ..
fi

# Build PaddleOCR-json
echo "Building PaddleOCR-json with musl..."
# Resolve Paddle inference path: prefer PADDLE_MUSL_DIR override
if [ -n "$PADDLE_MUSL_DIR" ] && [ -d "$PADDLE_MUSL_DIR" ]; then
    PADDLE_LIB="$PADDLE_MUSL_DIR"
else
    PADDLE_LIB="$(ls -d "$SOURCE_DIR"/*paddle_inference*/ 2>/dev/null | head -n1)"
fi
if [ -z "$PADDLE_LIB" ]; then
    echo "ERROR: Paddle inference directory not found in $SOURCE_DIR"
    exit 1
fi

GEN=""; if command -v ninja >/dev/null 2>&1; then GEN="-G Ninja"; fi
# Ensure pkg-config can locate the musl OpenCV install
export PKG_CONFIG_PATH="$OPENCV_INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"
cmake $GEN -S "$SRC_DIR" -B "$OUTPUT_DIR" \
    -DPADDLE_LIB="$PADDLE_LIB" \
    -DUSE_OPENCV_PKGCONFIG=ON \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DWITH_STATIC_LIB=ON \
    -DWITH_MKL=OFF \
    -DWITH_GPU=OFF \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_C_FLAGS="$CFLAGS" \
    -DCMAKE_CXX_FLAGS="$CXXFLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$LDFLAGS"

cmake --build "$OUTPUT_DIR" --config "$BUILD_TYPE"

echo "=== Build Complete ==="
echo "Static binary location: $OUTPUT_DIR/bin/PaddleOCR-json"
echo "To test: $OUTPUT_DIR/bin/PaddleOCR-json --help"

# Verify it's statically linked
echo "=== Verifying static linking ==="
ldd "$OUTPUT_DIR/bin/PaddleOCR-json" || echo "No dynamic dependencies - fully static!"
