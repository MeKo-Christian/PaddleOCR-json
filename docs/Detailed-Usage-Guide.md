# PaddleOCR-json Image to Text Program v1.4

# Detailed Usage Guide

This is a document explaining how to call `PaddleOCR-json.exe` through third-party programs.

`v1.4.0` version feature adjustments:

- To avoid privacy and security issues, clipboard OCR function is disabled by default. If needed, you can refer to the [Build Guide](Build-Guide.md) to build the program yourself and enable this function.

## Interaction Methods

There are three interaction methods between the caller and the engine process: single image mode, anonymous pipe mode, and TCP socket server mode.

## Single Image Mode

Specify `image_path=image path` in the startup parameters. The program will recognize the image, output the recognition content, and then end the process.

Note that it is best to pass a pure English path at this time.

<details>
<summary>
<strong>Clipboard related interfaces are deprecated, not recommended for use</strong>
</summary>

Supports clipboard recognition, command is `image_path=clipboard`

</details>

**Example:**
```
PaddleOCR-json.exe -image_path="D:/test/test 1.jpg"
```

## Pipe Mode

Interaction follows the dialogue principle. For each line of input (ending with \n), there will be exactly one line of output. The calling steps are as follows:

### 1. Start the program, redirect pipes

- Under Windows, there are three standard io pipes: standard input stdin, standard output stdout, standard error output stderr. The caller needs to redirect the program's stdin and stdout. **It is not recommended to redirect stderr**, which may receive unnecessary log information. In addition, you need to specify the working directory cwd (that is, the directory where PaddleOCR-json.exe is located).
- If you want to specify parameters (such as configuration file path or OCR parameters), you must pass them in at this stage. Parameters are detailed in the following description.



**Example:** (using python as an example)
```python
ret = subprocess.Popen(
        "program directory/PaddleOCR-json.exe", # Engine location
        cwd="program directory",                # Engine working directory
        stdout=subprocess.PIPE,       # Redirect standard output
        stdin=subprocess.PIPE         # Redirect standard input
    )
```

### 2. Monitor startup completion

- When this program starts, third-party link libraries will print a lot of log information. However, most log output is in the `stderr` stream, which can be ignored.
- The program outputs `OCR init completed.` in `stdout` to indicate that initialization is complete. The caller should first loop reading until the completion flag is read, then enter the formal work.
  
**Example:**
```python
while True:      # Loop until startup success or failure is detected
    strOut = str(ret.stdout.readline()) # Read one line of input
    if "OCR init completed." in strOut: # If startup success is detected
        break                           # Exit loop
    if not self.ret.poll() == None:     # If subprocess is not running
        raise Exception("OCR initialization failed") # Initialization failed, throw exception
print('Initialization completed!')
```

### 3. Call OCR

#### Instruction Format

All incoming instructions must be JSON format strings. **It is strongly recommended to enable ASCII escaping for the incoming JSON** to avoid any encoding issues.

**Example:**

```json
Original:
{"image_path": "测试图片\\test 1.jpg"}

After escaping:
{"image_path": "\u6D4B\u8BD5\u56FE\u7247\\test 1.jpg"}
```

JSON input values support the following parameters (only one can be passed per task):

| Key Name       | Value Description                       |
| -------------- | ---------------------------------------- |
| image_path     | Image path.                              |
| image_base64   | Image encoded as base64 string.          |

Note:

- The base64 string passed to image_base64 should **NOT** have a prefix like `data:image/jpg;base64,`. Just pass the data part. The engine will automatically analyze the image format.

#### Send Instructions and Get Return Values

1. After converting the instruction dictionary to a string, **a newline character `\n` must be added at the end**.
2. Send the string to the engine process's `stdin`.
3. Receive the return value from the engine process's `stdout`.

**Single Task Example:**
```python
imgObj = {"image_path": "test.png"} # Instruction to send
imgStr = json.dumps(imgObj, ensure_ascii=True, indent=None) # Convert to JSON string, enable ASCII escaping, disable auto line breaks
imgStr += "\n"                    # Add newline at the end
ret.stdin.write(imgStr.encode())  # Write to engine process's stdin stream
ret.stdin.flush()                 # Send
getBytes = ret.stdout.readline()  # Get one line of result
getStr = getBytes.decode('ascii', errors='ignore')  # Decode to ASCII string
getObj = json.loads(getStr)       # Deserialize JSON
print("Recognition result:", getObj)
```

### 4. Close Engine Process

After completing all image recognition tasks, you can close the engine process to release occupied system resources.

If you need to switch recognition languages, you also need to close the old engine process first, then start a new process.

There are two methods to close the process:

Method 1: Kill directly.

**Example:**

```python
self.ret.kill()  # Close subprocess
```

Method 2: Pass exit instruction. Can be included in JSON, or send the character `exit\n` directly.

**Example:**

```json
{"exit": ""}
```

## TCP Socket Server Mode

### Differences

The socket mode instruction format is exactly the same as pipe mode, only the startup parameters are different, and the interaction method is changed to TCP.

TCP communication performance is slightly lower than pipes. Considering transmission delay alone, TCP local loopback is one order of magnitude slower than pipes, and the gap is even larger for non-local loopback that goes through the network card. However, this few millisecond gap is negligible compared to the time consumption of OCR tasks themselves. Actual testing with 1000 tasks shows local loopback is only about 3s slower than pipes.

For **local communication**, TCP's advantage is that it is reliable transmission, and less prone to packet loss during continuous reading. Theoretically, it is safer for ultra-large data packets (single 100MB level) compared to pipes.

Of course, TCP socket mode also supports calling engine processes deployed on other devices within the local area network from this machine.

### Enable Method

When the startup parameter **does not** pass `image_path`, it will start multiple task loop mode, which can repeatedly accept caller instructions. The default interaction method is pipe mode. If you need to enable socket mode, you must specify `port` as a value from 0~65535. You can optionally specify the `addr` parameter.

| Key Name | Default Value | Value Description                                                                                                                                        |
| -------- | ------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------- |
| addr     | loopback      | Specify the IP address for the service. Optional values: `loopback`, `localhost` only provides service on local loopback (127.0.0.1). `any` provides service on any available IP address on this machine. Or other IPv4 addresses. |
| port     | -1            | Specify the port for the service. When 0 is passed, random port is used, when 1~65535 is passed, it is set to that port.                                                                                |

**Example:**

```
PaddleOCR-json.exe -port=0
```

**Output:**

If OCR initialization is successful, the program outputs `OCR init completed.` in `stdout` to indicate initialization is complete.

Next it will initialize the socket, if socket initialization is successful, it will output `Socket init completed. IP address:port number` in `stdout`.

If socket initialization fails, error information will be output in `stderr`, then the process ends.

**Success Example:**

```
stdin:   PaddleOCR-json.exe -port=8888
stdout:  …………
stdout:  OCR init completed.
stdout:  Socket init completed. 127.0.0.1:8888
（Task loop begins）
```

**Failure Example:**

```
stdin:   PaddleOCR-json.exe -port=8888
stdout:  …………
stdout:  OCR init completed.
stderr:  Failed to bind address.
（Process ends）
```

If you want the calling program to get the opened port (especially when `-port=0` random port is set), you can listen for another line of output after receiving `OCR init completed.`, and extract the number after the colon `:`.

Note that after successfully starting the server (receiving `Socket init completed.`), it is best to **release the strout redirection**, letting the engine process's stdout output back to the default console stream. This can prevent the `stdout` buffer from being accidentally filled up, causing blockage.

**Example:**

```python
# Assuming OCR initialization completion has been detected, below detects socket initialization completion
strOut = str(ret.stdout.readline())       # Read one line of input
if not self.ret.poll() == None:           # If subprocess is not running
    raise Exception("Socket initialization failed")    # Initialization failed, throw exception
if "Socket init completed. " in strOut:  # Initialization successful
    splits = strOut.split(":")
    self.ip = splits[0].split("Socket init completed. ")[1] # Extract IP address
    self.port = int(splits[1])   # Extract port number
    self.ret.stdout.close()      # Close pipe redirection, prevent buffer filling causing blockage
    print(f"Socket server initialization successful. {self.ip}:{self.port}")
else:
    raise Exception("Socket initialization failed")
```

### Interaction Method

For one OCR task, the client should follow these steps:

1. Connect to the server's IP:port
2. Send instruction (format same as pipe mode)
3. Loop to receive response
4. Disconnect TCP connection

**Single Task Example:**

```python
# Connect
clientSocket.connect(("127.0.0.1", "8888"))
# Send
clientSocket.sendall(sent data)
# Loop to receive response (because single receive buffer size is limited)
resData = b''
while True:
    res = clientSocket.recv(1024)
    if not res: # Receive complete
        break
    resData += res
# Close connection
clientSocket.close()  
# TODO: Process data resData. Need to convert from bytes to string first, then parse JSON to dictionary...
```

### Close Engine

Same as pipe mode, it is recommended to pass `exit` or `{"exit":""}` to end the process, the engine will release occupied network resources.

### No Concurrency Support

Although the engine supports network connections, it does not support concurrency and multi-client connections. If concurrency is needed, it is recommended that the caller acts as a relay.

For example, write a relay in Python, clients first connect to the relay via network, the relay then interacts with the engine process via TCP or pipe. When multiple clients make requests, they queue up to execute tasks sequentially. The relay can return queuing and estimated time information to clients first.

Since the PPOCR engine itself has a very aggressive tuning strategy, heavy tasks usually can fully utilize all CPU cores, so sequential calling is the most efficient. Opening multiple engine processes may instead reduce overall efficiency due to CPU scheduling strategy.

### Development Suggestions

Pipe mode and socket mode have exactly the same input/output value format, only the interaction method is different. When writing APIs, it is recommended to only overload the process interaction functions to adapt to these two modes, and the code for parameter parsing and other parts can be reused.

## Configuration Parameters

Configuration parameters specify various OCR properties and the paths to recognition model libraries. By loading different model libraries, the engine can recognize text in different languages.

### Language-related Parameters

PaddleOCR properties related to language and model libraries are as follows:

| Key Name            | Default Value                          | Value Description              |
| -------------------- | -------------------------------------- | ------------------------------ |
| det_model_dir       | "models/ch_PP-OCRv3_det_infer"         | det target detection model directory |
| cls_model_dir       | "models/ch_ppocr_mobile_v2.0_cls_infer"| cls direction classification model directory |
| rec_model_dir       | "models/ch_PP-OCRv3_rec_infer"         | rec text recognition model directory |
| rec_char_dict_path  | "models/dict_chinese.txt"              | rec text recognition dictionary path |
| rec_img_h           | 48                                     | V2 model libraries need to be changed to 32 |

For different languages, det and cls models are universal, only need to change rec model and rec dictionary. PPOCR-V3 models use the default `rec_img_h`, but V2 models need to be changed to `32`.

It is recommended to import language-related parameters in configuration file format, because the model parameters for each language are fixed, and the configuration file method is easier to call.

The configuration file format is: (key and value can be separated by = or space)

```
# Single line comment
key=value
key value
```

**File Example:** Assume we want to configure a set of German recognition library parameters, rec model is V2 version `german_mobile_v2.0_rec_infer`, dictionary file is `german_dict.txt`.

Put the above two files in the `models` directory, then create a `config_german_v2.txt`, content as follows:

```
# German recognition library (PPOCR-V2)

# rec recognition library directory
rec_model_dir models/german_mobile_v2.0_rec_infer

# rec dictionary path
rec_char_dict_path german_dict.txt

# Because it's V2 model, set to 32. V3 models don't need to write this parameter.
rec_img_h 32
```

Note that relative paths are **relative to the engine working directory!**

**Usage Example:**

```
PaddleOCR-json.exe -config_path="models/config_german_v2.txt"
```

You can find the required model libraries in PP-OCR series [V3 Multi-language Recognition Model List](https://github.com/PaddlePaddle/PaddleOCR/blob/release/2.6/doc/doc_ch/models_list.md#23-%E5%A4%9A%E8%AF%AD%E8%A8%80%E8%AF%86%E5%88%AB%E6%A8%A1%E5%9E%8B%E6%9B%B4%E5%A4%9A%E8%AF%AD%E8%A8%80%E6%8C%81%E7%BB%AD%E6%9B%B4%E6%96%B0%E4%B8%AD) and [Dictionary List](https://github.com/PaddlePaddle/PaddleOCR/tree/release/2.6/ppocr/utils/dict).

### OCR-related Parameters

PaddleOCR common properties related to OCR are as follows:

| Key Name        | Default Value | Value Description                                                                 |
| --------------- | ------------- | --------------------------------------------------------------------------------- |
| enable_mkldnn   | true          | Enable CPU inference acceleration, turning it off can reduce memory usage, but will slow down speed. |
| limit_side_len  | 960           | If the image's long side length is greater than this value, it will be shrunk to this value to improve speed. |
| cls             | false         | Enable cls direction classification, recognize images whose direction is not facing up. |
| use_angle_cls   | false         | Enable direction classification, must be the same as cls value. |

There are more parameters, such as prediction accuracy, filtering thresholds, batch size, etc. Under normal circumstances, these parameters are already optimal. However, if you are familiar with OCR working principles and want to adjust these parameters to suit your task requirements, you can refer to the comments in [args.cpp](../cpp/src/args.cpp).

For OCR-related parameters, it is not recommended to write them in configuration files, but to pass them through startup parameters.

**Example:** Enable direction correction, and increase the scaling threshold to improve recognition rate for large resolution images:

```
PaddleOCR-json.exe -limit_side_len=4096 -cls=1 -use_angle_cls=1
```

### Providing New APIs

Welcome developers to contribute new language APIs to this project, or improve existing APIs!

To ensure PR merging goes smoothly and format uniformity, suggestions:

1. Put the main files of the new API in a new folder under the [api](../api) directory.
2. At least should contain interface `PPOCR_api.xx`, calling example `demo.xx`, usage instructions `README.md`. Please do not add irrelevant files to the project.
3. Write the API name, directory link, short calling example code block into the root directory `README.md`, format as follows:

```
### 1. Python API

[Resource Directory](api/python)

<details>
<summary>Usage Example</summary>

```python
Example code
```

</details>
```

### [Project Build Guide](../cpp)

👆When you need to modify this project's code, please refer to it.
