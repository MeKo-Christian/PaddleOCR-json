package main

// demo1.go: Demonstrates basic OCR functionality
// demo2.go: Demonstrates visualization interface
// demo3.go: Demonstrates OCR text post-processing (paragraph merging) interface

import (
	"fmt"
	"os"
	"path/filepath"

	"paddleocr-json/ppocr"
	"paddleocr-json/visualize"
)

func main() {
	// Test image path
	testImagePath := filepath.Join("..", "..", "..", "api", "python", "test.jpg")

	// Initialize recognizer object, pass PaddleOCR-json engine path.
	ocr, err := ppocr.NewPPOCRPipe("../../../build/standard/bin/PaddleOCR-json", nil, nil)
	if err != nil {
		fmt.Printf("Failed to initialize OCR: %v\n", err)
		os.Exit(1)
	}

	if ocr.GetRunningMode() == "local" {
		fmt.Printf("OCR initialization successful, process ID is %d\n", ocr.(*ppocr.PPOCRPipe).GetPID())
	} else if ocr.GetRunningMode() == "remote" {
		fmt.Printf("Connected to remote OCR engine successfully, ip: %s, port: %d\n", ocr.(*ppocr.PPOCRSocket).GetIP(), ocr.(*ppocr.PPOCRSocket).GetPort())
	}
	fmt.Printf("\nTest image path: %s\n", testImagePath)

	// OCR recognize image, get text blocks
	getObj := ocr.Run(testImagePath)
	ocr.Exit() // End engine subprocess
	if getObj.Code != 100 {
		fmt.Println("Recognition failed!!")
		os.Exit(1)
	}

	// Convert ppocr result to visualize TextBlock format
	var textBlocks []visualize.TextBlock
	if dataSlice, ok := getObj.Data.([]interface{}); ok {
		for _, dataItem := range dataSlice {
			if dataMap, ok := dataItem.(map[string]interface{}); ok {
				// Extract box coordinates
				var box [][]int
				if boxData, ok := dataMap["box"].([]interface{}); ok {
					for _, boxItem := range boxData {
						if boxPoint, ok := boxItem.([]interface{}); ok {
							var point []int
							for _, coord := range boxPoint {
								if coordFloat, ok := coord.(float64); ok {
									point = append(point, int(coordFloat))
								}
							}
							if len(point) == 2 {
								box = append(box, point)
							}
						}
					}
				}

				// Extract other fields
				score, _ := dataMap["score"].(float64)
				text, _ := dataMap["text"].(string)
				end, _ := dataMap["end"].(string)

				textBlocks = append(textBlocks, visualize.TextBlock{
					Box:   box,
					Score: score,
					Text:  text,
					End:   end,
				})
			}
		}
	}

	// Visualization demonstration

	// Example 1: Pass text blocks and image path, display result
	fmt.Println("Display image!")
	vis, err := visualize.NewVisualizer(textBlocks, testImagePath)
	if err != nil {
		fmt.Printf("Failed to create visualizer: %v\n", err)
		os.Exit(1)
	}
	vis.Show(true, true, true, true) // Show all layers
	// Program blocks until the image viewer window is closed before continuing. If it doesn't move for a long time, comment out the line above and run again

	// Example 2: Display more detailed information
	vis, err = visualize.NewVisualizer(textBlocks, testImagePath)
	if err != nil {
		fmt.Printf("Failed to create visualizer: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("Get image!")
	// Disable bounding box, get original image
	visImg1 := vis.Get(false, false, false, true) // Only source image
	// Enable text and serial numbers, disable original image (display transparent background)
	visImg2 := vis.Get(false, true, true, false) // Text and order only
	// Get left-right comparison of two images, left is original image, right is separate text boxes
	contrastImg := visualize.CreateContrast(visImg1, visImg2)

	// Save the contrast image temporarily and show it
	tempPath := filepath.Join(os.TempDir(), "contrast_visualization.png")
	err = visualize.SaveImage(contrastImg, tempPath)
	if err != nil {
		fmt.Printf("Failed to save contrast image: %v\n", err)
	} else {
		fmt.Printf("Contrast image saved to: %s\n", tempPath)
	}

	// Save to local
	currentDir, _ := os.Getwd()
	outputPath := filepath.Join(currentDir, "visualization_result.png")
	fmt.Printf("Save image to %s\n", outputPath)
	err = vis.Save(outputPath, false, true, false, true) // Save with text overlay
	if err != nil {
		fmt.Printf("Failed to save visualization: %v\n", err)
	}

	fmt.Println("Program ended.")
}