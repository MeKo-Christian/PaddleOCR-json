#### Offline OCR Component Series Projects:
- **PaddleOCR-json**
- [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json)

|                  | PaddleOCR-json                                  | RapidOCR-json        |
| ---------------- | ----------------------------------------------- | -------------------- |
| CPU Requirements | CPU must have AVX instruction set. Does not support the following CPUs: | No special requirements 👍 |
|                  | Atom, Itanium, Celeron, Pentium                 |                      |
| Inference Acceleration Library | mkldnn 👍                                        | None                 |
| Recognition Speed | Fast (with mkldnn acceleration) 👍               | Medium               |
| Initialization Time | About 0.6s                                       | Within 0.1s, fast 👍 |
| Component Size (Compressed) | 100MB                                           | 70MB 👍              |
| Component Size (Deployed) | 369MB                                           | 80MB 👍              |
| CPU Usage         | High                                             | Low, friendly to low-end machines |
| Recommended Reserved Memory | 2000MB                                          | 800MB 👍             |

---

# PaddleOCR-json

> Supports: **Win7 x64**, **Linux x64**, [Docker](cpp/README-docker.md)

This is an offline image OCR text recognition program based on [PaddleOCR v2.6](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.6) and [v2.8](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.8) cpp_infer, which can quickly give your program OCR capabilities. It can be called as a subprocess by upper-level programs or as a separate process via TCP. This project provides APIs in languages like Python, so you can use it with two lines of code without worrying about technical details.

This project aims to provide a packaged OCR engine component so that developers without C++ programming basics can easily call OCR in other languages, enjoying faster runtime efficiency and more convenient packaging & deployment methods.

- **Convenient**: Easy to deploy, unzip and use, no need to install and configure environment, no need to connect to the internet. Easy to publish, can be embedded in program packages or as external components.
- **Fast**: Based on PPOCR C++ engine, recognition efficiency is higher than Python version PPOCR and other OCR engines that handle task flow in Python.
- **Accurate**: Comes with PPOCR-v3 / v4 recognition library, has good recognition rate for irregular fonts (handwriting, artistic fonts, small fonts, cluttered backgrounds, etc.).
- **Flexible**: Can specify OCR tasks in multiple ways, supports recognizing local image paths, Base64 encoded images, TCP LAN calls.

**Application: [Umi-OCR Batch Image to Text Tool](https://github.com/hiroi-sora/Umi-OCR)**

## Compatibility

- System: x86-64 Windows 7+, Linux.
- If Win7 reports missing VCOMP140.DLL, please install [VC Runtime Library](https://aka.ms/vs/17/release/vc_redist.x64.exe).
- CPU must have AVX instruction set. Common home CPUs generally meet this condition.

    | AVX   | Supported Product Series                          | Not Supported                                      |
    | ----- | ------------------------------------------------- | -------------------------------------------------- |
    | Intel | Core, Xeon                                        | Atom, Itanium, Celeron, Pentium                    |
    | AMD   | Bulldozer architecture and later, like Ryzen, Athlon, FX | K10 architecture and earlier                      |
- If you need to use OCR on CPUs without AVX, check out the neighbor [RapidOCR-json](https://github.com/hiroi-sora/RapidOCR-json).


## Preparation

Download the executable package:

- https://github.com/hiroi-sora/PaddleOCR-json/releases/latest

### Simple Trial

`PaddleOCR-json.exe -image_path="test.jpg"`

## Calling via API

The calling process is roughly divided into the following steps. Different APIs may have slight differences in specific interfaces.

- Start: Start and initialize the engine subprocess.
- Work: Call the image recognition interface, get the return value. Currently supports recognizing **local image files**, **images in clipboard**, **Base64 encoded images**.
- Close: End the engine process, release memory resources.

## API List

There are more detailed usage instructions and demos in the `Resource Directory`.

### 1. Python API

[Resource Directory](api/python)

<details>
<summary>Usage Example</summary>

```python
from PPOCR_api import GetOcrApi

# Initialize the recognizer object, pass the path to PaddleOCR_json.exe
ocr = GetOcrApi("……\PaddleOCR-json.exe")

# Recognize image, pass image path
getObj = ocr.run(r'………\test.png')
print(f'Image recognition completed, status code: {getObj["code"]} Result:\n{getObj["data"]}\n')
```

Python API has rich additional modules: visualization modules for developers to debug and observe; and text block post-processing (paragraph merging) technology from [Umi-OCR](https://github.com/hiroi-sora/Umi-OCR). Detailed usage see [Resource Directory](api/python)

</details>

### 2. Node.js API

[Resource Directory](api/node.js)

<details>
<summary>Usage Example</summary>

```
npm install paddleocrjson
```

```js
const OCR = require('paddleocrjson');

// const OCR = require('paddleocrjson/es5'); // ES5

const ocr = new OCR('PaddleOCR-json.exe', [/* '-port=9985', '-addr=loopback' */], {
    cwd: './PaddleOCR-json',
}, false);

ocr.flush({ image_path: 'path/to/test/img' })
    .then((data) => console.log(data));
    .then(() => ocr.terminate());
```

</details>

### 3. PowerShell API

[Resource Directory](api/PowerShell)

### 4. Java API

[Resource Directory](https://github.com/jerrylususu/PaddleOCR-json-java-api)

### 5. .NET API

[Resource Directory](https://github.com/aki-0929/PaddleOCRJson.NET)


### 6. Rust API

[Resource Directory](https://github.com/OverflowCat/paddleocr)

### 7. Go API

[Resource Directory](https://github.com/doraemonkeys/paddleocr)

### More Language APIs

Welcome to add! Please refer to [Detailed Usage Guide](docs/Detailed Usage Guide.md).


## Common Configuration Parameters Explanation

| Key Name        | Default Value | Value Explanation                                                                 |
| -------------- | ------------- | -------------------------------------------------------------------------------- |
| ensure_ascii   | true          | Enable ascii encoding conversion, output unicode characters in ascii encoding (pure English numbers), like `你好`→`\u4f60\u597d`. |
|                |               | In general, json decoders will automatically translate ascii codes back to original characters. This option is recommended to be enabled to improve encoding compatibility. |
| config_path    | ""            | Can specify different language configuration file paths, recognize multiple languages. [Details see next section](#language-library-and-switching-recognition-language). |
| models_path    | ""            | Can specify the path to the language library `models` folder. [Details see next section](#language-library-and-switching-recognition-language). |
| det            | true          | Enable det target recognition. If your image contains only one line of text and no blank areas, you can turn off det to speed up. |
| cls            | false         | Enable cls direction classification, recognize images whose direction is not facing up. |
| use_angle_cls  | false         | Enable direction classification, must be set with cls. |
| enable_mkldnn  | true          | Enable CPU inference acceleration, turning it off can reduce memory usage, but will slow down speed. |
| limit_side_len | 960           | Limit the image side length, reduce resolution, speed up. If the recognition rate for large/long images is low, you can increase this option's value. |
|                |               | Suggested to be a common multiple of 32 & 48, like 960, 2880, 4320 |

More parameters see [args.cpp](/cpp/src/args.cpp). (Does not support GPU-related, table recognition-related parameters. -)

### Language Library and Switching Recognition Language:

The Release compressed package comes with language libraries and configuration files for `Simplified Chinese, Traditional Chinese, English, Japanese, Korean` by default, in the `models` directory.

In the `models` directory, each `config_xxx.txt` is a set of language configuration files (e.g., English is `config_en.txt`). Just pass this file's path to the `config_path` parameter to switch to the corresponding language. For example, with Python API:

```python
enginePath = "D:/Test/PaddleOCR_json.exe"  # Engine path
argument = {"config_path": "models/config_en.txt"}  # Specify use English library
ocr = GetOcrApi(enginePath, argument)
```

If config_path is left empty, PaddleOCR-json defaults to loading and using the Simplified Chinese recognition library.

However, when using the default path or setting `config_path` separately, the PaddleOCR-json executable must be in the same directory as the language library. For example:

```
.
├─ PaddleOCR-json.exe
└─ models
    ├─ ...
```

If the language library is in another folder, PaddleOCR-json cannot find the language library.

In this case, you can use the `models_path` parameter to set the language library location. PaddleOCR-json will use the user-set language library location as the base to load other files.

This way, even if PaddleOCR-json and the language library are not in the same directory, it can be used normally. For example, with Python API:

```python
enginePath = "D:/Test/PaddleOCR_json.exe"  # Engine path
modelsPath = "D:/Hello/models"             # Language library path
# The order of parameters here does not affect the result
argument = {
  # Specify language library location
  "models_path": "D:/Hello/models",
  # Specify use English library
  "config_path": "D:/Hello/models/config_en.txt",
}
ocr = GetOcrApi(enginePath, argument)
```

This project supports PP-OCR series official V2~V4 models, or self-trained models that conform to PP specifications. More PP-OCR series official model downloads: https://github.com/PaddlePaddle/PaddleOCR/blob/main/doc/doc_ch/models_list.md

#### Deleting Language Libraries:

If you want to delete unused language library files to reduce software size, you can delete folders in the `models` directory that contain the corresponding language prefix and **rec_infer** suffix. For example, if you want to delete Japanese `japan` related libraries, just delete this folder:
`japan_PP-OCRv3_rec_infer`

A set of language rec library takes about 10MB space (uncompressed). If you delete to only 1 set of language, you can save about 60MB space.

Please do not delete cls_infer and det_infer suffix folders, these are detection/direction classification libraries shared by all languages.


## Return Value Explanation

Calling OCR once via API, whether successful or not, will return a dictionary.

In the dictionary, the root contains two elements: status code `code` and content `data`.

Status code `code` is an integer, each status code corresponds to a situation:

##### `100` Recognized Text

- data content is an array. Each item in the array is a dictionary containing three fixed elements:
  - `text`: Text content, string.
  - `box`: Text bounding box, array of length 4, respectively top-left, top-right, bottom-right, bottom-left `[x,y]`. Integer.
  - `score`: Recognition confidence, float from 0~1. Closer to **1** means the text content is more credible.
- (v1.4.0 new) If `cls` and `use_angle_cls` are enabled, there will be two more elements:
  - `cls_label`: Direction classification label, integer. **0** means text direction is clockwise 0° or 90°, **1** means 180° or 270°.
  - `cls_score`: Direction classification confidence, float from 0~1. Closer to **1** means direction classification is more credible.
- Example:
  ```
    {'code':100,'data':[{'box':[[13,5],[161,5],[161,27],[13,27]],'score':0.9996442794799805,'text':'Flying Causal Communication'}]}
  ```

##### `101` No Text Recognized

- data is string: `No text found in image. Path:"image path"`
- Example: ```{'code':101,'data':'No text found in image. Path: "D:\\blank.png"'}```
- This is normal, this result appears when recognizing blank images without text.

##### `200` Image Path Does Not Exist

- data: `Image path dose not exist. Path:"image path".`
- Example: `{'code':200,'data':'Image path dose not exist. Path: "D:\\notexist.png"'}`
- Note, when the system does not have utf-8 support enabled (`Use Unicode UTF-8 for worldwide language support`), cannot read paths with emoji and other special characters (like `😀.png`). But general Chinese and other Unicode character paths are fine, not affected by system region and default encoding.

##### `201` Image Path String Cannot Convert to wstring

- data: `Image path failed to convert to utf-16 wstring. Path: "image path".`
- When using API, theoretically won't report this error.
- When developing API, if the passed string encoding is illegal, may report this error.

##### `202` Image Path Exists, But Cannot Open File

- data: `Image open failed. Path: "image path".`
- May be caused by system permissions, etc.

##### `203` Image Opened Successfully, But Content Read Cannot Be Decoded by OpenCV

- data: `Image decode failed. Path: "image path".`
- Note, the engine does not distinguish various images by file extension, but for existing paths, all read bytes and try to decode. If the passed file path is not an image, or the image is damaged, this error will be reported.
- Conversely, changing the suffix of a normal image to something else (like `.png` to `.jpg or .exe`), can also be recognized normally.

<details>
<summary>
<strong>Clipboard related interfaces are deprecated, not recommended to use</strong>
</summary>

##### `210` Clipboard Open Failed

- data: `Clipboard open failed.`
- May be caused by other programs occupying the clipboard, etc.

##### `211` Clipboard Is Empty

- data: `Clipboard is empty.`

##### `212` Clipboard Format Not Supported

- data: `Clipboard format is not valid.`
- The engine can only recognize bitmaps or files in the clipboard. If not these two formats (like copying a piece of text), this error will be reported.

##### `213` Clipboard Get Content Handle Failed

- data: `Getting clipboard data handle failed.`
- May be caused by other programs occupying the clipboard, etc.

##### `214` Number of Files Queried by Clipboard Is Not 1

- data: `Clipboard number of query files is not valid. Number: number of files`
- Only allows copying one file at a time. Copying multiple files at once and then calling OCR will get this error.

##### `215` Clipboard Retrieve Graphic Object Information Failed

- data: `Clipboard get bitmap object failed.`
- Clipboard is bitmap, but getting bitmap information failed. May be caused by other programs occupying the clipboard, etc.

##### `216` Clipboard Get Bitmap Data Failed

- data: `Getting clipboard bitmap bits failed.`
- Clipboard is bitmap, getting bitmap information succeeded, but reading into buffer failed. May be caused by other programs occupying the clipboard, etc.

##### `217` Number of Channels in Clipboard Bitmap Not Supported

- data: `Clipboard number of image channels is not valid. Number: number of channels`
- The engine only allows reading images with channels 1 (black and white), 3 (RGB), 4 (RGBA). If bitmap channels are not 1, 3, or 4, this error will be reported.

</details>

##### `300` Base64 String Parse to String Failed

- data: `Base64 decode failed.`
- Caused by passing illegal Base64 string. (Note, the passed Base64 information should not have `data:image/jpg;base64,` prefix.)

##### `301` Base64 String Parse Successful, But Content Read Cannot Be Decoded by OpenCV

- data: `Base64 data imdecode failed.`

##### `400` Json Object to String Failed

- data: `Json dump failed.CODE_ERR_JSON_DUMP`
- Input exception: Caused by passing illegal json string, or string containing non-utf-8 encoded characters that cannot be parsed.

##### `401` Json String to Object Failed

- data: `Json dump failed.CODE_ERR_JSON_DUMP`
- Output exception: OCR result cannot be encoded to json string when outputting.

##### `402` Json Object Parse Certain Key Failed

- data: `Json parse key key name failed.`
- More precise prompt than error code `400`. If exception occurs, program prioritizes reporting `402`, if cannot handle then reports `400`.

##### `403` No Valid Tasks Found

- data: `No valid tasks.`
- The instruction passed this time does not contain valid tasks.


### [Detailed Usage Guide](docs/Detailed Usage Guide.md)

👆Welcome to refer when you need to modify or develop new API.


### Project Build Guide

#### Stable Version, Based on PP-OCR v2.6

- [Windows Platform Build Steps](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README.md)
- [Linux Platform Build Steps](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README-linux.md)
- [Docker Deployment](https://github.com/hiroi-sora/PaddleOCR-json/blob/release/1.4.1/cpp/README-docker.md)

#### Development Version, Based on PP-OCR v2.8

> Note: This version is based on Paddle Inference 3.0.0 inference backend, better performance on high-end CPUs with AVX512 instruction set. Ordinary home CPUs have performance degradation, recommend using the stable version above.

- [Windows Platform Build Steps](cpp/README.md)
- [Linux Platform Build Steps](cpp/README-linux.md)
- [Docker Deployment](cpp/README-docker.md)
- [Porting Guide](cpp/docs/Porting Guide.md) (Can be referred to when porting the project to different platforms)

### Thanks

This project uses [ReneNyffenegger/cpp-base64](https://github.com/ReneNyffenegger/cpp-base64):
> "base64 encoding and decoding with c++"

This project uses [nlohmann/json](https://github.com/nlohmann/json):
> "JSON for Modern C++"

Thanks to [PaddlePaddle/PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR), without it there would be no this project:
> "Awesome multilingual OCR toolkits based on PaddlePaddle"

Thanks to friends who developed APIs and contributed code for this project!

## Update Log

Version number links can go to corresponding backup branches.

#### [v1.4.1](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.4.1) `2024.8.28`

- Inference backend Paddle Inference due to instability of `3.0.0`, continue using old version inference library `2.3.2`.
- Fix: Language library `Traditional Chinese` configuration file incorrect problem.
- Recompile Linux release:
  - `glibc` dependency library adjusted down to `2.31` version, compatible with debian11, ubuntu20.04 and other old systems.

#### Test: v1.4.1 dev 1 `2024.7.31`

- Update inference backend to Paddle Inference `3.0.0 beta-1`.
- Significantly optimize memory usage: Peak from 2.5GB down to about 1.5GB.
- Add command line parameter: Memory auto-clean threshold `--cpu_mem`. See [document](cpp/README.md#about memory usage).
- Slightly optimize initialization time.
- Support `PP-OCR V4` series model libraries, and PPOCR algorithm competition [champion solution model libraries](https://github.com/PaddlePaddle/PaddleOCR/blob/main/doc/doc_ch/algorithm_rec_svtrv2.md).
- Due to backend dependency library update, on **non-AVX512** CPUs, OCR speed may have **slight decrease**.
- Due to language library `cyrillic` (Slavic letters/Russian) low accuracy, low usage frequency, no longer included in release package. Users who need can [download themselves](https://paddleocr.bj.bcebos.com/PP-OCRv3/multilingual/cyrillic_PP-OCRv3_rec_infer.tar).
- Python API: Fixed the problem that boolean type startup parameters set to `False` do not take effect.

#### [v1.4.0](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.4.0) `2024.7.22`

#### v1.4.0 beta 2 `2024.7.9`
- Return value new: Text direction classification related parameters.

#### v1.4.0 beta `2024.7.5`
- Compatible with Linux.
- Adjustment: Default disable clipboard image recognition function, need to compile to enable yourself.

#### v1.3.1 `2023.10.10`
- Compatible with Win7 x64.

#### [v1.3.0](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.3.0) `2023.6.19`
- Fixed some BUGs.

#### v1.3.0 alpha `2023.5.26`
- Refactor code, clearer structure, easier to port.
- New feature: Base64 pass image.
- New feature: Socket server mode.

#### [v1.2.1](https://github.com/hiroi-sora/PaddleOCR-json/tree/backups/1.2.1/new_builds) `2022.9.28`
- Fixed some BUGs.
- Solved the problem that non-Chinese windows are difficult to read Chinese paths, embrace utf-8, completely get rid of dependence on gbk and other regional encodings.
- New feature: Directly read and recognize images in clipboard memory.
- Error codes and prompts are more detailed.

#### [v1.2.0](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.2.0) `2022.8.29`
- Fixed some BUGs.
- Enhanced robustness when facing illegal encoding.
- Default enable mkldnn acceleration.
- New feature: json input and hot update.

#### v1.2.0 beta `2022.8.26`
- Refactor the entire project, core code sync PaddleOCR 2.6.
- Better support for v3 version recognition library.
- New feature: Startup parameters.
- New feature: ascii escape. (Thanks to @AutumnSun1996's suggestion [issue #4](https://github.com/hiroi-sora/PaddleOCR-json/issues/4))

#### [v1.1.1](https://github.com/hiroi-sora/PaddleOCR-json/tree/release/1.1.1) `2022.4.16`
- Corrected vulnerability: When `text detection` detects an area but `text recognition` does not detect text in the area, may output inconsistent bounding boxes.

#### v1.1.0 `2022.4.2`
- Modified json output format, changed to status code + content, convenient for caller to judge.

#### v1.0 `2022.3.28`
