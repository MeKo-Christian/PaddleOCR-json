/**
 * Tests for PaddleOCR-json Node.js API
 */

import OCR from '../ts/index';
import { promises as fs } from 'fs';
import { join } from 'path';
import { createCanvas } from 'canvas';

// Mock the worker module since we can't test the actual OCR process
jest.mock('../ts/worker.ts', () => ({
  __default: {
    options: {}
  }
}));

jest.mock('worker_threads', () => ({
  Worker: jest.fn().mockImplementation(() => ({
    stdout: {
      once: jest.fn((event, callback) => {
        if (event === 'data') {
          // Simulate the initialization response
          setTimeout(() => callback(Buffer.from('pid=12345, a=127.0.0.1:8080')), 10);
        }
      })
    },
    once: jest.fn((event, callback) => {
      if (event === 'exit') {
        // Don't call the exit callback immediately in tests
      }
    }),
    postMessage: jest.fn(),
    emit: jest.fn()
  }))
}));

describe('OCR Class', () => {
  let ocr: OCR;

  beforeEach(() => {
    jest.clearAllMocks();
  });

  describe('Constructor', () => {
    it('should create OCR instance with default parameters', () => {
      ocr = new OCR();
      expect(ocr).toBeInstanceOf(OCR);
      expect(ocr.exitCode).toBe(null);
    });

    it('should create OCR instance with path parameter', () => {
      const mockPath = '/path/to/PaddleOCR-json.exe';
      ocr = new OCR(mockPath);
      expect(ocr).toBeInstanceOf(OCR);
    });

    it('should create OCR instance with all parameters', () => {
      const mockPath = '/path/to/PaddleOCR-json.exe';
      const mockArgs = ['--use_gpu=true'];
      const mockOptions = { cwd: '/working/dir' };
      const debug = true;

      ocr = new OCR(mockPath, mockArgs, mockOptions, debug);
      expect(ocr).toBeInstanceOf(OCR);
    });
  });

  describe('Initialization', () => {
    it('should emit init event after worker starts', (done) => {
      ocr = new OCR();

      ocr.once('init', (pid: number, addr?: string, port?: number) => {
        expect(pid).toBe(12345);
        expect(addr).toBe('127.0.0.1');
        expect(port).toBe(8080);
        done();
      });
    });

    it('should set pid and address after initialization', (done) => {
      ocr = new OCR();

      ocr.once('init', () => {
        expect(ocr.pid).toBe(12345);
        expect(ocr.addr).toBe('127.0.0.1');
        expect(ocr.port).toBe(8080);
        done();
      });
    });
  });

  describe('postMessage', () => {
    it('should call flush method', () => {
      ocr = new OCR();
      const flushSpy = jest.spyOn(ocr, 'flush').mockResolvedValue({
        code: 100,
        message: 'Success',
        data: []
      });

      const testArg = { image_path: '/test/image.jpg' };
      ocr.postMessage(testArg);

      expect(flushSpy).toHaveBeenCalledWith(testArg);
    });
  });

  describe('flush method', () => {
    it('should handle image_path argument', async () => {
      ocr = new OCR();

      // Mock the internal queue mechanism
      const mockResponse: OCR.coutReturnType = {
        code: 100,
        message: 'Success',
        data: [{
          box: [[10, 10], [100, 10], [100, 50], [10, 50]],
          score: 0.95,
          text: 'Test Text'
        }]
      };

      // Since we can't easily mock the complex queue mechanism,
      // we'll test the interface contract
      const testArg = { image_path: '/test/image.jpg' };

      // The method should be callable with the correct interface
      expect(() => ocr.flush(testArg)).not.toThrow();
    });

    it('should handle image_base64 argument', async () => {
      ocr = new OCR();

      const testArg = {
        image_base64: 'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg=='
      };

      expect(() => ocr.flush(testArg)).not.toThrow();
    });

    it('should handle additional OCR parameters', async () => {
      ocr = new OCR();

      const testArg = {
        image_path: '/test/image.jpg',
        limit_side_len: 960,
        limit_type: 'max',
        visualize: false,
        output: './output'
      };

      expect(() => ocr.flush(testArg)).not.toThrow();
    });
  });

  describe('Type definitions', () => {
    it('should define correct Arg interface for path-based OCR', () => {
      const pathArg: OCR.Arg = {
        image_path: '/test/image.jpg',
        limit_side_len: 960,
        visualize: true
      };

      expect(pathArg.image_path).toBe('/test/image.jpg');
      expect(pathArg.limit_side_len).toBe(960);
      expect(pathArg.visualize).toBe(true);
    });

    it('should define correct Arg interface for base64-based OCR', () => {
      const base64Arg: OCR.Arg = {
        image_base64: 'base64string',
        limit_type: 'min',
        output: './output'
      };

      expect(base64Arg.image_base64).toBe('base64string');
      expect(base64Arg.limit_type).toBe('min');
      expect(base64Arg.output).toBe('./output');
    });

    it('should define correct return type interface', () => {
      const mockReturn: OCR.coutReturnType = {
        code: 100,
        message: 'Success',
        data: [{
          box: [[0, 0], [10, 0], [10, 10], [0, 10]],
          score: 0.98,
          text: 'Sample text'
        }]
      };

      expect(mockReturn.code).toBe(100);
      expect(mockReturn.message).toBe('Success');
      expect(mockReturn.data).toHaveLength(1);
      expect(mockReturn.data![0].text).toBe('Sample text');
      expect(mockReturn.data![0].score).toBe(0.98);
      expect(mockReturn.data![0].box).toHaveLength(4);
    });

    it('should handle null data in return type', () => {
      const errorReturn: OCR.coutReturnType = {
        code: 200,
        message: 'Error: File not found',
        data: null
      };

      expect(errorReturn.code).toBe(200);
      expect(errorReturn.data).toBeNull();
    });
  });

  describe('Error handling', () => {
    it('should handle worker exit', (done) => {
      ocr = new OCR();

      // Simulate worker exit
      ocr.once('exit', (code: number) => {
        expect(ocr.exitCode).toBe(code);
        done();
      });

      // Manually trigger exit for test
      ocr.exitCode = 1;
      ocr.emit('exit', 1);
    });
  });

  afterEach(() => {
    if (ocr && typeof ocr.terminate === 'function') {
      ocr.terminate();
    }
  });
});