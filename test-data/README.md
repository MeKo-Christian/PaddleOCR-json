# Test Data

This directory contains test images and data fixtures for PaddleOCR-json testing.

## Test Images

- `sample_text.png` - Simple text image for basic OCR testing
- `multilang_text.jpg` - Multi-language text sample
- `complex_layout.png` - Complex document layout with multiple text regions
- `rotated_text.jpg` - Text at various angles for rotation testing
- `low_quality.png` - Low quality/resolution image for robustness testing
- `empty_image.png` - Image with no text content

## JSON Fixtures

- `sample_responses.json` - Expected OCR responses for test images
- `error_responses.json` - Sample error responses for various failure modes
- `config_samples.json` - Sample configuration parameters for testing

## Usage

These test data files are used by:
- C++ unit tests (`cpp/tests/`)
- Python API tests (`api/python/tests/`)
- Node.js API tests (`api/node.js/__tests__/`)
- Integration tests in CI/CD pipeline

## Generating Test Images

Test images can be regenerated using the provided Python script:

```bash
python generate_test_images.py
```

This is useful when adding new test cases or updating existing ones.