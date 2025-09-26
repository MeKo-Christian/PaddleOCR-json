#!/usr/bin/env python3
"""
Generate test images for PaddleOCR-json testing.

This script creates various test images with different characteristics
to test OCR functionality under different conditions.
"""

import os
from PIL import Image, ImageDraw, ImageFont
import json

def create_simple_text_image():
    """Create a simple text image."""
    img = Image.new('RGB', (300, 100), color='white')
    draw = ImageDraw.Draw(img)

    try:
        # Try to use a system font
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
    except:
        # Fall back to default font
        font = ImageFont.load_default()

    draw.text((10, 30), "Hello World", fill='black', font=font)
    img.save('test-data/sample_text.png')
    return "Hello World"

def create_multilang_text_image():
    """Create a multi-language text image."""
    img = Image.new('RGB', (400, 150), color='white')
    draw = ImageDraw.Draw(img)

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20)
    except:
        font = ImageFont.load_default()

    texts = [
        ("English: Hello", 10, 10),
        ("中文: 你好", 10, 40),
        ("日本語: こんにちは", 10, 70),
        ("한국어: 안녕하세요", 10, 100)
    ]

    for text, x, y in texts:
        draw.text((x, y), text, fill='black', font=font)

    img.save('test-data/multilang_text.jpg')
    return ["English: Hello", "中文: 你好", "日本語: こんにちは", "한국어: 안녕하세요"]

def create_complex_layout_image():
    """Create an image with complex layout."""
    img = Image.new('RGB', (500, 300), color='white')
    draw = ImageDraw.Draw(img)

    try:
        title_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 28)
        body_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
    except:
        title_font = body_font = ImageFont.load_default()

    # Title
    draw.text((50, 20), "Document Title", fill='black', font=title_font)

    # Two columns
    draw.text((50, 80), "Column 1 Text", fill='black', font=body_font)
    draw.text((50, 110), "More content here", fill='black', font=body_font)

    draw.text((300, 80), "Column 2 Text", fill='black', font=body_font)
    draw.text((300, 110), "Additional info", fill='black', font=body_font)

    # Footer
    draw.text((50, 250), "Footer information", fill='gray', font=body_font)

    img.save('test-data/complex_layout.png')
    return ["Document Title", "Column 1 Text", "More content here",
            "Column 2 Text", "Additional info", "Footer information"]

def create_rotated_text_image():
    """Create an image with rotated text."""
    img = Image.new('RGB', (300, 200), color='white')

    # Create text image to rotate
    txt_img = Image.new('RGB', (150, 30), color='white')
    txt_draw = ImageDraw.Draw(txt_img)

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20)
    except:
        font = ImageFont.load_default()

    txt_draw.text((10, 5), "Rotated Text", fill='black', font=font)

    # Rotate and paste
    rotated = txt_img.rotate(15, expand=True, fillcolor='white')
    img.paste(rotated, (75, 85))

    img.save('test-data/rotated_text.jpg')
    return "Rotated Text"

def create_low_quality_image():
    """Create a low quality/resolution image."""
    # Create at normal size first
    img = Image.new('RGB', (200, 60), color='white')
    draw = ImageDraw.Draw(img)

    try:
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16)
    except:
        font = ImageFont.load_default()

    draw.text((10, 20), "Low Quality", fill='black', font=font)

    # Resize to make it low quality
    small = img.resize((50, 15))
    low_quality = small.resize((200, 60))

    low_quality.save('test-data/low_quality.png')
    return "Low Quality"

def create_empty_image():
    """Create an image with no text."""
    img = Image.new('RGB', (200, 100), color='white')
    img.save('test-data/empty_image.png')
    return None

def generate_expected_responses():
    """Generate expected OCR responses for test images."""
    responses = {
        "sample_text.png": {
            "code": 100,
            "data": [{
                "box": [[10, 30], [150, 30], [150, 55], [10, 55]],
                "score": 0.95,
                "text": "Hello World"
            }]
        },
        "multilang_text.jpg": {
            "code": 100,
            "data": [
                {"box": [[10, 10], [120, 10], [120, 30], [10, 30]], "score": 0.90, "text": "English: Hello"},
                {"box": [[10, 40], [80, 40], [80, 60], [10, 60]], "score": 0.88, "text": "中文: 你好"},
                {"box": [[10, 70], [150, 70], [150, 90], [10, 90]], "score": 0.85, "text": "日本語: こんにちは"},
                {"box": [[10, 100], [160, 100], [160, 120], [10, 120]], "score": 0.87, "text": "한국어: 안녕하세요"}
            ]
        },
        "empty_image.png": {
            "code": 101,
            "data": []
        }
    }

    with open('test-data/sample_responses.json', 'w', encoding='utf-8') as f:
        json.dump(responses, f, indent=2, ensure_ascii=False)

def generate_error_responses():
    """Generate sample error responses."""
    errors = {
        "file_not_found": {
            "code": 200,
            "data": "Image path does not exist. Path: \"/non/existent/path.jpg\""
        },
        "decode_error": {
            "code": 203,
            "data": "Image decode failed. Path: \"/corrupted/image.jpg\""
        },
        "base64_error": {
            "code": 300,
            "data": "Base64 decode failed."
        }
    }

    with open('test-data/error_responses.json', 'w') as f:
        json.dump(errors, f, indent=2)

def generate_config_samples():
    """Generate sample configuration parameters."""
    configs = {
        "basic_config": {
            "det": True,
            "cls": True,
            "rec": True,
            "use_angle_cls": True,
            "limit_side_len": 960,
            "det_db_thresh": 0.3,
            "det_db_box_thresh": 0.6
        },
        "high_accuracy_config": {
            "det": True,
            "cls": True,
            "rec": True,
            "use_angle_cls": True,
            "limit_side_len": 1280,
            "det_db_thresh": 0.2,
            "det_db_box_thresh": 0.5,
            "det_db_unclip_ratio": 2.0
        },
        "fast_config": {
            "det": True,
            "cls": False,
            "rec": True,
            "use_angle_cls": False,
            "limit_side_len": 720,
            "det_db_thresh": 0.4,
            "det_db_box_thresh": 0.7
        }
    }

    with open('test-data/config_samples.json', 'w') as f:
        json.dump(configs, f, indent=2)

def main():
    """Generate all test data."""
    print("Generating test images...")

    # Create test directory if it doesn't exist
    os.makedirs('test-data', exist_ok=True)

    # Generate test images
    create_simple_text_image()
    create_multilang_text_image()
    create_complex_layout_image()
    create_rotated_text_image()
    create_low_quality_image()
    create_empty_image()

    # Generate JSON fixtures
    generate_expected_responses()
    generate_error_responses()
    generate_config_samples()

    print("Test data generation complete!")
    print("Generated files:")
    for file in os.listdir('test-data'):
        print(f"  - {file}")

if __name__ == '__main__':
    main()