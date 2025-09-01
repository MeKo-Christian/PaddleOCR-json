# PaddleOCR-json-node-api
Node.js API based on [hiroi-sora/PaddleOCR-json](https://github.com/hiroi-sora/PaddleOCR-json).

<details>
<summary>Log</summary>

v1.1.0 2023.5.2

Adapted to [hiroi-sora/PaddleOCR-json v1.3.0 Alpha.1](https://github.com/hiroi-sora/PaddleOCR-json/releases/tag/v1.3.0_alpha.1)

(v1.2 rolled back to `paddleocrjson@1.0.11-a`)

v1.0.8 2023.1.20

 \-\-\-

v1.0.7 2022.11.8

 \-\-\-

v1.0.6 2022.11.8

Separated `OCR.postMessage` and `OCR.flush`.

Made the monitored data consistent with the data returned by `OCR.flush`.

Optimized path-related fields in dynamic parameters.

Used `npm` package management.

Used `ts` compilation.

v1.0.5 2022.10.12

 \-\-\-

v1.0.4 2022.10.1

Adapted to [hiroi-sora/PaddleOCR-json v1.2.1](https://github.com/hiroi-sora/PaddleOCR-json/releases/tag/v1.2.1).

No longer using `iconv-lite` package.

Changed startup parameter input method.

v1.0.3 2022.10.1

Adapted to [hiroi-sora/PaddleOCR-json v1.2.1](https://github.com/hiroi-sora/PaddleOCR-json/releases/tag/v1.2.1).

v1.0.2 2022.9.14

Added environment options.

v1.0.1 2022.9.14

Fixed bug where startup completion flag for Alpha version could not be recognized.
Changed JSON input to ASCII escaping.

v1.0.0 2022.9.10

 \-\-\-

</details>

## Quick Start

### OCR API

```
npm install paddleocrjson
```

```js
const OCR = require('paddleocrjson');

const OCR = require('paddleocrjson/es5'); // ES5

const ocr = new OCR('PaddleOCR-json.exe', [/* '-port=9985', '-addr=loopback' */], {
    cwd: './PaddleOCR-json',
}, /* debug */true);

ocr.flush({ image_path: 'path/to/test/img' })
    .then((data) => console.log(data));
    .then(() => ocr.terminate());

// debug = true
ocr.stdout.on('data', (chunk) =>{
    console.log(chunk.toString());
});
ocr.stderr.on('data', (data) =>{
    console.log(data.toString());
});
ocr.on('exit', (code) =>{
    console.log('exit code: ', code);
});

```

### Server Service

See [PunchlY/PaddleOCR-json-node-api/test](https://github.com/PunchlY/PaddleOCR-json-node-api/tree/main/api/node.js/test) for details.

## API

### OCR

`OCR` is a derived class of `worker_threads.Worker`.

#### new OCR(path, args, options, debug)

```js
const ocr = new OCR('PaddleOCR_json.exe', [], {
    cwd: './PaddleOCR-json',
}, false);
```

See [Node.js child_process.spawn](https://nodejs.org/api/child_process.html#child_processspawncommand-args-options) for `args` details and [hiroi-sora/PaddleOCR-json#Configuration Parameters](https://github.com/hiroi-sora/PaddleOCR-json#%E9%85%8D%E7%BD%AE%E5%8F%82%E6%95%B0%E8%AF%B4%E6%98%8E) for configuration parameters.

See [Node.js child_process.spawn](https://nodejs.org/api/child_process.html#child_processspawncommand-args-options) for `options` details.

#### OCR.flush(obj)

```js
ocr.flush({
    image_path: 'path/to/test/img',
    // limit_side_len: 960,
    // ...
});
```
`ocr.flush` returns a `Promise` object.

See [hiroi-sora/PaddleOCR-json/blob/main/docs/Detailed Usage Guide.md#Configuration Parameters](https://github.com/hiroi-sora/PaddleOCR-json/blob/main/docs/%E8%AF%A6%E7%BB%86%E4%BD%BF%E7%94%A8%E6%8C%87%E5%8D%97.md#%E9%85%8D%E7%BD%AE%E5%8F%82%E6%95%B0) for `obj` details.

<details>
<summary>
<strong>Clipboard-related interfaces have been deprecated and are not recommended for use</strong>
</summary>

If you want to recognize clipboard content, pass `{ image_path: null }` (set `obj.image_dir` to `null`).

</details>

#### Other

You can use `worker_threads.Worker`'s API to monitor or operate the `OCR` instance.

For example:
```js
ocr.on('init', (pid, addr, port) => {
    console.log('OCR init completed.');
    console.log('pid:', pid);
    console.log('addr:', addr);
    console.log('port:', port);
});

ocr.on('message', (data) => {
    console.log(data);
    // { code: ..., message: ..., data: ... }
});
```

## License

```
        DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

 Copyright (C) 2023 PunchlY

 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

  1. You just DO WHAT THE FUCK YOU WANT TO.
```
