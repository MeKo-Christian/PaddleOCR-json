# V1.3 Update Notes

## New Features

- Added Base64 method for passing images, you can encode images as Base64 strings and send them in JSON to the engine for recognition.
- Added socket server mode for process interaction, the engine process opens a TCP port, and callers can call OCR through IP + port.

## Startup Parameter Changes

- Removed the three debug-related parameters: `use_system_pause`, `ensure_chcp`, `use_debug`. The program will not pause by default.
- Changed the default value of `ensure_ascii` to `true`, enabling ASCII escaping by default.
- No longer supports passing image paths with `-image_dir=path`, please use `-image_path=path`.
- Added two parameters related to server mode: address `addr` and port `port`.

## JSON Input Value Changes

- No longer supports parameter hot updates, as they have limited use. All parameters must be specified at startup.
- No longer supports passing paths directly, paths must be included in JSON, such as `{"image_path":""}`
- No longer supports passing image paths with `{"image_dir":""}`, please use `{"image_path":""}`.
- Added process termination command: `exit`. You can terminate the engine process by passing `{"exit":""}` or directly passing `exit`. Of course, manually killing it also works.
- Added Base64 image transmission method: `{"image_base64":"base64 encoded string"}`.
- Pipe mode and socket mode have exactly the same input/output value format, only the interaction method is different. When writing APIs, it is recommended to only overload the process interaction functions to adapt to these two modes, and the code for parameter parsing and other parts can be reused.
