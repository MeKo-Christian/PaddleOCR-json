# Multi-stage Dockerfile for PaddleOCR-json (Just-compatible)
FROM ubuntu:22.04 AS base

# Install system dependencies
RUN apt-get update && apt-get install -y \
    wget tar zip unzip git \
    gcc g++ cmake make pkg-config \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Download dependencies
RUN cd cpp/.source && \
    wget -O paddle_inference.tgz https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Linux/CPU/gcc8.2_avx_mkl/paddle_inference.tgz && \
    tar -xzf paddle_inference.tgz && \
    mv paddle_inference paddle_inference_manylinux_cpu_avx_mkl_gcc8.2 && \
    wget -O models.zip https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip && \
    unzip models.zip && \
    wget -O opencv.zip https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.0-beta.2/opencv-release_debian_x86-64.zip && \
    unzip opencv.zip

# Build stage
FROM base AS builder

# Build arguments
ARG BUILD_MODE=standard
ARG BUILD_TYPE=Release

# Build the project
RUN cd cpp && \
    ./tools/linux_build_enhanced.sh ${BUILD_MODE} ${BUILD_TYPE}

# Runtime stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Create app directory
WORKDIR /app

# Copy built binaries and libraries
COPY --from=builder /app/cpp/build-${BUILD_MODE}/bin/ /app/bin/
COPY --from=builder /app/cpp/build-${BUILD_MODE}/lib/ /app/lib/
COPY --from=builder /app/cpp/.source/models/ /app/models/

# Copy distribution tools
COPY cpp/tools/linux_dist_tools/ /app/

# Set library path
ENV LD_LIBRARY_PATH=/app/lib:$LD_LIBRARY_PATH

# Create symlink for easy access
RUN ln -sf /app/bin/PaddleOCR-json /usr/local/bin/paddleocr-json

# Default command
CMD ["paddleocr-json", "--help"]
