# Testing Guide for PaddleOCR-json

This document describes the testing strategy and how to run tests for PaddleOCR-json.

## Overview

The testing suite includes:
- **C++ Unit Tests**: Core engine functionality testing using Google Test
- **Python API Tests**: Python API testing using pytest
- **Node.js API Tests**: TypeScript/JavaScript API testing using Jest
- **Integration Tests**: Cross-platform compatibility and end-to-end testing
- **CI/CD Pipeline**: Automated testing with GitHub Actions

## Quick Start

### Running All Tests

```bash
# Generate test data first
cd test-data
python generate_test_images.py

# Run C++ tests
cd cpp
mkdir -p build
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
./build/paddleocr_tests

# Run Python tests
cd api/python
pip install -r requirements-test.txt
pytest tests/ -v

# Run Node.js tests
cd api/node.js
npm install
npm test
```

## C++ Testing

### Setup

The C++ tests use Google Test framework and are integrated with CMake.

```bash
cd cpp
mkdir -p build
cmake -S . -B build -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target paddleocr_tests
```

### Running Tests

```bash
cd cpp/build
./paddleocr_tests

# With verbose output
./paddleocr_tests --gtest_output=xml:test_results.xml

# Run specific test suite
./paddleocr_tests --gtest_filter="Base64Test.*"
```

### Test Coverage

The C++ tests cover:
- Base64 encoding/decoding (`test_base64.cpp`)
- Command line argument parsing (`test_args.cpp`)
- Task processing and error codes (`test_task.cpp`)
- JSON response formatting

## Python API Testing

### Setup

```bash
cd api/python
pip install -r requirements-test.txt
```

### Running Tests

```bash
# Run all tests
pytest tests/ -v

# Run with coverage
pytest tests/ --cov=PPOCR_api --cov-report=html

# Run specific test file
pytest tests/test_ppocr_api.py -v

# Run specific test method
pytest tests/test_ppocr_api.py::TestPPOCR_pipe::test_run_success -v
```

### Test Coverage

Python tests cover:
- PPOCR_pipe class functionality
- PPOCR_socket class functionality
- GetOcrApi factory function
- Error handling and edge cases
- Mock-based testing without requiring actual OCR binary

## Node.js API Testing

### Setup

```bash
cd api/node.js
npm install
```

### Running Tests

```bash
# Run all tests
npm test

# Run with coverage
npm run test:coverage

# Run in watch mode
npm run test:watch

# Run specific test file
npx jest __tests__/ocr.test.ts
```

### Test Coverage

Node.js tests cover:
- OCR class constructor and initialization
- TypeScript interface definitions
- Event handling (init, exit)
- Message passing and response handling
- Integration scenarios

## Test Data

### Generating Test Images

```bash
cd test-data
python generate_test_images.py
```

This creates:
- `sample_text.png` - Basic text image
- `multilang_text.jpg` - Multi-language text
- `complex_layout.png` - Complex document layout
- `rotated_text.jpg` - Rotated text
- `low_quality.png` - Low quality image
- `empty_image.png` - No text content

### JSON Fixtures

- `sample_responses.json` - Expected OCR responses
- `error_responses.json` - Error response examples
- `config_samples.json` - Configuration parameter examples

## CI/CD Pipeline

The GitHub Actions workflow (`.github/workflows/test.yml`) automatically:

1. **Builds and tests C++ components** on Ubuntu
2. **Tests Python API** on multiple Python versions (3.8-3.11)
3. **Tests Node.js API** on multiple Node versions (16, 18, 20)
4. **Runs integration tests** to verify cross-platform compatibility
5. **Performs linting and formatting checks**
6. **Uploads coverage reports** to Codecov

### Triggering CI

Tests run automatically on:
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches

### Manual Workflow Trigger

```bash
gh workflow run test.yml
```

## Testing Best Practices

### Writing Tests

1. **Unit Tests**: Test individual functions and classes in isolation
2. **Integration Tests**: Test component interactions
3. **Mock External Dependencies**: Use mocks for OCR binary, file system, network
4. **Test Error Cases**: Include negative test cases and edge conditions
5. **Parameterized Tests**: Use test parameters for multiple input scenarios

### Test Data Management

1. **Minimal Test Data**: Keep test images small and focused
2. **Version Control**: Check in essential test data, generate others
3. **Cleanup**: Always clean up temporary files in tests
4. **Deterministic**: Ensure tests produce consistent results

### Coverage Goals

- **C++ Code**: Aim for >80% line coverage
- **Python API**: Aim for >90% line coverage
- **Node.js API**: Aim for >85% line coverage
- **Integration**: Cover all major user workflows

## Troubleshooting

### Common Issues

1. **Missing Dependencies**
   ```bash
   # C++: Install gflags
   sudo apt-get install libgflags-dev

   # Python: Install test requirements
   pip install -r requirements-test.txt

   # Node.js: Install dev dependencies
   npm install
   ```

2. **Font Issues in Test Image Generation**
   ```bash
   # Install fonts on Ubuntu
   sudo apt-get install fonts-dejavu-core
   ```

3. **CMake Configuration**
   ```bash
   # Clear CMake cache if needed
   rm -rf cpp/build
   mkdir cpp/build
   ```

### Debug Mode

Enable verbose output for debugging:

```bash
# C++
./paddleocr_tests --gtest_output=xml --gtest_print_time=1

# Python
pytest -v -s tests/

# Node.js
npm test -- --verbose
```

## Adding New Tests

### C++ Tests

1. Add test file in `cpp/tests/test_*.cpp`
2. Update `cpp/tests/CMakeLists.txt` if needed
3. Follow Google Test conventions

### Python Tests

1. Add test file in `api/python/tests/test_*.py`
2. Use pytest fixtures from `conftest.py`
3. Mock external dependencies

### Node.js Tests

1. Add test file in `api/node.js/__tests__/*.test.ts`
2. Use Jest mocking capabilities
3. Test both TypeScript interfaces and runtime behavior

## Performance Testing

For performance testing:

```bash
# C++ benchmark mode
./paddleocr_tests --benchmark

# Python timing tests
pytest tests/ --benchmark-only

# Node.js performance tests
npm run test:performance
```

## Contributing

When adding new features:

1. **Write tests first** (TDD approach)
2. **Ensure all existing tests pass**
3. **Add tests for new functionality**
4. **Update documentation** if test procedures change
5. **Verify CI pipeline passes** before merging

For questions or issues with testing, please open an issue on GitHub.