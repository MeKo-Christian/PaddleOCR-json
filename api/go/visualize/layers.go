package visualize

import (
	"fmt"
	"image"
	"image/color"
	"image/draw"
	"math"

	"github.com/golang/freetype/truetype"
	"golang.org/x/image/font"
	"golang.org/x/image/font/gofont/goregular"
	"golang.org/x/image/math/fixed"
)

// CreateBoxLayer creates a bounding box visualization layer
func CreateBoxLayer(textBlocks []TextBlock, size image.Rectangle) *image.RGBA {
	img := image.NewRGBA(size)

	// Default colors
	fill := color.RGBA{0, 80, 0, 64}        // #00500040
	outline := color.RGBA{17, 255, 34, 255} // #11ff22
	width := 6

	for _, tb := range textBlocks {
		if len(tb.Box) < 4 {
			continue
		}

		// Convert box coordinates to polygon
		polygon := make([]image.Point, 4)
		for i, point := range tb.Box {
			if len(point) >= 2 {
				polygon[i] = image.Point{X: point[0], Y: point[1]}
			}
		}

		// Draw filled polygon
		if len(polygon) == 4 {
			drawPolygon(img, polygon, fill, outline, width)
		}
	}

	return img
}

// CreateTextLayer creates a text visualization layer
func CreateTextLayer(textBlocks []TextBlock, size image.Rectangle) *image.RGBA {
	img := image.NewRGBA(size)

	// Load default font
	fontData := goregular.TTF
	ttfFont, err := truetype.Parse(fontData)
	if err != nil {
		// Fallback: don't draw text if font fails
		return img
	}

	// Default colors and scaling
	fill := color.RGBA{255, 0, 0, 255} // #ff0000
	ttfScale := 0.9

	fontCache := make(map[int]font.Face)

	for _, tb := range textBlocks {
		if len(tb.Box) < 1 || len(tb.Text) == 0 {
			continue
		}

		text := tb.Text
		xy := image.Point{X: tb.Box[0][0], Y: tb.Box[0][1]}

		// Calculate font size based on box height
		boxHeight := int(math.Sqrt(
			math.Pow(float64(tb.Box[3][0]-tb.Box[0][0]), 2)+
				math.Pow(float64(tb.Box[3][1]-tb.Box[0][1]), 2),
		) * ttfScale)

		if boxHeight <= 0 {
			boxHeight = 12 // minimum font size
		}

		// Get or create font face
		face, exists := fontCache[boxHeight]
		if !exists {
			face = truetype.NewFace(ttfFont, &truetype.Options{
				Size: float64(boxHeight),
			})
			fontCache[boxHeight] = face
		}

		// Draw text
		d := &font.Drawer{
			Dst:  img,
			Src:  image.NewUniform(fill),
			Face: face,
			Dot:  fixed.Point26_6{X: fixed.Int26_6(xy.X << 6), Y: fixed.Int26_6(xy.Y << 6)},
		}
		d.DrawString(text)
	}

	return img
}

// CreateOrderLayer creates a sequence number visualization layer
func CreateOrderLayer(textBlocks []TextBlock, size image.Rectangle) *image.RGBA {
	img := image.NewRGBA(size)

	// Load default font
	fontData := goregular.TTF
	ttfFont, err := truetype.Parse(fontData)
	if err != nil {
		return img
	}

	// Default colors and size
	fill := color.RGBA{34, 51, 255, 255} // #2233ff
	bg := color.RGBA{255, 255, 255, 230} // #ffffffe0
	ttfSize := 50

	face := truetype.NewFace(ttfFont, &truetype.Options{
		Size: float64(ttfSize),
	})

	for i, tb := range textBlocks {
		if len(tb.Box) < 1 {
			continue
		}

		text := fmt.Sprintf("%d", i+1)
		xy := image.Point{X: tb.Box[0][0], Y: tb.Box[0][1]}

		// Get text bounds
		d := &font.Drawer{
			Face: face,
		}
		bounds, _ := d.BoundString(text)

		width := int((bounds.Max.X - bounds.Min.X) >> 6)
		height := int((bounds.Max.Y - bounds.Min.Y) >> 6)

		// Add padding
		width = int(float64(width) * 1.1)
		height = int(float64(height) * 1.1)

		// Draw background rectangle
		rect := image.Rect(xy.X, xy.Y, xy.X+width, xy.Y+height)
		draw.Draw(img, rect, &image.Uniform{C: bg}, image.Point{}, draw.Over)

		// Draw text
		d = &font.Drawer{
			Dst:  img,
			Src:  image.NewUniform(fill),
			Face: face,
			Dot:  fixed.Point26_6{X: fixed.Int26_6(xy.X << 6), Y: fixed.Int26_6(xy.Y << 6)},
		}
		d.DrawString(text)
	}

	return img
}