package main

// demo1.go: Demonstrates basic OCR functionality
// demo2.go: Demonstrates visualization interface
// demo3.go: Demonstrates OCR text post-processing (paragraph merging) interface

import (
	"fmt"
	"os"
	"path/filepath"

	"paddleocr-json/ppocr"
	"paddleocr-json/tbpu"
	"paddleocr-json/visualize"
)

func main() {
	// Test image path
	testImagePath := filepath.Join("..", "..", "..", "api", "python", "test.jpg")

	// Initialize recognizer object, pass PaddleOCR-json engine path.
	ocr, err := ppocr.GetOcrApi("../../../build/standard/bin/PaddleOCR-json", nil, nil, "pipe")
	if err != nil {
		fmt.Printf("Failed to initialize OCR: %v\n", err)
		os.Exit(1)
	}

	if ocr.GetRunningMode() == "local" {
		if pipe, ok := ocr.(*ppocr.PPOCRPipe); ok {
			fmt.Printf("OCR initialization successful, process ID is %d\n", pipe.GetPID())
		}
	} else if ocr.GetRunningMode() == "remote" {
		if socket, ok := ocr.(*ppocr.PPOCRSocket); ok {
			fmt.Printf("Connected to remote OCR engine successfully, ip: %s, port: %d\n", socket.GetIP(), socket.GetPort())
		}
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

	// OCR original result visualization Image
	vis1, err := visualize.NewVisualizer(textBlocks, testImagePath)
	if err != nil {
		fmt.Printf("Failed to create visualizer: %v\n", err)
		os.Exit(1)
	}
	img1 := vis1.Get(false, false, true, true) // Show order numbers and source

	fmt.Println("========== Original Result ==========")
	ppocr.PrintResult(getObj)

	// Get layout parser object
	parser := tbpu.GetParser("multi_para")

	// Convert visualize TextBlocks back to tbpu format for processing
	var tbpuBlocks []tbpu.TextBlock
	for _, tb := range textBlocks {
		tbpuBlocks = append(tbpuBlocks, tbpu.TextBlock{
			Box:   tb.Box,
			Score: tb.Score,
			Text:  tb.Text,
			End:   tb.End,
		})
	}

	// Pass OCR result list, return new text block list
	textBlocksNew := parser.Run(tbpuBlocks)

	// Convert back to visualize format
	var visualizeBlocksNew []visualize.TextBlock
	for _, tb := range textBlocksNew {
		visualizeBlocksNew = append(visualizeBlocksNew, visualize.TextBlock{
			Box:   tb.Box,
			Score: tb.Score,
			Text:  tb.Text,
			End:   tb.End,
		})
	}

	// Note: After processing, the structure of the original list textBlocks may be destroyed, don't use the original list anymore (or make a deep copy backup first).
	fmt.Println("========== Organized Result ==========")

	// Create a new result object for display
	newData := make([]interface{}, len(textBlocksNew))
	for i, tb := range textBlocksNew {
		newData[i] = map[string]interface{}{
			"box":   tb.Box,
			"score": tb.Score,
			"text":  tb.Text,
			"end":   tb.End,
		}
	}
	newGetObj := ppocr.OCRResult{
		Code: getObj.Code,
		Data: newData,
	}
	ppocr.PrintResult(newGetObj)

	// Visualize post-processing result visualization Image
	vis2, err := visualize.NewVisualizer(visualizeBlocksNew, testImagePath)
	if err != nil {
		fmt.Printf("Failed to create visualizer for processed results: %v\n", err)
		os.Exit(1)
	}
	img2 := vis2.Get(false, false, true, true) // Show order numbers and source

	fmt.Println("Display visualization result. Left is original result, right is result after merging natural paragraphs.")

	// Concatenate images left and right and display
	contrastImg := visualize.CreateContrast(img1, img2)

	// Save the contrast image
	tempPath := filepath.Join(os.TempDir(), "paragraph_merge_comparison.png")
	err = visualize.SaveImage(contrastImg, tempPath)
	if err != nil {
		fmt.Printf("Failed to save comparison image: %v\n", err)
	} else {
		fmt.Printf("Comparison image saved to: %s\n", tempPath)
	}

	fmt.Println("Program ended.")
}