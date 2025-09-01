# 👉 demo1.py: Demonstrates basic OCR functionality
#    demo2.py: Demonstrates visualization interface
#    demo3.py: Demonstrates OCR text post-processing (paragraph merging) interface

from PPOCR_api import GetOcrApi

import os

# Test image path
TestImagePath = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test.jpg")

# Initialize recognizer object, pass PaddleOCR-json engine path.
# Engine download address: https://github.com/hiroi-sora/PaddleOCR-json/releases
# Windows: Pass path to PaddleOCR-json.exe.
# Linux: Pass path to run.sh
ocr = GetOcrApi(r"Your Path/PaddleOCR-json.exe")

if ocr.getRunningMode() == "local":
    print(f"OCR initialization successful, process ID is {ocr.ret.pid}")
elif ocr.getRunningMode() == "remote":
    print(f"Connected to remote OCR engine successfully, ip: {ocr.ip}, port: {ocr.port}")
print(f"\nTest image path: {TestImagePath}")

# Example 1: Recognize local image
res = ocr.run(TestImagePath)
print(f"\nExample 1-Image path recognition result (raw information):\n{res}")
print(f"\nExample 1-Image path recognition result (formatted output):")
ocr.printResult(res)

# Example 2: Recognize image byte stream
with open(TestImagePath, "rb") as f:  # Get image byte stream
    # In practice, byte stream can be obtained through network download or screenshot, input directly to OCR without saving to local temporary storage.
    imageBytes = f.read()
res = ocr.runBytes(imageBytes)
print(f"\nExample 2-Byte stream recognition result:")
ocr.printResult(res)

# Example 3: Recognize PIL Image object
try:
    from PIL import Image
    from io import BytesIO
except Exception:
    print("Install Pillow library to test Example 3.")
    Image = None
if Image:
    # Create PIL Image object
    pilImage = Image.open(TestImagePath)
    # Convert Image object to byte stream
    buffered = BytesIO()
    pilImage.save(buffered, format="PNG")
    imageBytes = buffered.getvalue()
    # Input to OCR
    res = ocr.runBytes(imageBytes)
    print(f"\nExample 3-PIL Image recognition result:")
    ocr.printResult(res)

# Following examples disabled by default
# Example 4: Recognize clipboard image
if ocr.isClipboardEnabled():
    res = ocr.runClipboard()
    if res["code"] == 212:
        print(f"\nExample 4-No image in current clipboard.")
    else:
        print(f"\nExample 4-Clipboard recognition result:")
        ocr.printResult(res)
