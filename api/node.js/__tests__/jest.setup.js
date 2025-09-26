// Jest setup file for PaddleOCR-json Node.js API tests

// Increase timeout for async operations
jest.setTimeout(10000);

// Mock console methods to reduce noise during testing
global.console = {
  ...console,
  // Uncomment to silence console output during tests
  // log: jest.fn(),
  // warn: jest.fn(),
  // error: jest.fn(),
};

// Global test utilities
global.createMockOCRResponse = (code = 100, text = 'Test text', score = 0.95) => ({
  code,
  message: code === 100 ? 'Success' : 'Error',
  data: code === 100 ? [{
    box: [[10, 10], [100, 10], [100, 50], [10, 50]],
    score,
    text
  }] : null
});

// Clean up after tests
afterEach(() => {
  jest.clearAllMocks();
});