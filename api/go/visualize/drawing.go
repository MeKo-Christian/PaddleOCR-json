package visualize

import (
	"image"
	"image/color"
	"math"
)

// drawPolygon draws a filled polygon with outline
func drawPolygon(img *image.RGBA, points []image.Point, fill, outline color.RGBA, width int) {
	if len(points) < 3 {
		return
	}

	bounds := img.Bounds()

	// Simple polygon filling using scanline algorithm
	minY := bounds.Max.Y
	maxY := bounds.Min.Y
	for _, p := range points {
		if p.Y < minY {
			minY = p.Y
		}
		if p.Y > maxY {
			maxY = p.Y
		}
	}

	// For each scanline
	for y := minY; y <= maxY; y++ {
		var intersections []int

		// Find intersections with polygon edges
		for i := 0; i < len(points); i++ {
			j := (i + 1) % len(points)
			if (points[i].Y <= y && points[j].Y > y) || (points[i].Y > y && points[j].Y <= y) {
				// Calculate intersection x coordinate
				x := points[i].X + (y-points[i].Y)*(points[j].X-points[i].X)/(points[j].Y-points[i].Y)
				intersections = append(intersections, x)
			}
		}

		// Sort intersections
		for i := 0; i < len(intersections)-1; i++ {
			for j := i + 1; j < len(intersections); j++ {
				if intersections[i] > intersections[j] {
					intersections[i], intersections[j] = intersections[j], intersections[i]
				}
			}
		}

		// Fill between pairs of intersections
		for i := 0; i < len(intersections); i += 2 {
			if i+1 < len(intersections) {
				x1 := intersections[i]
				x2 := intersections[i+1]

				// Draw horizontal line
				for x := x1; x <= x2; x++ {
					if x >= bounds.Min.X && x < bounds.Max.X && y >= bounds.Min.Y && y < bounds.Max.Y {
						img.Set(x, y, fill)
					}
				}
			}
		}
	}

	// Draw outline
	for i := 0; i < len(points); i++ {
		j := (i + 1) % len(points)
		drawLine(img, points[i], points[j], outline, width)
	}
}

// drawLine draws a line between two points
func drawLine(img *image.RGBA, p1, p2 image.Point, col color.RGBA, width int) {
	// Bresenham's line algorithm
	dx := int(math.Abs(float64(p2.X - p1.X)))
	dy := int(math.Abs(float64(p2.Y - p1.Y)))

	sx := -1
	if p1.X < p2.X {
		sx = 1
	}
	sy := -1
	if p1.Y < p2.Y {
		sy = 1
	}

	err := dx - dy
	x, y := p1.X, p1.Y

	for {
		// Draw pixel (with width)
		for wx := -width / 2; wx <= width/2; wx++ {
			for wy := -width / 2; wy <= width/2; wy++ {
				px, py := x+wx, y+wy
				if px >= img.Bounds().Min.X && px < img.Bounds().Max.X &&
					py >= img.Bounds().Min.Y && py < img.Bounds().Max.Y {
					img.Set(px, py, col)
				}
			}
		}

		if x == p2.X && y == p2.Y {
			break
		}

		e2 := 2 * err
		if e2 > -dy {
			err -= dy
			x += sx
		}
		if e2 < dx {
			err += dx
			y += sy
		}
	}
}