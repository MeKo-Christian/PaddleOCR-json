#!/usr/bin/env bash
set -euo pipefail

JOBS="${1:-8}"
SRC=/opt/paddle
BUILD=/opt/build
STAGE=/out/paddle_inference

echo "=== Building Paddle Inference (musl, OpenBLAS) ==="
echo "Paddle src: $SRC"
echo "Build dir : $BUILD"
echo "Stage dir : $STAGE"
echo "Jobs      : $JOBS"

mkdir -p "$BUILD" "$STAGE"

# Common CPU/OpenBLAS CMake options for Paddle
# These flags may vary by Paddle version; adjust as needed.
cmake -S "$SRC" -B "$BUILD" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-include cstdint -Wno-error" \
  -DWITH_GPU=OFF \
  -DWITH_ROCM=OFF \
  -DWITH_XPU=OFF \
  -DWITH_NCCL=OFF \
  -DWITH_MKL=OFF \
  -DWITH_AVX=ON \
  -DWITH_TESTING=OFF \
  -DWITH_PYTHON=OFF \
  -DWITH_DISTRIBUTE=OFF \
  -DWITH_TENSORRT=OFF \
  -DWITH_SHARED=ON \
  -DWITH_INFERENCE=ON \
  -DWITH_BLAS=openblas \
  -DOPENBLAS_ROOT=/usr \
  -DWITH_STACKTRACE=OFF \
  -DON_INFER=ON || true

# Try known inference targets; not all versions expose the same
if ! cmake --build "$BUILD" --target inference_api_lib -j"$JOBS"; then
  echo "inference_api_lib target missing; trying 'inference_lib_dist'..." >&2
  if ! cmake --build "$BUILD" --target inference_lib_dist -j"$JOBS"; then
    echo "Fallback to building core libs; final layout may differ." >&2
    cmake --build "$BUILD" -j"$JOBS"
  fi
fi

# Heuristic staging: collect headers/libs Paddle installs in common layouts
echo "=== Staging artifacts ==="
mkdir -p "$STAGE/paddle/lib" "$STAGE/third_party/install" "$STAGE/paddle/include"

# Headers
if [ -d "$SRC/paddle/fluid/inference/api" ]; then
  mkdir -p "$STAGE/paddle/include/paddle/fluid/inference/api"
  cp -r "$SRC/paddle/fluid/inference/api"/* "$STAGE/paddle/include/paddle/fluid/inference/api/" || true
fi

# Libraries (search build tree for typical inference libs)
find "$BUILD" -maxdepth 4 -type f \( -name 'libpaddle_inference.so' -o -name 'libpaddle_inference_c.*' -o -name 'libpaddle_inference.*.so' \) -print -exec cp -v {} "$STAGE/paddle/lib/" \; || true

# Third-party installs commonly under build/third_party/…/install
for tp in protobuf zlib xxhash glog gflags openblas onednn mklml; do
  d=$(find "$BUILD" -type d -path "*third_party/*${tp}*/install" -print -quit) || true
  if [ -n "$d" ]; then
    mkdir -p "$STAGE/third_party/install/${tp}"
    cp -rv "$d"/* "$STAGE/third_party/install/${tp}/" || true
  fi
done

# If Paddle used system OpenBLAS, ensure we stage it under third_party/install/openblas
if [ ! -d "$STAGE/third_party/install/openblas" ]; then
  echo "Staging system OpenBLAS into third_party/install/openblas"
  mkdir -p "$STAGE/third_party/install/openblas/lib" "$STAGE/third_party/install/openblas/include"
  # Prefer static if available
  if [ -f /usr/lib/libopenblas.a ]; then
    cp -v /usr/lib/libopenblas.a "$STAGE/third_party/install/openblas/lib/" || true
  fi
  if [ -f /usr/lib/libopenblas.so ]; then
    cp -v /usr/lib/libopenblas.so* "$STAGE/third_party/install/openblas/lib/" || true
  fi
  # Headers
  if [ -d /usr/include ]; then
    cp -v /usr/include/cblas.h "$STAGE/third_party/install/openblas/include/" 2>/dev/null || true
    cp -v /usr/include/openblas* "$STAGE/third_party/install/openblas/include/" 2>/dev/null || true
  fi
fi

echo "=== Done ==="
echo "Staged into: $STAGE"
