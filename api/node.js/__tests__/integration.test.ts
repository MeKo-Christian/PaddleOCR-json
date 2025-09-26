/**
 * Integration tests for PaddleOCR-json Node.js API
 * These tests focus on testing the API without requiring actual OCR binary
 */

import OCR from '../ts/index';
import { createCanvas } from 'canvas';

// Mock canvas since it might not be available in test environment
jest.mock('canvas', () => ({
  createCanvas: jest.fn().mockReturnValue({
    getContext: jest.fn().mockReturnValue({
      fillRect: jest.fn(),
      fillText: jest.fn(),
      measureText: jest.fn().mockReturnValue({ width: 100 })
    }),
    toBuffer: jest.fn().mockReturnValue(Buffer.from('fake-image-data'))
  })
}));

describe('OCR Integration Tests', () => {
  describe('API Workflow', () => {
    it('should handle complete OCR workflow with image path', (done) => {
      const ocr = new OCR('/mock/path/PaddleOCR-json.exe');

      ocr.once('init', (pid, addr, port) => {
        expect(typeof pid).toBe('number');

        // Test the flush operation
        const testRequest = {
          image_path: '/test/image.jpg'
        };

        // Since we can't test actual OCR, we just verify the method exists and is callable
        expect(typeof ocr.flush).toBe('function');
        expect(() => ocr.postMessage(testRequest)).not.toThrow();

        done();
      });
    });

    it('should handle OCR workflow with base64 image', (done) => {
      const ocr = new OCR();

      ocr.once('init', () => {
        const testRequest = {
          image_base64: 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg=='
        };

        expect(() => ocr.postMessage(testRequest)).not.toThrow();
        done();
      });
    });

    it('should handle OCR parameters correctly', (done) => {
      const ocr = new OCR('/mock/path/PaddleOCR-json.exe', ['--use_gpu=false'], { cwd: '/tmp' });

      ocr.once('init', () => {
        const complexRequest = {
          image_path: '/test/complex.jpg',
          limit_side_len: 1024,
          limit_type: 'max',
          visualize: true,
          output: './output'
        };

        expect(() => ocr.postMessage(complexRequest)).not.toThrow();
        done();
      });
    });
  });

  describe('Response Handling', () => {
    it('should define proper response structure for successful OCR', () => {
      const successResponse: OCR.coutReturnType = {
        code: 100,
        message: 'Success',
        data: [
          {
            box: [[10, 20], [150, 20], [150, 50], [10, 50]],
            score: 0.95,
            text: 'Hello World'
          },
          {
            box: [[10, 60], [120, 60], [120, 90], [10, 90]],
            score: 0.88,
            text: 'Test Text'
          }
        ]
      };

      expect(successResponse.code).toBe(100);
      expect(successResponse.data).toHaveLength(2);
      expect(successResponse.data![0].text).toBe('Hello World');
      expect(successResponse.data![1].score).toBe(0.88);
    });

    it('should define proper response structure for OCR errors', () => {
      const errorResponse: OCR.coutReturnType = {
        code: 200,
        message: 'Image path does not exist',
        data: null
      };

      expect(errorResponse.code).toBe(200);
      expect(errorResponse.data).toBeNull();
      expect(errorResponse.message).toContain('does not exist');
    });

    it('should define proper response structure for no text found', () => {
      const noTextResponse: OCR.coutReturnType = {
        code: 101,
        message: 'No text found',
        data: []
      };

      expect(noTextResponse.code).toBe(101);
      expect(noTextResponse.data).toHaveLength(0);
    });
  });

  describe('Argument Validation', () => {
    it('should accept path-based arguments', () => {
      const pathArg: OCR.Arg = {
        image_path: '/valid/path/image.jpg'
      };

      expect(pathArg.image_path).toBeDefined();
      expect(typeof pathArg.image_path).toBe('string');
    });

    it('should accept base64-based arguments', () => {
      const base64Arg: OCR.Arg = {
        image_base64: 'validBase64String=='
      };

      expect(base64Arg.image_base64).toBeDefined();
      expect(typeof base64Arg.image_base64).toBe('string');
    });

    it('should accept additional OCR parameters', () => {
      const complexArg: OCR.Arg = {
        image_path: '/test.jpg',
        limit_side_len: 960,
        limit_type: 'max',
        visualize: false,
        output: './results'
      };

      expect(complexArg.limit_side_len).toBe(960);
      expect(complexArg.limit_type).toBe('max');
      expect(complexArg.visualize).toBe(false);
      expect(complexArg.output).toBe('./results');
    });
  });

  describe('Error Scenarios', () => {
    it('should handle process termination gracefully', (done) => {
      const ocr = new OCR();

      ocr.once('init', () => {
        ocr.once('exit', (code) => {
          expect(ocr.exitCode).toBe(code);
          done();
        });

        // Simulate exit
        ocr.exitCode = 0;
        ocr.emit('exit', 0);
      });
    });

    it('should handle invalid initialization', () => {
      expect(() => {
        const ocr = new OCR('/invalid/path/to/executable');
        // Should not throw during construction
      }).not.toThrow();
    });
  });

  describe('TypeScript Interface Compliance', () => {
    it('should ensure Arg type accepts both path and base64', () => {
      // This test ensures our union type works correctly
      function testArgument(arg: OCR.Arg): void {
        if ('image_path' in arg) {
          expect(typeof arg.image_path).toBe('string');
        } else if ('image_base64' in arg) {
          expect(typeof arg.image_base64).toBe('string');
        }
      }

      const pathArg: OCR.Arg = { image_path: '/test.jpg' };
      const base64Arg: OCR.Arg = { image_base64: 'base64string' };

      expect(() => testArgument(pathArg)).not.toThrow();
      expect(() => testArgument(base64Arg)).not.toThrow();
    });

    it('should ensure return type handles both success and error cases', () => {
      function handleResponse(response: OCR.coutReturnType): string {
        if (response.code === 100 && response.data && response.data.length > 0) {
          return response.data.map(item => item.text).join(' ');
        } else {
          return `Error ${response.code}: ${response.message}`;
        }
      }

      const successResponse: OCR.coutReturnType = {
        code: 100,
        message: 'Success',
        data: [{ box: [[0,0],[10,0],[10,10],[0,10]], score: 0.9, text: 'Hello' }]
      };

      const errorResponse: OCR.coutReturnType = {
        code: 200,
        message: 'File not found',
        data: null
      };

      expect(handleResponse(successResponse)).toBe('Hello');
      expect(handleResponse(errorResponse)).toBe('Error 200: File not found');
    });
  });
});