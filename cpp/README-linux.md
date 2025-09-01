# PaddleOCR-json V1.4 Linux Build Guide

> Note: This development version is based on Paddle Inference 3.0.0 inference backend, and there is a significant performance degradation issue on ordinary home CPUs without AVX512 instruction set. Ordinary users are recommended to switch to the stable branch of this project.

This document helps how to compile PaddleOCR-json V1.4 (corresponding to PaddleOCR v2.8) on Linux. Recommended for readers with some Linux command line experience.

This article refers to PaddleOCR official's [compilation guide](https://github.com/PaddlePaddle/PaddleOCR/blob/release/2.8/deploy/cpp_infer/readme_ch.md), but it is recommended to follow this article.

In addition, this article will use Debian/Ubuntu series linux as an example for explanation. Users of other linux distributions please replace some corresponding commands (such as apt).

Related documents:
- [Windows Build Guide](./README.md)
- [Docker Deployment](./README-docker.md)
- Other platforms [Porting Guide](docs/Migration-Guide.md)

Reference documents:
- [PaddleOCR Official Documentation](https://github.com/PaddlePaddle/PaddleOCR/blob/release/2.8/deploy/cpp_infer/readme_ch.md#12-%E7%BC%96%E8%AF%91opencv%E5%BA%93)
- [OpenCV Official Documentation](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html)


## 1. Preparation

### 1.0 Compatibility Check:

**PaddleOCR-json only supports CPUs with AVX instruction set. For more details, please check [CPU Requirements](../README.md#offline-ocr-components-series-projects) and [Compatibility](../README.md#compatibility).**

Please check your CPU compatibility first:

```sh
lscpu | grep avx
```

**If your CPU supports AVX instruction set, your output will look like this (you can find 'avx' characters in the output):**

```
Flags:                              fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology tsc_reliable nonstop_tsc cpuid pni pclmulqdq vmx ssse3 fma cx16 sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni umip waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize flush_l1d arch_capabilities
```

**If you don't see any output, it means your CPU does not support AVX instruction set.**

> [!TIP]
> If your CPU does not support AVX instruction set, we recommend you try the neighboring [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json)
>
> Of course, you can also replace with a prediction library that does not require AVX instruction set to compile PaddleOCR-json (such as `manylinux_cpu_noavx_openblas_gcc8.2`). But it probably won't run.
> 
> If you insist on using a prediction library without AVX instruction set to compile PaddleOCR-json, if you encounter problems during compilation and runtime, it is likely due to prediction library incompatibility. At this time, you can prioritize checking [PaddleOCR official repository](https://github.com/PaddlePaddle/PaddleOCR/issues)
> * If you encounter a `core dump` error, you can also check [this issue](https://github.com/hiroi-sora/PaddleOCR-json/issues/170)

### 1.1 Install Required Tools

```sh
sudo apt install wget tar zip unzip git gcc g++ cmake make libgomp1
```

- wget (for downloading prediction library)
- tar, zip, unzip (for decompressing software)
- git
- gcc and g++
- cmake and make
- libgomp1 (OpenMP shared library, PaddleOCR underlying dependency)

### 1.2 Download Required Resources

PaddleOCR-json source code:

```sh
git clone https://github.com/hiroi-sora/PaddleOCR-json.git
cd PaddleOCR-json
```

Download resource library:

```sh
# Storage directory
mkdir -p cpp/.source
cd cpp/.source
# Inference library
wget https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Linux/CPU/gcc8.2_avx_mkl/paddle_inference.tgz
tar -xf paddle_inference.tgz
mv paddle_inference paddle_inference_manylinux_cpu_avx_mkl_gcc8.2
# Model library
wget https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip
unzip -x models_v1.4.1.zip
```

- [paddle_inference](https://www.paddlepaddle.org.cn/inference/master/guides/install/download_lib.html) (Linux, C++ prediction library, gcc compiler version, manylinux_cpu_avx_mkl_gcc8.2)
- [Model library](https://github.com/hiroi-sora/PaddleOCR-json/releases/tag/v1.4.1-dev) (models.zip)

### 1.3 Prepare OpenCV

#### Method 1: Download pre-compiled lightweight OpenCV package (recommended)

```sh
wget https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.0-beta.2/opencv-release_debian_x86-64.zip
unzip -x opencv-release_debian_x86-64.zip
```

This OpenCV library only compiles the few dependencies required by PaddleOCR-json, making it lighter and simpler.

However, it has only been tested on Debian-based systems. If you find it incompatible with your system, you can use the following methods to prepare OpenCV.

#### Method 2: Install libopencv-dev to the system

If you are only using PaddleOCR-json locally, you can directly install OpenCV development tools locally.

The installation process is simple, but if you want to transfer the built PaddleOCR-json to other devices for use, you need to manually collect the OpenCV dependency libraries in the system path.

```sh
sudo apt install libopencv-dev
```

#### Method 3: Compile OpenCV locally

You can refer to [OpenCV Official Documentation](https://docs.opencv.org/4.x/d7/d9f/tutorial_linux_install.html), or the following steps:

> [!TIP]
> The steps and compilation script are not necessarily compatible with all systems, for reference only.

1. In the `cpp/.source` directory, download OpenCV release v4.10.0 source code, unzip to get `opencv-4.10.0`:

```sh
wget -O opencv.zip https://github.com/opencv/opencv/archive/refs/tags/4.10.0.zip
unzip opencv.zip
ls -d opencv*/  # Check the directory name after unzipping
```

2. Call the one-click compilation script, passing the directory name after OpenCV source code unzipping:

```sh
../tools/linux_build_opencv.sh opencv-4.10.0
```

3. If compilation is successful, an `opencv-lib` directory will be generated in the `.source` directory.

<details>
<summary>Recommended compilation parameters and explanations</summary>
</br>

If you do not use the one-click compilation script `tools/linux_build_opencv.sh`, but compile manually, it is recommended to use the following parameters:

| Parameter                           | Description                                       |
| ----------------------------------- | ------------------------------------------------- |
| -DCMAKE_BUILD_TYPE=Release          |                                                   |
| -DBUILD_LIST=core,imgcodecs,imgproc | PPOCR only depends on these three modules         |
| -DBUILD_SHARED_LIBS=ON              |                                                   |
| -DBUILD_opencv_world=OFF            |                                                   |
| -DOPENCV_FORCE_3RDPARTY_BUILD=ON    | Force building all third-party libraries to avoid missing dependencies in some systems |
| -DWITH_ZLIB=ON                      | Image format codec support                        |
| -DWITH_TIFF=ON                      | Image format codec support                        |
| -DWITH_OPENJPEG=ON                  | Image format codec support                        |
| -DWITH_JASPER=ON                    | Image format codec support                        |
| -DWITH_JPEG=ON                      | Image format codec support                        |
| -DWITH_PNG=ON                       | Image format codec support                        |
| -DWITH_OPENEXR=ON                   | Image format codec support                        |
| -DWITH_WEBP=ON                      | Image format codec support                        |
| -DWITH_IPP=ON                       | Enable Intel CPU acceleration library             |
| -DWITH_LAPACK=ON                    | Enable mathematical operation acceleration library|
| -DWITH_EIGEN=ON                     | Enable mathematical operation acceleration library|
| -DBUILD_PERF_TESTS=OFF              | Close unnecessary test/documentation/language modules |
| -DBUILD_TESTS=OFF                   | Close unnecessary test/documentation/language modules |
| -DBUILD_DOCSL=OFF                   | Close unnecessary test/documentation/language modules |
| -DBUILD_JAVA=OFF                    | Close unnecessary test/documentation/language modules |
| -DBUILD_opencv_python2=OFF          | Close unnecessary test/documentation/language modules |
| -DBUILD_opencv_python3=OFF          | Close unnecessary test/documentation/language modules |

</details>


### 1.4 Check

After completion, it should be like this:
```
PaddleOCR-json
└─ cpp
    ├─ .source
    │    ├─ models
    │    └─ paddle_inference_manylinux_cpu_avx_mkl_gcc8.2
    ├─ CMakeLists.txt
    ├─ README.md
    ├─ docs
    ├─ external-cmake
    ├─ include
    └─ src
```

To facilitate the subsequent compilation of PaddleOCR-json itself, set the dependency library paths as environment variables:

```sh
export PADDLE_LIB="$(pwd)/$(ls -d *paddle_inference*/ | head -n1)"
export MODELS="$(pwd)/models"

# If using method 1 or 3 to prepare OpenCV, record the OpenCV path.
# If using method 2 to install libopencv-dev, no need to do this.
export OPENCV_DIR="$(pwd)/opencv-release"
```

You can use echo to check

```sh
echo $PADDLE_LIB
echo $MODELS
echo $OPENCV_DIR  # Optional
```

Go back to the `cpp` directory

```sh
cd ..
```

## 2. Build & Compile Project

0. If no customization of the project is needed, jump to [4. One-click compile + run](#compile-run)

1. Under `PaddleOCR-json/cpp`, create a new folder `build`

```sh
mkdir build
```

2. Use CMake to build the project. Parameter meanings see [CMake Build Parameters](#cmake-args)

```sh
cmake -S . -B build/ \
    -DPADDLE_LIB=$PADDLE_LIB \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENCV_DIR=$OPENCV_DIR  # Optional: OpenCV path
```

Explanation:
* `-S .` : Specify the current folder `PaddleOCR-json/cpp` as the CMake project root folder
* `-B build/` : Specify `PaddleOCR-json/cpp/build` folder as the project folder
* `-DPADDLE_LIB=$PADDLE_LIB` : Use the environment variable `$PADDLE_LIB` set earlier to specify the prediction library location
* `-DCMAKE_BUILD_TYPE=Release` : Set this project to `Release` project. You can also change it to `Debug`.
* `-DOPENCV_DIR=$OPENCV_DIR` : Use the environment variable `$OPENCV_DIR` set earlier to specify the self-compiled OpenCV location. If installing libopencv-dev, no need to set this parameter

3. Use CMake to compile the project

```sh
cmake --build build/
```

- Here we use the `--build build/` command to specify the project folder `build` to compile.

<a id="cmake-args"></a>

#### CMake Build Parameters

You can use `-DParameterName=Value` to add new CMake parameters.

The following are some compilation parameters:

| Parameter Name    | Description                                              |
| ----------------- | -------------------------------------------------------- |
| `WITH_MKL`        | Use MKL or OpenBlas, default use MKL.                    |
| `WITH_GPU`        | Use GPU or CPU, default use CPU.                         |
| `WITH_STATIC_LIB` | Compile into static library or shared library, default compile into static library. |
| `WITH_TENSORRT`   | Use TensorRT, default off.                               |

> [!NOTE]
> * `WITH_STATIC_LIB`: On Linux, this parameter cannot be compiled when set to `ON`, so it is forcibly set to `OFF`.

The following are some dependency library path related parameters. Except for `PADDLE_LIB` which is required, others depend on the situation.

| Parameter Name | Description                  |
| -------------- | ---------------------------- |
| `PADDLE_LIB`   | Path to paddle_inference     |
| `OPENCV_DIR`   | Path to library              |
| `CUDA_LIB`     | Path to library              |
| `CUDNN_LIB`    | Path to library              |
| `TENSORRT_DIR` | Use TensorRT compile and set its path |

> [!NOTE]
> * You can also set the OpenCV library path by setting the environment variable `OpenCV_DIR`, note that the variable name is case sensitive.
> * `OPENCV_DIR` or environment variable: On Linux, if already installed in the system, no need to specify.

The following are some PaddleOCR-json function related parameters.

| Parameter Name            | Description                              |
| ------------------------- | ---------------------------------------- |
| `ENABLE_CLIPBOARD`        | Enable clipboard function. Default off.  |
| `ENABLE_REMOTE_EXIT`      | Enable remote shutdown engine process command. Default on. |
| `ENABLE_JSON_IMAGE_PATH`  | Enable json command image_path. Default on. |

> [!NOTE]
> * `ENABLE_CLIPBOARD`: There is no clipboard function on Linux, even if enabled, it cannot be used.
> * `ENABLE_REMOTE_EXIT`: This parameter controls the "[pass `exit` to shutdown engine process](../docs/Detailed-Usage-Guide.md#4-shutdown-engine-process)" function.
> * `ENABLE_JSON_IMAGE_PATH`: This parameter controls the "use `{"image_path":""}` to specify path" function.

The following are some CMake function related parameters.

| Parameter Name        | Description                             |
| --------------------- | --------------------------------------- |
| `INSTALL_WITH_TOOLS`  | CMake install with tool files. Default on. |

#### About Clipboard Reading

On Linux, the function to read data from the clipboard does not exist. Even if `ENABLE_CLIPBOARD` is set to `ON`, it cannot be used.

#### Build or Compile Failed?

If the error contains `unable to access 'https://github.com/LDOUBLEV/AutoLog.git/': gnutls_handshake() failed: The TLS connection was non-properly terminated.`, the reason is network problem, please use global proxy. If no proxy, you can try changing the github address in `deploy/cpp_infer/external-cmake/auto-log.cmake` to `https://gitee.com/Double_V/AutoLog`.

Welcome to submit Issue.


## 3. Configure & Run Executable

1. At this step, you should be able to find an executable called `PaddleOCR-json` in the `build/bin/` folder

```sh
ls ./build/bin/PaddleOCR-json
```

2. Running directly will get an error like this

```sh
./build/bin/PaddleOCR-json
```

```
./build/bin/PaddleOCR-json: error while loading shared libraries: libiomp5.so: cannot open shared object file: No such file or directory
```

> [!NOTE]
> This is because the system cannot find the shared library `libiomp5.so` in the paths listed in the environment variable `LD_LIBRARY_PATH`.

3. Here we directly update the environment variable `LD_LIBRARY_PATH` to solve it.

```sh
# All prediction library shared libraries have been automatically copied to the "build/bin" folder, here we store it in a variable.
LIBS="$(pwd)/build/bin/"
LD_LIBRARY_PATH=$LIBS ./build/bin/PaddleOCR-json
```

> [!TIP]
> 
> [What is LD_LIBRARY_PATH?](https://developer.aliyun.com/article/1269445)
> 
> [Risks of using LD_LIBRARY_PATH?](https://m.ituring.com.cn/article/22101)

> [!NOTE]
> If you plan to use PaddleOCR-json for a long time, you can refer to the [Installation Chapter](#5-installation).

4. At this step, PaddleOCR-json can already run. But it will prompt you that the configuration file is missing. All the files we need are in the model library `module` folder prepared earlier.

```sh
LD_LIBRARY_PATH=$LIBS ./build/bin/PaddleOCR-json \
    -models_path="$MODELS" \
    -config_path="$MODELS/config_chinese.txt" \
    -image_path="/path/to/image.jpg" # Path to the image
```

> [!TIP]
> For more configuration parameters, please refer to [Simple Trial](../README.md#simple-trial) and [Common Configuration Parameters Explanation](../README.md#common-configuration-parameters-explanation)

5. If you want to package and transfer to other devices to run, you also need to [place OpenCV binary package](#13-prepare-opencv).

<a id="compile-run"></a>

## 4. One-click Compile + Run

We have prepared two simple scripts for one-click compile and run PaddleOCR-json.

> [!WARNING]
> Please note that these scripts below do not install PaddleOCR-json into your system. Not suitable for users who want long-term use. Just for developers' repeated compilation and testing

### 4.1 One-click Compile

After completing [Chapter 1 Preparation](#1-preparation), you can use the following script to directly build + compile the project.

```sh
./tools/linux_build.sh
```

### 4.2 One-click Run

After compilation is completed (after [completing Chapter 2](#2-build--compile-project)), you can use the following script to directly run PaddleOCR-json

```sh
./tools/linux_run.sh [configuration parameters]
```

* [Common Configuration Parameters](../README.md#common-configuration-parameters-explanation)

## 5. Installation

You can use CMake to install PaddleOCR-json into the system. Run the following command directly with `sudo` permission.

```sh
sudo cmake --install build
```

CMake will install the executable files and runtime libraries from the `build` folder to the system folder `/usr/`, so you can directly call this software with `PaddleOCR-json`.

If you want to install to a specified location, you can add the parameter `--prefix /installation/path/` to the above command to specify an installation path. For example, `--prefix build/install` will install all files to the `build/install` folder.

> [!TIP]
> When installing on Linux, CMake will additionally install some tool scripts and documents to facilitate direct use by users ([that is, the things in the `linux_dist_tools/` folder](./tools/linux_dist_tools/)). This function can help developers package software more conveniently. However, if you want to install PaddleOCR-json into the system folder, you do not need these tool files. You can disable the installation of these tool files by turning off the CMake parameter `INSTALL_WITH_TOOLS`.

> [!TIP]
> When CMake installs PaddleOCR-json, it will copy all shared dependency libraries in the `build/bin` folder to the `lib` folder of the installation directory. However, many shared libraries on Linux are split in the system folder (such as `/usr/lib/`). CMake cannot automatically find these shared dependency libraries. If you need to package PaddleOCR-json into a dependency-free software, you need to manually find the required shared dependency libraries from the system folder and copy them to the `build/bin` folder. This way, CMake can package the complete shared dependency libraries together during installation.

## 6. Other Issues

### [Switch Language/Model Library/Preset](./README.md#5.-switch-language-model-library-preset)

### [About Memory Usage](./README.md#about-memory-usage)
