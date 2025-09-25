// Package visualize provides visualization functionality for OCR results
package visualize

import "image"

// TextBlock represents a text block from OCR results
type TextBlock struct {
	Box   [][]int `json:"box"`
	Score float64 `json:"score"`
	Text  string  `json:"text"`
	End   string  `json:"end,omitempty"`
}

// Visualizer provides visualization functionality for OCR results
type Visualizer struct {
	sourceImg *image.RGBA
	size      image.Rectangle
	boxImg    *image.RGBA
	textImg   *image.RGBA
	orderImg  *image.RGBA
}