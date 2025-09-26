"""Pytest configuration and fixtures for PaddleOCR-json Python API tests."""

import pytest
import os
import sys
import tempfile
import base64
from PIL import Image, ImageDraw, ImageFont
from io import BytesIO

# Add the parent directory to the path so we can import PPOCR_api
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

@pytest.fixture
def test_image_path():
    """Create a test image with text for OCR testing."""
    # Create a simple test image with text
    img = Image.new('RGB', (200, 100), color='white')
    draw = ImageDraw.Draw(img)

    # Try to use a basic font, fall back to default if not available
    try:
        font = ImageFont.load_default()
    except:
        font = None

    draw.text((10, 30), "Hello World", fill='black', font=font)

    # Save to temporary file
    temp_file = tempfile.NamedTemporaryFile(suffix='.png', delete=False)
    img.save(temp_file.name, 'PNG')

    yield temp_file.name

    # Cleanup
    try:
        os.unlink(temp_file.name)
    except:
        pass

@pytest.fixture
def test_image_bytes(test_image_path):
    """Get test image as bytes."""
    with open(test_image_path, 'rb') as f:
        return f.read()

@pytest.fixture
def test_image_base64(test_image_bytes):
    """Get test image as base64 string."""
    return base64.b64encode(test_image_bytes).decode('utf-8')

@pytest.fixture
def mock_ocr_exe():
    """Mock OCR executable path for testing."""
    return "/mock/path/to/PaddleOCR-json.exe"

@pytest.fixture
def sample_ocr_response():
    """Sample OCR response for testing."""
    return {
        "code": 100,
        "data": [
            {
                "box": [[10, 30], [150, 30], [150, 60], [10, 60]],
                "score": 0.95,
                "text": "Hello World"
            }
        ]
    }

@pytest.fixture
def sample_error_response():
    """Sample error response for testing."""
    return {
        "code": 200,
        "data": "Image path does not exist. Path: \"/non/existent/path.jpg\""
    }