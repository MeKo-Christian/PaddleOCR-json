package main
// demo1.go: Demonstrates basic OCR functionality
// demo2.go: Demonstrates visualization interface
// demo3.go: Demonstrates OCR text post-processing (paragraph merging) interface

import (
	"fmt"
	"os"
	"path/filepath"

	"paddleocr-json/ppocr"
)

func main() {
	// Test image path
	testImagePath := filepath.Join("..", "..", "..", "api", "python", "test.jpg")

	// Initialize recognizer object, pass PaddleOCR-json engine path.
	// Engine download address: https://github.com/MeKo-Christian/PaddleOCR-json/releases
	// Windows: Pass path to PaddleOCR-json.exe.
	// Linux: Pass path to run.sh
	ocr, err := ppocr.NewPPOCRPipe("../../../build/standard/bin/PaddleOCR-json", nil, nil)
	if err != nil {
		fmt.Printf("Failed to initialize OCR: %v\n", err)
		os.Exit(1)
	}
	defer ocr.Exit()

	if ocr.GetRunningMode() == "local" {
		fmt.Printf("OCR initialization successful, process ID is %d\n", ocr.(*ppocr.PPOCRPipe).GetPID())
	} else if ocr.GetRunningMode() == "remote" {
		fmt.Printf("Connected to remote OCR engine successfully, ip: %s, port: %d\n", ocr.(*ppocr.PPOCRSocket).GetIP(), ocr.(*ppocr.PPOCRSocket).GetPort())
	}
	fmt.Printf("\nTest image path: %s\n", testImagePath)

	// Example 1: Recognize local image
	fmt.Println("\nExample 1 - Image path recognition result (raw information):")
	res := ocr.Run(testImagePath)
	fmt.Printf("%+v\n", res)
	fmt.Println("\nExample 1 - Image path recognition result (formatted output):")
	ppocr.PrintResult(res)

	// Example 2: Recognize image byte stream
	fmt.Println("\nExample 2 - Byte stream recognition result:")
	imageBytes, err := os.ReadFile(testImagePath)
	if err != nil {
		fmt.Printf("Failed to read image file: %v\n", err)
		return
	}
	// In practice, byte stream can be obtained through network download or screenshot, input directly to OCR without saving to local temporary storage.
	res = ocr.RunBytes(imageBytes)
	ppocr.PrintResult(res)

	// Example 3: Recognize PIL Image object (simulated with image bytes)
	fmt.Println("\nExample 3 - Image bytes recognition result:")
	// In Go, we can simulate PIL Image functionality by reading and processing image bytes
	// For this demo, we'll just use the same image bytes as example 2
	res = ocr.RunBytes(imageBytes)
	ppocr.PrintResult(res)

	// Following examples disabled by default
	// Example 4: Recognize clipboard image
	if ocr.IsClipboardEnabled() {
		fmt.Println("\nExample 4 - Clipboard recognition result:")
		res = ocr.RunClipboard()
		if res.Code == 212 {
			fmt.Println("No image in current clipboard.")
		} else {
			ppocr.PrintResult(res)
		}
	}
}