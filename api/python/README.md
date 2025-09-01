# PaddleOCR-json Python API

This API allows you to easily call PaddleOCR-json. Compared to the native Python PaddleOCR library, PaddleOCR-json has better performance. You can enjoy both the high efficiency of the C++ inference library and the ease of Python development.

First, download the OCR engine binary from the project's [Releases](https://github.com/hiroi-sora/PaddleOCR-json/releases), then download the [python api](https://github.com/hiroi-sora/PaddleOCR-json/tree/main/api/python) (all files in the current directory) to your local machine, and you can call the binary through the Python interface.

The Python API has three major modules:

- Basic OCR interface
- Result visualization module, which draws OCR results onto images and displays or saves them.
- Text post-processing module, supporting paragraph merging, vertical text organization, etc.

## Basic OCR Interface

```python
from PPOCR_api import GetOcrApi
```

### The process of calling OCR is divided into three steps

1. Initialize the OCR engine process
2. Execute one or more image recognition tasks through the OCR engine
3. Close the OCR engine process

### Step 1: Initialization

**Interface:** `GetOcrApi()`

**Parameters:**

| Name       | Default | Type | Description                                                           |
| ---------- | ------- | ---- | --------------------------------------------------------------------- |
| exePath    | Required | str  | Path to the engine binary file, or remote server address, see below. |
| modelsPath | None    | str  | Recognition library path, if None, the recognition library is assumed to be in the same directory as the engine. |
| argument   | None    | dict | Startup parameter dictionary. Use this to specify config files, recognition language. |
| ipcMode    | "pipe"  | str  | Inter-process communication method, options are socket mode `socket` or pipe mode `pipe`. |

#### About `exePath`

Currently allows two modes to call the engine:

1. Engine deployed locally:

   - Download the OCR engine binary from [Releases](https://github.com/hiroi-sora/PaddleOCR-json/releases) to local, unzip.
   - Windows platform: `exePath` passes the path to `PaddleOCR-json.exe`.
   - Linux platform: `exePath` passes the path to `run.sh`

2. Engine deployed remotely:

   - Deploy the PaddleOCR-json program on the server, enable server mode, and ensure the client can access the server.
   - Client: `exePath` passes `"remote://ip:port"`.

#### About `modelsPath`

The intention of this parameter is to automatically handle relative path errors in different working paths. When the API starts the engine process, it sets the working path to the engine's parent folder. If the user directly passes the `models_path` path to the `argument` dictionary, path errors can easily occur. The `modelsPath` parameter will first convert the input path to an absolute path based on the current Python running path, then input it to the engine in the form of the `models_path` parameter, thereby preventing path errors. Of course, you can also override this path by inputting a new `models_path` parameter into the `argument` dictionary.

[More details about the `models_path` parameter can be found here](../../README.md#common-configuration-parameters-explanation).

**Return Value:**

If initialization succeeds, returns the engine API object. If initialization fails or remote connection fails, throws an exception.

**Example 1:** Simplest case

```python
ocr = GetOcrApi(r"…………\PaddleOCR_json.exe")
```

**Example 2:** Specify using Traditional Chinese recognition library (need to place recognition library files in the engine's models directory first)

Note, if config_path is a relative path, the root is the path where PaddleOCR-json.exe is located, not the Python script's path.

```python
argument = {'config_path': "models/config_chinese_cht.txt"}
ocr = GetOcrApi(r"…………\PaddleOCR_json.exe", argument)
```

**Example 3:** Specify using socket communication method

Using pipe communication (default) and socket communication, in terms of usage, is transparent, meaning the calling methods are exactly the same.

There is a slight performance difference, pipe is slightly more efficient, while socket TCP may have slightly better stability for large data transmission (such as Base64 image data over 30MB). For ordinary users, use the default settings.

```python
ocr = GetOcrApi(r"…………\PaddleOCR_json.exe", ipcMode="socket")
```

**Example 4:** Use socket mode to connect to remote server

In socket communication mode, you can connect to a remote PaddleOCR-json server. This way, you don't need to deploy the entire system on the same machine.

```python
ip = '192.168.10.1'
port = 1234
ocr = GetOcrApi(r"remote://192.168.10.1:1234", ipcMode="socket")
```

Here we use a URI to replace the engine location, indicating the server's IP and port. Then use the parameter `ipcMode` to use socket mode (cannot use pipe mode). In this case, inputting the `argument` parameter will have no effect, because this Python script does not start the engine process.

In this deployment scenario, we recommend using the `runBase64()` or `runBytes()` methods to transmit files, as the `run()` method's path transmission method is prone to errors. Of course, you can also disable the server's [path transmission json command image_path](../../cpp/README.md#cmake-build-parameters).

### Step 2: Recognize Images

The Python API provides rich interfaces, you can call OCR in various ways.

#### 1. Recognize Local Images

**Method:** `run()`

**Description:** Perform OCR on a local image

**Parameters:**

| Name    | Default | Type | Description                                 |
| ------- | ------- | ---- | ------------------------------------------- |
| imgPath | Required | str  | Path to the image to recognize, e.g. `D:/test/test.png` |

**Return Value Dictionary:**

| Key  | Type | Description                                                    |
| ---- | ---- | -------------------------------------------------------------- |
| code | int  | Status code. 100 if recognition succeeds and has text. Other cases see main README. |
| data | list | If recognition succeeds, data is the OCR result list.         |
| data | str  | If recognition fails, data is the error message string.       |

**Example:**

```python
res = ocr.run("test.png")
print("Recognition result:\n", res)
```

#### 2. Recognize Image Byte Stream

**Method:** `runBytes()`

**Description:** Perform OCR on an image byte stream. Through this interface, you can recognize PIL Image or screenshots or network downloaded images, all in memory, without needing to save to disk first.

**Parameters:**

| Name       | Default | Type  | Description       |
| ---------- | ------- | ----- | ----------------- |
| imageBytes | Required | bytes | Byte stream object |

Return Value Dictionary: Same as above

**Example:**

```python
with open("test.png", 'rb') as f: # Get image byte stream
    imageBytes = f.read() # In actual use, can download from network or screenshot to get byte stream
res = ocr.runBytes(imageBytes)
print("Byte stream recognition result:\n", res)
```

#### 3. Recognize Image Base64 Encoded String

**Method:** `runBase64()`

**Description:** Perform OCR on a Base64 encoded string.

**Parameters:**

| Name        | Default | Type | Description               |
| ----------- | ------- | ---- | ------------------------- |
| imageBase64 | Required | str  | Base64 encoded string |

Return Value Dictionary: Same as above

#### 4. Format Output OCR Result

**Method:** `printResult()`

**Description:** Used for debugging, print an OCR result.

**Parameters:**

| Name | Default | Type | Description              |
| ---- | ------- | ---- | ------------------------ |
| res  | Required | dict | Return result of one OCR |

No Return Value

**Example:**

```python
res = ocr.run("test.png")
print("Formatted output:")
ocr.printResult(res)
```

**Note:** Clipboard related interfaces are deprecated, not recommended for use

#### 5. Recognize Clipboard Image

**Method:** `runClipboard()`

**Description:** Perform OCR on the first image in the current clipboard

No Parameters

Return Value Dictionary: Same as above

**Example:**

```python
res = ocr.runClipboard()
print("Clipboard recognition result:\n", res)
```

**Method:** `isClipboardEnabled()`

**Description:** Check if clipboard function is enabled.

No Parameters

Return Value

If clipboard is enabled: `True`

If clipboard is not enabled: `False`

**Method:** `getRunningMode()`

**Description:** Check the running mode of PaddleOCR-json engine, local or remote

No Parameters

Return Value String

If engine runs locally: `"local"`

If engine runs remotely: `"remote"`

See detailed examples in [demo1.py](demo1.py)

### Step 3: Close OCR Engine Process

Generally, when the program ends or the ocr object is released, the engine subprocess will be automatically closed, no need for manual management.

If you want to manually close the engine process, you can use the `exit()` method.

**Example:**

```python
ocr.exit()
```

If you need to change the recognition language, just recreate the ocr object, the old object will automatically close the old engine process when destructed.

**Example:**

```python
argument = {'config_path': "language1.txt"}
ocr = GetOcrApi(r"…………\PaddleOCR_json.exe", argument)
# TODO: Recognize language1

argument = {'config_path': "language2.txt"}
ocr = GetOcrApi(r"…………\PaddleOCR_json.exe", argument)
# TODO: Recognize language2
```

## Result Visualization Module

Pure Python implementation, does not depend on PPOCR engine's C++ opencv visualization module, avoids Chinese compatibility issues.

Requires PIL image processing library: `pip install pillow`

```python
from PPOCR_visualize import visualize
```

### Get Text Blocks

First, successfully execute OCR once, get the text block list (i.e. the `['data']` part)

```python
testImg = "D:/test.png"
getObj = ocr.run(testImg)
if not getObj["code"] == 100:
    print('Recognition failed!!')
    exit()
textBlocks = getObj["data"]  # Extract text block data
```

### Display Result Image

Just one line of code, pass in text blocks and original image path, open image viewer window

```python
visualize(textBlocks, testImg).show()
```

At this time, the program blocks until the image viewer window is closed before continuing.

### Save Image to Local

```python
visualize(textBlocks, testImg).save('Visualization result.png')
```

### Get PIL Image Object

```python
vis = visualize(textBlocks, testImg)
img = vis.get()
```

### Adjust Display Layers

The above `show`, `save`, `get` three interfaces can all enable or disable specified layers:

- `isBox` T enables bounding box layer.
- `isText` T enables text layer.
- `isOrder` T enables order layer.
- `isSource` T enables original image. F disables original image, i.e. gets pure visualization result with transparent background.

### Left-Right Comparison

Pass in two PIL Image objects, return a new Image formed by splicing them left and right

```python
img_12 = visualize.createContrast(img1, img2)
```

### Adjust Display Effects (Color, Thickness, Font, etc.)

Import PIL library to operate on image objects

```python
from PIL import Image
```

Interface creates each layer, passes in text blocks, layer size to generate, custom parameters, then merges each layer

For color-related parameters, can pass 6-digit RGB hex code (like `#112233`) or 8-digit RGBA code (last two digits control transparency, like `#11223344`)

```python
# Create each layer
img = Image.open(testImg).convert('RGBA')  # Original image background layer
imgBox = visualize.createBox(textBlocks, img.size,  # Bounding box layer
                             outline='#ccaa99aa', width=10)
imgText = visualize.createText(textBlocks, img.size,  # Text layer
                               fill='#44556699')
# Merge each layer
img = visualize.composite(img, imgBox)
img = visualize.composite(img, imgText)
img.show() # Display
```

See detailed examples in [demo2.py](demo2.py)

## Text Post-processing tbpu

(text block processing unit)

```python
from tbpu import GetParser
```

Technology brought by [Umi-OCR](https://github.com/hiroi-sora/Umi-OCR) and [Gap Tree Sorting Algorithm](https://github.com/hiroi-sora/GapTree_Sort_Algorithm).

In the results returned by OCR, an element containing text, bounding box, confidence is called a "text block" - text block.

A text block is not necessarily a complete sentence or paragraph. On the contrary, it is generally scattered text. An OCR result often consists of multiple text blocks, and the original order of these text blocks may not conform to the reading order.

The function of text block post-processing tbpu is: process the original OCR text blocks, adjust their order, and divide into paragraphs.

### Scheme List

| Scheme ID        | Scheme Name      |
| ---------------- | ---------------- |
| `multi_para`     | Multi-column - Natural Paragraph   |
| `multi_line`     | Multi-column - Always Line Break |
| `multi_none`     | Multi-column - No Line Break   |
| `single_para`    | Single-column - Natural Paragraph   |
| `single_line`    | Single-column - Always Line Break |
| `single_none`    | Single-column - No Line Break   |
| `single_code`    | Single-column - Code Block   |

You can also experience the effects of these schemes intuitively in [Umi-OCR](https://github.com/hiroi-sora/Umi-OCR).

Get the corresponding post-processing parser object through `GetParser("scheme id")`. Call through the `run()` interface, pass in the OCR result list, get the processed new list, see below.

### Usage

Pass the text block list (i.e. the `['data']` part) to the interface, return a new text block list.

```python
from tbpu import GetParser

textBlocks = getObj["data"]

# Get "Multi-column - Natural Paragraph" layout parser object
parser = GetParser("multi_para")
# Pass in OCR result list, return new text block list
textBlocksNew = parser.run(textBlocks)
```

- After execution, the structure of the original list textBlocks may be destroyed, do not use the original list anymore (or deep copy and backup first).
- In the new text block list textBlocksNew, the order of each text block will be reordered according to the selected scheme.
- At the same time, each text block in textBlocksNew will add a key `["end"]`, indicating what the ending symbol of this text block is (i.e. the interval symbol with the next text block). Taking `multi_para` as an example:
  - If a text block is at the end of a natural paragraph, then `["end"]=="\n"`.
  - If in the middle of a natural paragraph, and the context is Chinese, then `["end"]==""`.
  - If in the middle of a natural paragraph, and the context is English, then `["end"]==" "`.

Used with result visualization:

```python
from tbpu import GetParser

# OCR original result visualization
textBlocks = getObj["data"]
img1 = visualize(textBlocks, testImg).get(isOrder=True)

# Execute text block post-processing: Multi-column - Natural Paragraph
parser = GetParser("multi_para")
textBlocksNew = parser.run(textBlocks)

# Post-processing result visualization
img2 = visualize(textBlocksNew, testImg).get(isOrder=True)

# Splice images left and right and display
visualize.createContrast(img1, img2).show()
```

See detailed examples in [demo3.py](demo3.py)
