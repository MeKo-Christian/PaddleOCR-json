#    demo1.py: Demonstrates basic OCR functionality
#    demo2.py: Demonstrates visualization interface
# 👉 demo3.py: Demonstrates OCR text post-processing (paragraph merging) interface

from PPOCR_api import GetOcrApi
from PPOCR_visualize import visualize  # Visualization
from tbpu import GetParser  # Interface to get layout parser

import os

# Test image path
TestImagePath = os.path.join(os.path.dirname(os.path.abspath(__file__)), "test.jpg")

# Initialize recognizer object, pass PaddleOCR-json engine path.
ocr = GetOcrApi(r"Your Path/PaddleOCR-json.exe")

if ocr.getRunningMode() == "local":
    print(f"OCR initialization successful, process ID is {ocr.ret.pid}")
elif ocr.getRunningMode() == "remote":
    print(f"Connected to remote OCR engine successfully, ip: {ocr.ip}, port: {ocr.port}")
print(f"\nTest image path: {TestImagePath}")

# OCR recognize image, get text blocks
getObj = ocr.run(TestImagePath)
ocr.exit()  # End engine subprocess
if not getObj["code"] == 100:
    print("Recognition failed!!")
    exit()
textBlocks = getObj["data"]  # Extract text block data

# OCR original result visualization Image
img1 = visualize(textBlocks, TestImagePath).get(isOrder=True)
ocr.exit()  # End engine subprocess
print("========== Original Result ==========")
ocr.printResult(getObj)

# Get layout parser object
parser = GetParser("multi_para")
# Pass OCR result list, return new text block list
textBlocksNew = parser.run(textBlocks)
# Note: After processing, the structure of the original list textBlocks may be destroyed, don't use the original list anymore (or make a deep copy backup first).
print("========== Organized Result ==========")
getObj["data"] = textBlocksNew
ocr.printResult(getObj)

# Visualize post-processing result visualization Image
img2 = visualize(textBlocksNew, TestImagePath).get(isOrder=True)
print("Display visualization result. Left is original result, right is result after merging natural paragraphs.")
visualize.createContrast(img1, img2).show()  # Concatenate images left and right and display
