package visualize

import (
	"image"
	"image/draw"
	"image/png"
	"math"
	"os"
)

// CreateContrast creates a side-by-side comparison of two images
func CreateContrast(img1, img2 image.Image) *image.RGBA {
	bounds1 := img1.Bounds()
	bounds2 := img2.Bounds()

	width := bounds1.Dx() + bounds2.Dx()
	height := int(math.Max(float64(bounds1.Dy()), float64(bounds2.Dy())))

	result := image.NewRGBA(image.Rect(0, 0, width, height))

	// Draw first image
	draw.Draw(result, bounds1, img1, bounds1.Min, draw.Src)

	// Draw second image
	bounds2Dest := image.Rect(bounds1.Dx(), 0, bounds1.Dx()+bounds2.Dx(), bounds2.Dy())
	draw.Draw(result, bounds2Dest, img2, bounds2.Min, draw.Src)

	return result
}

// Composite overlays img2 onto img1
func Composite(img1, img2 image.Image) *image.RGBA {
	result := image.NewRGBA(img1.Bounds())
	draw.Draw(result, img1.Bounds(), img1, img1.Bounds().Min, draw.Src)
	draw.Draw(result, img2.Bounds(), img2, img2.Bounds().Min, draw.Over)
	return result
}

// SaveImage saves an image to a file (public function)
func SaveImage(img image.Image, path string) error {
	return saveImage(img, path)
}

// saveImage saves an image to a file
func saveImage(img image.Image, path string) error {
	file, err := os.Create(path)
	if err != nil {
		return err
	}
	defer file.Close()

	return png.Encode(file, img)
}

// saveAndOpenImage saves an image and attempts to open it
func saveAndOpenImage(img image.Image, path string) error {
	if err := saveImage(img, path); err != nil {
		return err
	}

	// Try to open with default image viewer
	// This is platform-dependent and may not work on all systems
	return nil // For now, just save the file
}