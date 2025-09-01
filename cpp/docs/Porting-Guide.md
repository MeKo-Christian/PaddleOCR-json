# Porting Guide

I (the project author) have limited capabilities and only provide Windows 10/11 versions. However, the PaddleOCR library supports multiple platforms. Therefore, during the V1.3.0 refactoring process, I focused on portability by separating all platform-dependent code into separate functions and files. In theory, porting workers only need to rewrite some platform-dependent functions to port to the target platform.

The code of this project is mainly divided into two types: "OCR Inference" and "OCR Task Flow Control". The inference part of the code has been adapted for multiple platforms, and only some code in the task flow needs to be rewritten.

When building the project, you need to download and import the PaddlePaddle core library and OpenCV components for the corresponding platform (Linux/macOS). The [Build Guide](../Readme.md) will introduce this.

During development, you can refer to `task.h` and `task_win32.cpp`.

The task flow is controlled by the `Task` class, and the platform-dependent functions that need to be rewritten are the following member functions:

### `imread_u8`

Importance: High. Must be rewritten.

Replace OpenCV's imread, read an image from a utf-8 encoded path string, and return cv::Mat. On failure, set the error code through `set_state`, then return an empty Mat. Note that if the incoming path string is `clipboard`, then perform clipboard OCR.

**Example:**

```cpp
// Replace cv imread, receive utf-8 string input, return Mat.
// flag: Same as cv::imread(path, flag), specifies the image reading format, default is cv::IMREAD_COLOR
cv::Mat Task::imread_u8(std::string pathU8, int flag) {
    if (pathU8 == u8"clipboard")  { // If clipboard task
        return imread_clipboard(flag); // Return clipboard recognition
    }
    // TODO: Try to read image from path pathU8
    if(success) {
        return imageMat;
    }
    else {
        // TODO: Set different error codes and messages according to error type, as follows↓
        set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8)); // Report status: cannot read
        return cv::Mat(); // Return empty Mat
    }
}
```

### `imread_clipboard`

Importance: Low. If you don't want to write it temporarily, you can directly `return cv::Mat();`.

Try to read an image from the current clipboard and return cv::Mat. On failure, set the error code through `set_state`, then return an empty Mat.

**Example:**

```cpp
// Read an image from clipboard, return Mat.
cv::Mat Task::imread_clipboard(int flag)
{
    // TODO: Read clipboard
    if(success) {
        return imageMat
    }
    else {
        // TODO: Set different error codes and messages according to error type, as follows↓
        set_state(CODE_ERR_CLIP_OPEN, MSG_ERR_CLIP_OPEN); // Report status: clipboard open failed
        return cv::Mat(); // Return empty Mat
    }
}
```

### `socket_mode`

Importance: Low. If you don't want to write it temporarily, you can directly `return 0;`.

Socket server mode workflow. Roughly divided into these steps:

- Initialize socket
- Bind address and port number
- Output actual address and port number: e.g. `Socket init completed. 127.0.0.1:8888`
- Start task loop:
  - Accept client connection
  - Accept client command
  - Parse command
  - If class attribute `is_exit` is set to true, exit loop, end process
  - Call OCR
  - Send recognition result back to client
  - Close this connection

Note that after successful server initialization, you must cout output `Socket init completed. IP address:port` so that the client can know and extract the port number.

## Platform File

Please write the rewritten member functions above in a separate .cpp file, such as `task_linux.cpp`. Then add conditional compilation for the entire file.

**Example:** `task_linux.cpp`

```cpp
// Linux platform task processing code

#ifdef _LINUX

#include "include/paddleocr.h"
#include "include/args.h"
#include "include/task.h"

#include <platform-related libraries>

namespace PaddleOCR
{
    cv::Mat Task::imread_u8(std::string pathU8, int flag){
        …………
    }

    cv::Mat Task::imread_clipboard(int flag){
        …………
    }

    int Task::socket_mode(){
        …………
    }
}

#endif
```

If you need to use some auxiliary functions, please write them in this file as well. If you need to add platform-dependent member functions to the class (such as Windows needing `Task::imread_wstr` to handle wide byte codes), then define the member functions in `task.h` and add conditional compilation.

If you need to add new error code and error message macro definitions, please write them at the top of `task.h`.

In short, please refer to `task.h` and `task_win32.cpp`.

#### Umi-OCR Dependency Related

Umi-OCR v1 needs to call `imread_u8` and `imread_clipboard`.
[Umi-OCR v2](https://github.com/hiroi-sora/Umi-OCR_v2) only needs `imread_u8`, no need for `imread_clipboard` and `socket_mode`.

## Contributing Code

I am very happy and grateful for you to contribute code to this project! If possible, please try to follow the following conventions:

About coding:

- Try to write comments.
- Please try to consider and distinguish different exception situations, use error codes and error message macro definitions to feedback to the caller.
- Code files use utf-8 encoding.
- If you need to output debugging information, and do not want the caller to receive it (to avoid interfering with the normal OCR process), use cerr instead of cout.
- Try to reuse existing functions in the project instead of reinventing the wheel.