#    demo1.py: Demonstrates basic OCR functionality
# 👉 demo2.py: Demonstrates visualization interface
#    demo3.py: Demonstrates OCR text post-processing (paragraph merging) interface

from PPOCR_api import GetOcrApi
from PPOCR_visualize import visualize

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

# Visualization demonstration

# Example 1: Pass text blocks and image path, display result
print("Display image!")
visualize(textBlocks, TestImagePath).show()
# Program blocks until the image viewer window is closed before continuing. If it doesn't move for a long time, comment out the line above and run again

# Example 2: Display more detailed information
vis = visualize(textBlocks, TestImagePath)
print("Get image!")
# Disable bounding box, get original image PIL Image object
visImg1 = vis.get(isBox=False)
# Enable text and serial numbers, disable original image (display transparent background), get PIL Image object
visImg2 = vis.get(isText=True, isOrder=True, isSource=False)
# Get left-right comparison of two images, left is original image, right is separate text boxes
vis = visualize.createContrast(visImg1, visImg2)
# Display the comparison
vis.show()
# Next, you can further process visImg with PIL library.

# Save to local
print(f"Save image to {os.path.dirname(os.path.abspath(__file__))}\\visualization_result.png ")
vis.save(f"{os.path.dirname(os.path.abspath(__file__))}\\visualization_result.png", isText=True)

print("Program ended.")
