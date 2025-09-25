// Package visualize provides visualization functionality for OCR results
// Project homepage: https://github.com/MeKo-Christian/PaddleOCR-json
package visualize

import (
	"fmt"
	"image"
	"image/draw"
	"os"
)

// NewVisualizer creates a new visualizer for the given text blocks and image
func NewVisualizer(textBlocks []TextBlock, imagePath string) (*Visualizer, error) {
	// Open source image
	file, err := os.Open(imagePath)
	if err != nil {
		return nil, fmt.Errorf("failed to open image: %w", err)
	}
	defer file.Close()

	sourceImg, _, err := image.Decode(file)
	if err != nil {
		return nil, fmt.Errorf("failed to decode image: %w", err)
	}

	// Convert to RGBA if needed
	bounds := sourceImg.Bounds()
	rgbaImg := image.NewRGBA(bounds)
	draw.Draw(rgbaImg, bounds, sourceImg, bounds.Min, draw.Src)

	v := &Visualizer{
		sourceImg: rgbaImg,
		size:      bounds,
	}

	// Create visualization layers
	v.boxImg = CreateBoxLayer(textBlocks, bounds)
	v.textImg = CreateTextLayer(textBlocks, bounds)
	v.orderImg = CreateOrderLayer(textBlocks, bounds)

	return v, nil
}

// Get returns a composite visualization image
func (v *Visualizer) Get(includeBox, includeText, includeOrder, includeSource bool) *image.RGBA {
	result := image.NewRGBA(v.size)

	layers := []struct {
		img  image.Image
		flag bool
	}{
		{v.sourceImg, includeSource},
		{v.boxImg, includeBox},
		{v.textImg, includeText},
		{v.orderImg, includeOrder},
	}

	for _, layer := range layers {
		if layer.img != nil && layer.flag {
			result = Composite(result, layer.img)
		}
	}

	return result
}

// Show saves the visualization to a temporary file and opens it
func (v *Visualizer) Show(includeBox, includeText, includeOrder, includeSource bool) error {
	img := v.Get(includeBox, includeText, includeOrder, includeSource)
	return saveAndOpenImage(img, "temp_visualization.png")
}

// Save saves the visualization to a file
func (v *Visualizer) Save(path string, includeBox, includeText, includeOrder, includeSource bool) error {
	img := v.Get(includeBox, includeText, includeOrder, includeSource)
	return saveImage(img, path)
}