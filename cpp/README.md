# PaddleOCR-json V1.4 Windows Build Guide

> Note: This development version is based on Paddle Inference 3.0.0 inference backend, and there is a significant performance degradation issue on ordinary home CPUs without AVX512 instruction set. Ordinary users are recommended to switch to the stable branch of this project.

This document helps how to compile PaddleOCR-json V1.4 (corresponding to PPOCR v2.8) on Windows. For beginners, using the simplest steps. Experts can adjust as appropriate.

This article refers to PPOCR official's [compilation guide](https://github.com/PaddlePaddle/PaddleOCR/blob/release/2.8/deploy/cpp_infer/docs/windows_vs2019_build.md), but it is recommended to follow this article.

Related documents:
- [Linux Build Guide](./README-linux.md)
- [Docker Deployment](./README-docker.md#using-docker-deployment)
- Other platforms [Porting Guide](docs/Migration-Guide.md)


## 1. Preparation

Resource links followed by (version in parentheses), please check carefully.

### 1.1 Tools to Install:

- [Visual Studio 2022](https://learn.microsoft.com/zh-cn/visualstudio/releases/2022/release-notes) (2022 or 2019 both ok, Community)
- [Cmake](https://cmake.org/download/) (Windows x64 Installer)
- [Git](https://git-scm.com/download/win) (64-bit Git for Windows Setup)

### 1.2 Resources to Download:

- [paddle_inference.zip](https://paddle-inference-lib.bj.bcebos.com/3.0.0-beta1/cxx_c/Windows/CPU/x86-64_avx-mkl-vs2019/paddle_inference.zip) (Windows C++ prediction library, 3.0.0, cpu_avx_mkl)
- [Opencv](https://github.com/opencv/opencv/releases) (windows.exe)
- [Model library](https://github.com/hiroi-sora/PaddleOCR-json/releases/download/v1.4.1-dev/models_v1.4.1.zip) (models.zip)

### 1.3 Place Resources

1. Clone this repository. Under `PaddleOCR-json/cpp`, create a new folder `.source` to store external resources. (The dot in front is for better file name sorting)
2. Unzip the downloaded `models_v1.4.1.zip`, `paddle_inference` and `Opencv` into `.source`.
   - `paddle_inference` should be unzipped into a separate folder, and rename the folder with a suffix according to the version, for example if it's cpu_avx_mkl version, call it `paddle_inference_cpu_avx_mkl`, to distinguish.
   - Opencv looks like an exe, but it's actually a self-extracting package, run it and choose the directory to extract.

After completion, it should be like this:
```
PaddleOCR-json
└─ cpp
    ├─ .source
    │    ├─ opencv
    │    ├─ models
    │    └─ paddle_inference_cpu_avx_mkl
    ├─ CMakeLists.txt
    ├─ README.md
    ├─ docs
    ├─ external-cmake
    ├─ include
    └─ src
```

## 2. Build Project

> **Note:** This project uses a `justfile` to simplify the build process. For a simpler build experience, please refer to the root [README.md](../../README.md) for instructions on using `just`.

1. After cmake installation, there will be a cmake-gui program in the system, open cmake-gui. Fill in the source code path in the first input box, and the compilation output path in the second input box, see the template below.  
Then, click the first button in the lower left corner Configure, the first time you click it will pop up a prompt box for Visual Studio configuration, select your Visual Studio version (2019, 2022 both ok), target platform select x64. Then click the finish button to start automatic configuration.

Where is the source code: `……/PaddleOCR-json/cpp`

Where to build the binaries: `……/PaddleOCR-json/cpp/build`

![](docs/imgs/b1.png)

After execution, it will report an error, which is normal, click OK.
![](docs/imgs/b2.png)

2. Fill in two configurations:

OPENCV_DIR:  
`……/PaddleOCR-json/cpp/.source/opencv/build/x64/vc16/lib`

PADDLE_LIB:  
`……/PaddleOCR-json/cpp/.source/paddle_inference_cpu_avx_mkl`

Make sure **NOT** to check `WITH_GPU` below.

Don't touch other items!

![](docs/imgs/b3.png)

Click the **first button Configure** in the lower left corner to apply the configuration, wait a few seconds, see the output `Configuring done` is ok.

Click the **second button Generate** in the lower left corner to generate the Visual Studio project sln file. See the output `Generating done` is ok. Then, you will see `PaddleOCR-json.sln` and a bunch of files generated under `PaddleOCR-json/cpp/build`.

#### CMake Build Parameters Explanation

Like the CMake options we checked/filled in earlier (`WITH_GPU`, `OPENCV_DIR`, `PADDLE_LIB`), they are CMake parameters. You can also refer to and modify these parameters yourself.

The following are some compilation parameters:

| Parameter Name    | Description                                                                                                                             |
| ----------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| `WITH_MKL`        | Use MKL or OpenBlas, default use MKL.                                                                                                    |
| `WITH_GPU`        | Use GPU or CPU, default use CPU.                                                                                                         |
| `WITH_STATIC_LIB` | Compile into static library or shared library, default compile into static library. (On Linux, this parameter cannot be compiled when set to `ON`, so it is forcibly set to `OFF`.) |
| `WITH_TENSORRT`   | Use TensorRT, default off.                                                                                                               |

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

The following are some PaddleOCR-json function related parameters.

| Parameter Name            | Description                              |
| ------------------------- | ---------------------------------------- |
| `ENABLE_CLIPBOARD`        | Enable clipboard function. Default off.  |
| `ENABLE_REMOTE_EXIT`      | Enable remote shutdown engine process command. Default on. |
| `ENABLE_JSON_IMAGE_PATH`  | Enable json command image_path. Default on. |

> [!NOTE]
> * `ENABLE_REMOTE_EXIT`: This parameter controls the "[pass `exit` to shutdown engine process](../docs/Detailed-Usage-Guide.md#4-shutdown-engine-process)" function.
> * `ENABLE_JSON_IMAGE_PATH`: This parameter controls the "use `{"image_path":""}` to specify path" function.

#### About Clipboard Reading

On Windows, the function to read data from the clipboard exists, but has been deprecated. All clipboard-related code is not compiled by default. If you need PaddleOCR-json to read the clipboard, please modify the CMake parameter `ENABLE_CLIPBOARD=ON` and recompile yourself.

#### Build Failed?

If the error contains `Could NOT find Git (missing: GIT_EXECUTABLE)`, the reason is that Git is not installed on the computer, please install it first (preferably in the default directory).

If the error contains `unable to access 'https://github.com/LDOUBLEV/AutoLog.git/': gnutls_handshake() failed: The TLS connection was non-properly terminated.`, the reason is network problem, please use global proxy. If no proxy, you can try changing the github address in `deploy/cpp_infer/external-cmake/auto-log.cmake` to `https://gitee.com/Double_V/AutoLog`.

For other reasons, please confirm that your operation steps are consistent with this article, especially the order of clicking buttons.

## 3. Configure Project

1. Go back to the build folder under the project directory, open `PaddleOCR-json.sln`. **Change Debug to Release**.

![](docs/imgs/b5.png)

2. Adjust project settings.

- Solution Explorer → **ALL_BUILD** → Right-click → Properties, modify:
  - General → Output Directory: `$(ProjectDir)\bin\Release`
  - Debugging → Command: `$(ProjectDir)\bin\Release\PaddleOCR-json.exe`
  - Debugging → Working Directory: `$(ProjectDir)\bin\Release`
- Solution Explorer → **PaddleOCR-json** → Right-click → Properties, modify:
  - General → Output Directory: `$(ProjectDir)\bin\Release`

![](docs/imgs/b6.png)

3. Press F5 to compile. If the output is similar to `Build: 4 succeeded, 0 failed……`, and then a console window pops up and reports an error `Cannot find opencv_world***.dll`, then **compilation is normal**. You can find the generated `PaddleOCR-json.exe` in `build/bin/Release`.

![](docs/imgs/b7.png)

> [!TIP]
> If during compilation, a lot of syntax errors are reported, such as:
> ```
> C2447 "{" missing function header
> C2059 syntax error: "if"
> C2143 syntax error: missing ";"
> …………etc
> ```
> Then it may be a problem with the line break encoding of the source code files.  
> Solution 1: Download the repository code through `git clone`, do not directly download the zip package from Github.  
> Solution 2: Batch convert the line breaks of all `.h` and `.cpp` files to [CRLF](https://www.bing.com/search?q=%E6%89%B9%E9%87%8F%E8%BD%AC%E6%8D%A2+LF+%E5%92%8C+CRLF).

4. Copy the necessary runtime libraries. Copy the following files to the `build/bin/Release` folder.

- `opencv_world***.dll`: `PaddleOCR-json/cpp/.source/opencv/build/x64/vc16/bin/opencv_world***.dll`

> Of course, you can also directly put the `opencv` runtime library into the Windows `PATH` environment variable. Refer to [this document](https://cloud.baidu.com/article/3297806), add the path `opencv/build/x64/vc16/bin/` to `PATH`. This way, you don't need to copy the `opencv` runtime library.

5. Copy the model library. Copy the entire `models` from `.source` to the `build/bin/Release` folder.

6. In `build/bin/Release`, Shift+Right-click, open terminal (or PowerShell) here, enter `./PaddleOCR-json.exe`. If the following text is output, it's normal.

```
OCR anonymous pipe mode.
OCR init time: 0.62887s
OCR init completed.
```

If you need to port to other platforms, you can refer to the document [Porting Guide](docs/Migration-Guide.md)

## 4. Installation

You can use CMake to install PaddleOCR-json. Under `cpp`, Shift+Right-click, open terminal (or PowerShell) here, run the following command.

```sh
cmake --install build
```

CMake will install the executable files and runtime libraries from the `build` folder to the `build/install/bin` folder. CMake cannot install software to the system folder on Windows, but you can add the folder `cpp/build/install/bin` to the Windows `PATH` environment variable. Refer to [this document](https://cloud.baidu.com/article/3297806).

If you want to install to a specified location, you can add the parameter `--prefix /installation/path/` to the above command to specify an installation path. For example, `--prefix build/install` will install all files to the `build/install` folder.

## 5. Switch Language/Model Library/Preset

You can specify the model library or preset through command line parameters at startup.

| Parameter           | Description                                                                 |
| ------------------- | --------------------------------------------------------------------------- |
| det_model_dir       | Text detection model library path. All languages can use `models/ch_PP-OCRv4_det_infer` |
| cls_model_dir       | Direction classification model library path. All languages can use `models/ch_ppocr_mobile_v2.0_cls_infer` |
| rec_model_dir       | Text recognition model library path. Different languages should use different recognition libraries. |
| rec_char_dict_path  | Dictionary file path corresponding to the text recognition model library. |
| rec_img_h           | Special parameter for text recognition model. V2 model needs to be manually set to 32, V3/V4 model does not need to be set. |

`det`, `cls`, `rec` support using PP-OCR series official models, or self-trained models that conform to PP specifications. Support V2~V4 models.

It is recommended to fill in relative paths, with PaddleOCR-json directory as root. Assuming all models are stored in the `models` directory, then you can set parameters like this:

(On Linux, replace `PaddleOCR-json.exe` with `run.sh`)

```sh
PaddleOCR-json.exe \
    --det_model_dir=models/ch_PP-OCRv4_det_infer \
    --cls_model_dir=models/ch_ppocr_mobile_v2.0_cls_infer \
    --rec_model_dir=models/ch_PP-OCRv4_rec_infer \
    --rec_char_dict_path=models/dict_chinese.txt
```

You can also write the above parameters into a txt, format like: `config_chinese.txt`

```sh
# This is a single line comment

# det detection model library
det_model_dir models/ch_PP-OCRv4_det_infer
# cls direction classifier library
cls_model_dir models/ch_ppocr_mobile_v2.0_cls_infer
# rec recognition model library
rec_model_dir models/ch_PP-OCRv4_rec_infer
# dictionary path
rec_char_dict_path models/dict_chinese.txt
```

Then, pass this txt into the `config_path` parameter:

```sh
PaddleOCR-json.exe --config_path=models/config_chinese.txt
```

The `model.zip` we provide already contains preset files for multiple languages:

| Language  | Preset File                    | Model Version |
| --------- | ------------------------------ | ------------- |
| Simplified Chinese | `models/config_chinese.txt`    | V4            |
| English   | `models/config_en.txt`         | V4            |
| Traditional Chinese | `models/config_chinese_cht.txt`| V2            |
| 日本語    | `models/config_japan.txt`      | V4            |
| 한국어    | `models/config_korean.txt`     | V4            |

(Note: Traditional Chinese uses V2 version model because V2 vertical recognition accuracy is better than subsequent versions.)

More PP-OCR series official model downloads: https://github.com/PaddlePaddle/PaddleOCR/blob/main/doc/doc_ch/models_list.md

## 6. Other Issues

### About Memory Usage

The backend inference library of this project is `Paddle Inference 3.0.0 beta.1 cpu_mkl`. Compared to the old version, `3.0.0` greatly optimizes memory usage. After large batch, continuous OCR (about 1000 images), memory usage stabilizes at around 1.5G.

It is recommended to use this project on machines with at least 4G memory, reserve 2G free memory for this project.

If necessary to use this project on 2G memory machines, it is recommended to add startup parameter `--cpu_mem=1200`, let PaddleOCR-json perform automatic memory cleanup when memory usage exceeds 1200MB.

- `--cpu_mem` is a new parameter added in PaddleOCR-json v1.4.1, v1.4.0 or earlier versions cannot use it.
- `--cpu_mem` is not recommended to be lower than `1000`, otherwise frequent memory cleanup may affect OCR efficiency. You can also use the neighboring [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json), which is more friendly to low-configuration machines.

In addition, this project does not automatically release memory when idle by default. Assuming it currently occupies 1500MB, then even if no OCR is performed next, the engine will always maintain this memory usage until it reaches the upper limit set by `--cpu_mem`, and automatically performs cleanup.

If you want the engine to not occupy so much memory when idle, feasible methods are:

1. External restart engine. Use a program/script to manage the engine's input and output. If there is no activity for a period of time, kill the engine process and restart one. Umi-OCR does this.
2. Clean up memory from inside the engine. In the branch `release/1.4.0_autoclean` there is a modified engine. A new parameter `--auto_memory_cleanup` is added, which will start a thread to check the engine status, and release memory when it is idle.

> [!CAUTION]
> **Whether it is `--auto_memory_cleanup` or `--cpu_mem` performing a cleanup, memory usage will still remain about 300~600MB. This is the lower limit of Paddle Inference inference library instance occupancy. If you want close to 0 occupancy when idle, you can only use method 1, external restart engine process.**

More details please see these Issues: [#43](https://github.com/hiroi-sora/PaddleOCR-json/issues/43), [#90](https://github.com/hiroi-sora/PaddleOCR-json/issues/90), [#135](https://github.com/hiroi-sora/PaddleOCR-json/issues/135)

> If you plan to use method 2 mentioned above, please pull and switch to the `release/1.4.0_autoclean` branch:  
> ```sh
> git fetch origin release/1.4.0_autoclean
> git checkout -b release/1.4.0_autoclean origin/release/1.4.0_autoclean
> ```
