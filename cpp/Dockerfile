# Build environment
FROM debian:stable AS build

# Build arguments
ARG ENABLE_REMOTE_EXIT="ON" \
    ENABLE_JSON_IMAGE_PATH="OFF"

SHELL ["/bin/bash", "-c"]

# Check AVX instruction set, exit if not supported
RUN if [[ -z $(lscpu | grep avx) ]] ; \
    then echo "Current CPU doesn't support AVX." ; \
    exit -1 ; \
    fi

# Install build environment
RUN \
    apt update -y && \
    apt install -y libopencv-dev && \
    apt install -y wget tar zip unzip git gcc g++ cmake make pkg-config

WORKDIR /src/

# Download model library
RUN wget https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Linux/CPU/gcc8.2_avx_mkl/paddle_inference.tgz && \
    tar -xf paddle_inference.tgz

COPY . .

ENV PADDLE_LIB="/src/.source/paddle_inference/"

# Build project + compile
RUN cd /src/ && \
    mkdir -p .source && cp -r /src/paddle_inference .source && \
    # Check if libopencv-dev is properly installed
    if [[ -z $(pkg-config --silence-errors --libs opencv4) ]] ; \
        then echo "Cannot find libopencv-dev in the system." ; exit -1 ; \
    fi && \
    mkdir -p build && \
        cmake -S . -B build \
        -DPADDLE_LIB=$PADDLE_LIB \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_CLIPBOARD=OFF \
        -DENABLE_REMOTE_EXIT=${ENABLE_REMOTE_EXIT} \
        -DENABLE_JSON_IMAGE_PATH=${ENABLE_JSON_IMAGE_PATH} && \
    cmake --build build --config=Release && \
    cmake --install build --prefix build/install



# Deployment environment
FROM debian:stable-slim

SHELL ["/bin/bash", "-c"]

# Install runtime environment
RUN apt update -y && \
    apt install -y wget unzip libopencv-core-dev libopencv-imgcodecs-dev libopencv-imgproc-dev libgomp1

# Install PaddleOCR-json related files to system
COPY --from=build /src/build/install/ /usr/

WORKDIR /app

# Download prediction library
RUN wget https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip && \
	mv models_v1.4.1.zip models.zip && \
    unzip -x models.zip && rm models.zip

# Expose port 3746, then run PaddleOCR-json in socket mode
EXPOSE 3746
ENTRYPOINT ["PaddleOCR-json", "-models_path=/app/models", "-addr=any", "-port=3746"]
